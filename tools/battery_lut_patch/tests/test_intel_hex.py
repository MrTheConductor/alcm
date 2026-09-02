import pytest

import intel_hex


def make_fixture_lines(base_upper16=0x0800, record_len=16, num_records=8, fill=0xFF):
    """Builds a synthetic hex image: one extended-linear-address record for
    `base_upper16`, followed by `num_records` data records of `record_len`
    bytes each, filled with `fill`, starting at absolute address
    base_upper16<<16."""
    lines = [intel_hex._build_record(0, 0x04, base_upper16.to_bytes(2, "big"))]
    for i in range(num_records):
        addr = (i * record_len) & 0xFFFF
        data = bytes([fill]) * record_len
        lines.append(intel_hex._build_record(addr, 0x00, data))
    lines.append(intel_hex._build_record(0, 0x01, b""))
    return lines


def test_read_region_roundtrips_fill_value():
    lines = make_fixture_lines(fill=0xAB)
    base = 0x08000000
    region = intel_hex.read_region(lines, base + 16, 16)
    assert region == bytes([0xAB]) * 16


def test_patch_region_single_record():
    lines = make_fixture_lines(fill=0x00)
    base = 0x08000000
    patch = bytes(range(16))
    result = intel_hex.patch_region(lines, base + 32, patch)
    assert result.bytes_patched == 16

    readback = intel_hex.read_region(result.lines, base + 32, 16)
    assert readback == patch

    # Untouched neighboring record should be unaffected.
    neighbor = intel_hex.read_region(result.lines, base + 48, 16)
    assert neighbor == bytes([0x00]) * 16


def test_patch_region_spans_multiple_records():
    lines = make_fixture_lines(fill=0x00, record_len=16, num_records=8)
    base = 0x08000000
    # A 64-byte patch starting mid-record, spanning parts of 5 records.
    patch = bytes(range(64))
    target = base + 24
    result = intel_hex.patch_region(lines, target, patch)
    assert result.bytes_patched == 64

    readback = intel_hex.read_region(result.lines, target, 64)
    assert readback == patch

    # Bytes just before/after the patch remain the original fill.
    before = intel_hex.read_region(result.lines, target - 8, 8)
    assert before == bytes([0x00]) * 8
    after = intel_hex.read_region(result.lines, target + 64, 8)
    assert after == bytes([0x00]) * 8


def test_patch_region_recomputes_checksum():
    lines = make_fixture_lines(fill=0x00)
    base = 0x08000000
    result = intel_hex.patch_region(lines, base, bytes(range(16)))
    # Every emitted line must itself be re-parseable (parsing verifies its
    # own checksum internally and raises if it's wrong).
    for line in result.lines:
        intel_hex._parse_record(line)


def test_patch_region_fills_uncovered_gap():
    # An EMPTY scatter region (like ALCM's reserved battery LUT block)
    # never gets an input section placed in it, so the toolchain's .hex
    # has no record at all for its address range - patch_region must
    # synthesize new records to fill that gap, not just patch existing
    # ones in place.
    lines = make_fixture_lines(fill=0x00, num_records=4)  # covers 0x08000000-0x0800003F
    base = 0x08000000
    patch = bytes(range(16))

    result = intel_hex.patch_region(lines, base + 0x1000, patch)
    assert result.bytes_patched == 16

    readback = intel_hex.read_region(result.lines, base + 0x1000, 16)
    assert readback == patch

    # Every emitted line (including the newly synthesized ones) must be
    # individually well-formed.
    for line in result.lines:
        intel_hex._parse_record(line)

    # The file must still end with the EOF record.
    assert intel_hex._parse_record(result.lines[-1])[2] == intel_hex.RECORD_EOF


def test_patch_region_fills_partial_gap_alongside_existing_record():
    # A patch that's half covered by an existing record and half in a
    # gap must do both: overwrite the covered part in place, synthesize
    # records for the rest.
    lines = make_fixture_lines(fill=0x00, num_records=1)  # covers 0x08000000-0x0800000F only
    base = 0x08000000
    patch = bytes(range(32))  # bytes 0-15 land in the existing record, 16-31 in a gap

    result = intel_hex.patch_region(lines, base, patch)
    assert result.bytes_patched == 32
    assert intel_hex.read_region(result.lines, base, 32) == patch
    # Original record count (ext-addr + 1 data + eof) plus 1 new ext-addr
    # + 1 new data record for the gap.
    assert len(result.lines) == len(lines) + 2


def test_bad_checksum_raises():
    lines = make_fixture_lines()
    original_checksum = int(lines[1][-2:], 16)
    flipped = original_checksum ^ 0xFF  # guaranteed different from the original
    corrupted = lines[1][:-2] + f"{flipped:02X}"
    with pytest.raises(intel_hex.IntelHexError):
        intel_hex._parse_record(corrupted)


def test_read_write_roundtrip(tmp_path):
    lines = make_fixture_lines(fill=0x42)
    path = tmp_path / "test.hex"
    intel_hex.write_lines(str(path), lines)
    read_back = intel_hex.read_lines(str(path))
    assert read_back == lines
