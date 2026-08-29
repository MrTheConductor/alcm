import json

import pytest

import lut
from constants import BATTERY_LUT_MAX_BREAKPOINTS, BLOCK_SIZE


# Same known-answer vectors as
# Library/HK32F030Mxx_Library_V1.1.6/HK32F030M_Project/tests/test_crc_ccitt.h
# - this tool's CRC must match the firmware's crc16_ccitt.c bit-for-bit, or
# every patched block silently fails on-device validation.
def test_crc16_matches_firmware_vectors():
    assert lut.crc16_ccitt(b"") == 0x0000
    assert lut.crc16_ccitt(bytes([0x31])) == 0x2672
    assert lut.crc16_ccitt(bytes([0x12, 0x34, 0x56, 0x78])) == 0xB42C
    assert lut.crc16_ccitt(b"123456789") == 0x31C3
    assert lut.crc16_ccitt(bytes(range(256))) == 0x7E55


def test_load_curve_json_with_cell_count(tmp_path):
    doc = {
        "cell_count": 10,
        "breakpoints": [
            {"cell_mv": 4200, "percent": 100.0},
            {"cell_mv": 3700, "percent": 50.0},
            {"cell_mv": 3000, "percent": 0.0},
        ],
    }
    path = tmp_path / "curve.json"
    path.write_text(json.dumps(doc))

    curve = lut.load_curve(str(path))
    assert curve.cell_count == 10
    assert [bp.cell_mv for bp in curve.breakpoints] == [4200, 3700, 3000]
    assert [bp.percent for bp in curve.breakpoints] == [100.0, 50.0, 0.0]


def test_load_curve_json_without_cell_count(tmp_path):
    # The shipped chemistry reference curves (examples/*.json) deliberately
    # omit cell_count - it varies by board even within one chemistry.
    doc = {
        "breakpoints": [
            {"cell_mv": 4200, "percent": 100.0},
            {"cell_mv": 3000, "percent": 0.0},
        ],
    }
    path = tmp_path / "curve.json"
    path.write_text(json.dumps(doc))

    curve = lut.load_curve(str(path))
    assert curve.cell_count is None


def test_load_curve_csv(tmp_path):
    path = tmp_path / "curve.csv"
    path.write_text("cell_count,cell_mv,percent\n10,4200,100\n,3700,50\n,3000,0\n")

    curve = lut.load_curve(str(path))
    assert curve.cell_count == 10
    assert [bp.cell_mv for bp in curve.breakpoints] == [4200, 3700, 3000]


def test_load_curve_sorts_descending(tmp_path):
    # Deliberately out of order in the source file.
    doc = {
        "cell_count": 10,
        "breakpoints": [
            {"cell_mv": 3000, "percent": 0.0},
            {"cell_mv": 4200, "percent": 100.0},
            {"cell_mv": 3700, "percent": 50.0},
        ],
    }
    path = tmp_path / "curve.json"
    path.write_text(json.dumps(doc))

    curve = lut.load_curve(str(path))
    assert [bp.cell_mv for bp in curve.breakpoints] == [4200, 3700, 3000]


def _curve(cell_count, pairs):
    """pairs: list of (cell_mv, percent)."""
    return lut.BatteryCurve(
        cell_count=cell_count,
        breakpoints=[lut.CellBreakpoint(cell_mv=v, percent=p) for v, p in pairs],
    )


def test_validate_curve_rejects_too_few_breakpoints():
    with pytest.raises(lut.LutError):
        lut.validate_curve(_curve(10, [(4200, 100)]))


def test_validate_curve_rejects_too_many_breakpoints():
    pairs = [(4200 - i * 100, 100 - i * 10) for i in range(BATTERY_LUT_MAX_BREAKPOINTS + 1)]
    with pytest.raises(lut.LutError):
        lut.validate_curve(_curve(10, pairs))


def test_validate_curve_rejects_non_descending_voltage():
    with pytest.raises(lut.LutError):
        lut.validate_curve(_curve(10, [(3700, 100), (4200, 50), (3000, 0)]))


