"""Battery LUT breakpoint parsing, packing, and CRC - the pure-Python
mirror of battery_lut.h / battery_lut.c's validation rules."""
from __future__ import annotations

import csv
import json
import struct
from dataclasses import dataclass

from constants import (
    BATTERY_LUT_MAGIC,
    BATTERY_LUT_MAX_BREAKPOINTS,
    BATTERY_LUT_SCHEMA_VERSION,
    BLOCK_STRUCT_FORMAT,
    CRC16_INIT,
    CRC16_POLY,
    PAYLOAD_STRUCT_FORMAT,
)


class LutError(Exception):
    pass


def crc16_ccitt(data: bytes) -> int:
    """CRC-16/XMODEM: poly 0x1021, init 0x0000, MSB-first, non-reflected.
    Must match Library/.../src/crc16_ccitt.c bit-for-bit - see that file's
    table-driven implementation; this is the same algorithm, computed
    without a lookup table since a 60-byte block doesn't need one."""
    crc = CRC16_INIT
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ CRC16_POLY) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


@dataclass
class CellBreakpoint:
    cell_mv: float  # per-cell voltage
    percent: float  # state of charge, 0-100


@dataclass
class PackBreakpoint:
    voltage_tenths: int  # whole-pack voltage, tenths of a volt
    percent_tenths: int  # state of charge, tenths of a percent (0-1000)


@dataclass
class BatteryCurve:
    # None if the curve file doesn't specify one - cell count varies by
    # board even within one cell chemistry (e.g. floatwheel ships P42A
    # cells at both 18S and 20S depending on the board), so the reference
    # chemistry curves under examples/ deliberately leave this unset and
    # require --cell-count on the command line instead of risking a wrong
    # default.
    cell_count: int | None
    breakpoints: list[CellBreakpoint]  # per-cell, descending order by cell_mv
    # Whole-pack internal resistance, milliohms. 0 (the default) means IR
    # (load-sag) compensation is disabled - same "unset = old behavior"
    # philosophy as cell_count, but 0 is itself a meaningful, safe value
    # here rather than requiring an explicit override.
    r_int_milliohms: int = 0


def load_curve(path: str) -> BatteryCurve:
    """Loads a per-cell curve from JSON or CSV. cell_count is optional in
    the file - if omitted, it must be supplied via --cell-count instead.
    r_int_milliohms is also optional and defaults to 0 (no IR compensation).

    JSON: {"cell_count": N, "r_int_milliohms": N, "breakpoints": [{"cell_mv": .., "percent": ..}, ...]}
         ("cell_count" and "r_int_milliohms" may both be omitted)
    CSV:  cell_count,r_int_milliohms,cell_mv,percent   (one header row;
          cell_count/r_int_milliohms only need to be filled in on one
          row, or omitted entirely)
    """
    if path.lower().endswith(".json"):
        with open(path) as f:
            doc = json.load(f)
        cell_count = int(doc["cell_count"]) if "cell_count" in doc and doc["cell_count"] is not None else None
        r_int_milliohms = int(doc.get("r_int_milliohms") or 0)
        raw_points = doc["breakpoints"]
    elif path.lower().endswith(".csv"):
        with open(path, newline="") as f:
            rows = list(csv.DictReader(f))
        if not rows:
            raise LutError(f"{path}: no data rows")
        cell_count = None
        r_int_milliohms = 0
        raw_points = []
        for row in rows:
            if row.get("cell_count"):
                cell_count = int(row["cell_count"])
            if row.get("r_int_milliohms"):
                r_int_milliohms = int(row["r_int_milliohms"])
            raw_points.append({"cell_mv": row["cell_mv"], "percent": row["percent"]})
    else:
        raise LutError(f"{path}: expected a .json or .csv file")

    breakpoints = [
        CellBreakpoint(cell_mv=float(point["cell_mv"]), percent=float(point["percent"]))
        for point in raw_points
    ]
    breakpoints.sort(key=lambda bp: bp.cell_mv, reverse=True)
    return BatteryCurve(cell_count=cell_count, breakpoints=breakpoints, r_int_milliohms=r_int_milliohms)


def validate_curve(curve: BatteryCurve) -> None:
    """Checks breakpoint shape only - cell_count is validated separately
    by resolve_cell_count(), since it may not be known yet at this point
    (deferred to --cell-count)."""
    if len(curve.breakpoints) < 2:
        raise LutError("need at least 2 breakpoints")
    if len(curve.breakpoints) > BATTERY_LUT_MAX_BREAKPOINTS:
        raise LutError(
            f"{len(curve.breakpoints)} breakpoints given, max is {BATTERY_LUT_MAX_BREAKPOINTS}"
        )
    for prev, curr in zip(curve.breakpoints, curve.breakpoints[1:]):
        if curr.cell_mv >= prev.cell_mv:
            raise LutError(
                "breakpoints must be strictly descending in voltage: "
                f"{prev.cell_mv:.0f}mV then {curr.cell_mv:.0f}mV"
            )
        if curr.percent > prev.percent:
            raise LutError(
                "breakpoints must be non-increasing in percent as voltage drops: "
                f"{prev.percent:.1f}% then {curr.percent:.1f}%"
            )
    for bp in curve.breakpoints:
        if bp.cell_mv <= 0:
            raise LutError(f"cell voltage must be positive, got {bp.cell_mv:.0f}mV")
        if not (0 <= bp.percent <= 100):
            raise LutError(f"percent {bp.percent:.1f}% out of range 0-100")


