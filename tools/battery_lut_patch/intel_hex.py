"""Minimal Intel HEX reader/patcher, stdlib-only.

Patches bytes into an existing Intel HEX file in place: it does not
re-serialize the whole file, it only rewrites the data records that
overlap the target address range (recomputing just those records'
checksums) and leaves every other line byte-for-byte untouched. This
avoids needing to reproduce every quirk of the toolchain's own HEX
output.

Handles record type 00 (data) and 04 (extended linear address, needed
because ALCM's flash lives at 0x08000000 - above what a single 16-bit
record address can express). Other record types (01 EOF, 02/03 segment
address) are passed through unchanged.
"""
from __future__ import annotations

from dataclasses import dataclass


RECORD_DATA = 0x00
RECORD_EOF = 0x01
RECORD_EXTENDED_LINEAR_ADDRESS = 0x04


class IntelHexError(Exception):
    pass


def _checksum(byte_values: bytes) -> int:
    """Two's-complement checksum over every byte in a record except the
    checksum byte itself (len, addr_hi, addr_lo, type, data...)."""
    return (0x100 - (sum(byte_values) & 0xFF)) & 0xFF


def _parse_record(line: str) -> tuple[int, int, int, bytes, int]:
    line = line.strip()
    if not line.startswith(":"):
        raise IntelHexError(f"not an Intel HEX record: {line!r}")
    raw = bytes.fromhex(line[1:])
    if len(raw) < 5:
        raise IntelHexError(f"record too short: {line!r}")
    length, addr_hi, addr_lo, rec_type = raw[0], raw[1], raw[2], raw[3]
    data = raw[4 : 4 + length]
    checksum = raw[4 + length]
    if len(data) != length:
        raise IntelHexError(f"record length mismatch: {line!r}")
    if _checksum(raw[:-1]) != checksum:
        raise IntelHexError(f"bad checksum in record: {line!r}")
    address = (addr_hi << 8) | addr_lo
    return length, address, rec_type, data, checksum


def _build_record(address: int, rec_type: int, data: bytes) -> str:
    addr_hi = (address >> 8) & 0xFF
    addr_lo = address & 0xFF
    body = bytes([len(data), addr_hi, addr_lo, rec_type]) + data
    checksum = _checksum(body)
    return ":" + (body + bytes([checksum])).hex().upper()


@dataclass
class PatchResult:
    lines: list[str]
    bytes_patched: int


def read_lines(path: str) -> list[str]:
    with open(path, "r", newline=None) as f:
        return [line.rstrip("\r\n") for line in f if line.strip()]


def write_lines(path: str, lines: list[str]) -> None:
    with open(path, "w", newline="\r\n") as f:
        for line in lines:
            f.write(line + "\r\n")


def read_region(lines: list[str], base_address: int, length: int) -> bytes:
    """Reads `length` bytes starting at `base_address` out of the hex
    image, wherever they land across one or more data records. Bytes not
    covered by any record come back as 0x00."""
    result = bytearray(length)
    covered = bytearray(length)
    ext_addr = 0

    for line in lines:
        rec_len, addr, rec_type, data, _ = _parse_record(line)
        if rec_type == RECORD_EXTENDED_LINEAR_ADDRESS:
            ext_addr = int.from_bytes(data, "big") << 16
            continue
        if rec_type != RECORD_DATA:
            continue

        abs_addr = ext_addr + addr
        overlap_start = max(abs_addr, base_address)
        overlap_end = min(abs_addr + rec_len, base_address + length)
        if overlap_start >= overlap_end:
            continue

        rec_offset = overlap_start - abs_addr
        out_offset = overlap_start - base_address
        span = overlap_end - overlap_start
        result[out_offset : out_offset + span] = data[rec_offset : rec_offset + span]
        covered[out_offset : out_offset + span] = b"\x01" * span

    return bytes(result)