def test_validate_curve_rejects_increasing_percent():
    with pytest.raises(lut.LutError):
        lut.validate_curve(_curve(10, [(4200, 50), (3700, 100), (3000, 0)]))


def test_resolve_cell_count_prefers_override():
    curve = _curve(10, [(4200, 100), (3000, 0)])
    assert lut.resolve_cell_count(curve, 15) == 15


def test_resolve_cell_count_falls_back_to_curve():
    curve = _curve(10, [(4200, 100), (3000, 0)])
    assert lut.resolve_cell_count(curve, None) == 10


def test_resolve_cell_count_raises_when_neither_given():
    curve = _curve(None, [(4200, 100), (3000, 0)])
    with pytest.raises(lut.LutError):
        lut.resolve_cell_count(curve, None)


def test_resolve_cell_count_rejects_out_of_range():
    curve = _curve(None, [(4200, 100), (3000, 0)])
    with pytest.raises(lut.LutError):
        lut.resolve_cell_count(curve, 0)
    with pytest.raises(lut.LutError):
        lut.resolve_cell_count(curve, 300)


def test_resolve_r_int_milliohms_prefers_override():
    curve = _curve(10, [(4200, 100), (3000, 0)])
    curve.r_int_milliohms = 50
    assert lut.resolve_r_int_milliohms(curve, 100) == 100


def test_resolve_r_int_milliohms_falls_back_to_curve():
    curve = _curve(10, [(4200, 100), (3000, 0)])
    curve.r_int_milliohms = 50
    assert lut.resolve_r_int_milliohms(curve, None) == 50


def test_resolve_r_int_milliohms_defaults_to_zero():
    curve = _curve(10, [(4200, 100), (3000, 0)])
    assert curve.r_int_milliohms == 0
    assert lut.resolve_r_int_milliohms(curve, None) == 0


def test_resolve_r_int_milliohms_rejects_out_of_range():
    curve = _curve(10, [(4200, 100), (3000, 0)])
    with pytest.raises(lut.LutError):
        lut.resolve_r_int_milliohms(curve, -1)
    with pytest.raises(lut.LutError):
        lut.resolve_r_int_milliohms(curve, 70000)


def test_pack_and_unpack_block_roundtrip():
    curve = _curve(10, [(4200, 100), (3700, 50), (3000, 0)])
    block = lut.pack_block(curve, cell_count=10)
    assert len(block) == BLOCK_SIZE

    decoded = lut.unpack_block(block)
    assert decoded["magic_ok"]
    assert decoded["schema_ok"]
    assert decoded["crc_ok"]
    assert decoded["cell_count"] == 10
    assert decoded["breakpoint_count"] == 3
    assert decoded["r_int_milliohms"] == 0  # default, IR compensation disabled
    # 4200mV * 10 cells / 100 = 420 (42.0V pack, tenths-of-a-volt)
    assert [(bp.voltage_tenths, bp.percent_tenths) for bp in decoded["breakpoints"]] == [
        (420, 1000),
        (370, 500),
        (300, 0),
    ]


def test_pack_block_includes_r_int_milliohms():
    curve = _curve(10, [(4200, 100), (3000, 0)])
    block = lut.pack_block(curve, cell_count=10, r_int_milliohms=150)
    decoded = lut.unpack_block(block)
    assert decoded["crc_ok"]
    assert decoded["r_int_milliohms"] == 150


def test_pack_block_scales_by_cell_count():
    curve = _curve(None, [(4200, 100), (3000, 0)])
    block_15s = lut.pack_block(curve, cell_count=15)
    decoded = lut.unpack_block(block_15s)
    assert decoded["cell_count"] == 15
    # 4200mV * 15 / 100 = 630 (63.0V pack)
    assert decoded["breakpoints"][0].voltage_tenths == 630


def test_unpack_block_detects_corruption():
    curve = _curve(10, [(4200, 100), (3700, 50), (3000, 0)])
    block = bytearray(lut.pack_block(curve, cell_count=10))
    block[0] ^= 0xFF  # corrupt the magic number
    decoded = lut.unpack_block(bytes(block))
    assert not decoded["magic_ok"]