def resolve_cell_count(curve: BatteryCurve, cell_count_override: int | None) -> int:
    """Picks the cell count to pack: the CLI override if given, else
    whatever the curve file specified. Raises if neither is present."""
    cell_count = cell_count_override if cell_count_override is not None else curve.cell_count
    if cell_count is None:
        raise LutError(
            "no cell_count in the curve file and no --cell-count given - "
            "this curve doesn't know your pack's series count"
        )
    if not (1 <= cell_count <= 255):
        raise LutError(f"cell_count must be 1-255, got {cell_count}")
    return cell_count


def resolve_r_int_milliohms(curve: BatteryCurve, override: int | None) -> int:
    """Picks the pack internal resistance to pack: the CLI override if
    given, else whatever the curve file specified, else 0 (compensation
    disabled) - unlike cell_count, there's no error case, since 0 is
    itself a valid, safe default."""
    r_int_milliohms = override if override is not None else curve.r_int_milliohms
    if not (0 <= r_int_milliohms <= 0xFFFF):
        raise LutError(f"r_int_milliohms must be 0-65535, got {r_int_milliohms}")
    return r_int_milliohms


def pack_block(curve: BatteryCurve, cell_count: int, r_int_milliohms: int = 0) -> bytes:
    """Packs a validated curve, at a specific cell count and pack
    internal resistance, into the exact bytes battery_lut_block_t
    expects in flash, including a correct CRC16."""
    validate_curve(curve)
    if not (1 <= cell_count <= 255):
        raise LutError(f"cell_count must be 1-255, got {cell_count}")
    if not (0 <= r_int_milliohms <= 0xFFFF):
        raise LutError(f"r_int_milliohms must be 0-65535, got {r_int_milliohms}")

    # cell_mv descending * a positive cell_count preserves order, so no
    # re-sort needed here.
    pack_breakpoints = []
    for bp in curve.breakpoints:
        # 1 decivolt (voltage_tenths unit) == 100 mV per cell * cell_count.
        voltage_tenths = round(bp.cell_mv * cell_count / 100.0)
        percent_tenths = round(bp.percent * 10.0)
        if not (0 <= voltage_tenths <= 0xFFFF):
            raise LutError(
                f"pack voltage {voltage_tenths/10:.1f}V out of range at cell_count={cell_count}"
            )
        pack_breakpoints.append(PackBreakpoint(voltage_tenths=voltage_tenths, percent_tenths=percent_tenths))

    breakpoint_fields: list[int] = []
    for bp in pack_breakpoints:
        breakpoint_fields.extend([bp.voltage_tenths, bp.percent_tenths])
    # Pad unused breakpoint slots with zeros.
    while len(breakpoint_fields) < BATTERY_LUT_MAX_BREAKPOINTS * 2:
        breakpoint_fields.extend([0, 0])

    payload = struct.pack(
        PAYLOAD_STRUCT_FORMAT,
        BATTERY_LUT_MAGIC,
        BATTERY_LUT_SCHEMA_VERSION,
        cell_count,
        len(pack_breakpoints),
        0,  # reserved0
        *breakpoint_fields,
        r_int_milliohms,
        0,  # reserved2
    )

    crc = crc16_ccitt(payload)
    return payload + struct.pack("<HH", crc, 0)  # crc16, reserved1


def unpack_block(block_bytes: bytes) -> dict:
    """Unpacks raw flash bytes for `verify` - does not validate, just
    decodes for display. Returns a dict with the raw whole-pack fields.

    Accepts anything at least struct-sized - the caller may hand over the
    full padded reserved region (RESERVED_REGION_SIZE, larger than the
    packed struct) rather than trimming it themselves."""
    struct_size = struct.calcsize(BLOCK_STRUCT_FORMAT)
    if len(block_bytes) < struct_size:
        raise ValueError(f"need at least {struct_size} bytes, got {len(block_bytes)}")
    fields = struct.unpack(BLOCK_STRUCT_FORMAT, block_bytes[:struct_size])
    magic = fields[0]
    schema_version, cell_count, breakpoint_count, _reserved0 = fields[1:5]
    raw_breakpoints = fields[5 : 5 + BATTERY_LUT_MAX_BREAKPOINTS * 2]
    r_int_milliohms, _reserved2 = fields[5 + BATTERY_LUT_MAX_BREAKPOINTS * 2 : 7 + BATTERY_LUT_MAX_BREAKPOINTS * 2]
    crc16, _reserved1 = fields[7 + BATTERY_LUT_MAX_BREAKPOINTS * 2 :]

    breakpoints = []
    for i in range(min(breakpoint_count, BATTERY_LUT_MAX_BREAKPOINTS)):
        voltage_tenths = raw_breakpoints[i * 2]
        percent_tenths = raw_breakpoints[i * 2 + 1]
        breakpoints.append(PackBreakpoint(voltage_tenths=voltage_tenths, percent_tenths=percent_tenths))

    payload = block_bytes[: struct.calcsize(BLOCK_STRUCT_FORMAT) - 4]
    crc_ok = crc16_ccitt(payload) == crc16

    return {
        "magic": magic,
        "magic_ok": magic == BATTERY_LUT_MAGIC,
        "schema_version": schema_version,
        "schema_ok": schema_version == BATTERY_LUT_SCHEMA_VERSION,
        "cell_count": cell_count,
        "breakpoint_count": breakpoint_count,
        "breakpoints": breakpoints,
        "r_int_milliohms": r_int_milliohms,
        "crc16": crc16,
        "crc_ok": crc_ok,
    }