_INSERT_CHUNK_SIZE = 16  # matches this toolchain's own record length; any size <=255 would work


def patch_region(lines: list[str], base_address: int, patch_bytes: bytes) -> PatchResult:
    """Overwrites `patch_bytes` into the hex image starting at
    `base_address`, rewriting any existing data records that overlap that
    range in place (recomputing their checksums) and leaving everything
    else byte-for-byte untouched.

    A scatter-reserved region that the linker never populates (an `EMPTY`
    execution region, as ALCM's battery LUT block is) has no record at
    all for its address range - flash there is implicitly erased/0xFF,
    same as any other never-written address. For any part of the target
    range not already covered by a record, this synthesizes new type-00
    data records (chunked to _INSERT_CHUNK_SIZE bytes) and inserts them
    - preceded by a fresh extended-linear-address record for
    base_address's upper 16 bits, so the insertion is correct regardless
    of what context was active nearby - just before the file's EOF
    record. Record order doesn't matter to any Intel HEX reader (each
    record carries its own absolute address), so insertion position
    otherwise doesn't matter.
    """
    out_lines: list[str] = []
    ext_addr = 0
    bytes_patched = 0
    end_address = base_address + len(patch_bytes)
    covered = bytearray(len(patch_bytes))
    eof_index = len(lines)

    for i, line in enumerate(lines):
        rec_len, addr, rec_type, data, _ = _parse_record(line)

        if rec_type == RECORD_EXTENDED_LINEAR_ADDRESS:
            ext_addr = int.from_bytes(data, "big") << 16
            out_lines.append(line)
            continue

        if rec_type == RECORD_EOF:
            eof_index = len(out_lines)
            out_lines.append(line)
            continue

        if rec_type != RECORD_DATA:
            out_lines.append(line)
            continue

        abs_addr = ext_addr + addr
        overlap_start = max(abs_addr, base_address)
        overlap_end = min(abs_addr + rec_len, end_address)

        if overlap_start >= overlap_end:
            out_lines.append(line)
            continue

        rec_offset = overlap_start - abs_addr
        patch_offset = overlap_start - base_address
        span = overlap_end - overlap_start

        new_data = bytearray(data)
        new_data[rec_offset : rec_offset + span] = patch_bytes[patch_offset : patch_offset + span]
        bytes_patched += span
        covered[patch_offset : patch_offset + span] = b"\x01" * span

        out_lines.append(_build_record(addr, rec_type, bytes(new_data)))

    # Fill any gaps (bytes no existing record covered) with newly
    # synthesized records, inserted just before EOF.
    new_lines: list[str] = []
    offset = 0
    while offset < len(patch_bytes):
        if covered[offset]:
            offset += 1
            continue
        gap_start = offset
        while offset < len(patch_bytes) and not covered[offset]:
            offset += 1
        gap_bytes = patch_bytes[gap_start:offset]
        gap_addr = base_address + gap_start
        new_lines.append(
            _build_record(0, RECORD_EXTENDED_LINEAR_ADDRESS, ((gap_addr >> 16) & 0xFFFF).to_bytes(2, "big"))
        )
        for chunk_start in range(0, len(gap_bytes), _INSERT_CHUNK_SIZE):
            chunk = gap_bytes[chunk_start : chunk_start + _INSERT_CHUNK_SIZE]
            chunk_addr = gap_addr + chunk_start
            new_lines.append(_build_record(chunk_addr & 0xFFFF, RECORD_DATA, chunk))
            bytes_patched += len(chunk)

    out_lines[eof_index:eof_index] = new_lines

    if bytes_patched != len(patch_bytes):
        raise IntelHexError(
            f"only patched {bytes_patched} of {len(patch_bytes)} bytes at "
            f"0x{base_address:08X} - this shouldn't happen (overlapping gap "
            "accounting bug)"
        )

    return PatchResult(lines=out_lines, bytes_patched=bytes_patched)
