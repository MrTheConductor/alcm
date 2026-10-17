"""Sanity-checks the shipped chemistry reference curves under examples/ -
see the README's "Known cell types" for where each one comes from - actually
load, validate, and pack at the real board/cell_count pairings floatwheel
itself ships (see that project's per-target BATTERY_STRING defines) where one
exists."""
import os

import pytest

import lut

_EXAMPLES_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "examples")

# (filename, a real cell_count floatwheel pairs it with, per its own uvprojx build defines)
_REFERENCE_CURVES = [
    ("dg40.json", 20),  # ADV
    ("s50s.json", 20),  # ADV2
]


@pytest.mark.parametrize("filename,cell_count", _REFERENCE_CURVES)
def test_reference_curve_loads_and_packs(filename, cell_count):
    path = os.path.join(_EXAMPLES_DIR, filename)
    curve = lut.load_curve(path)

    # These ship without a baked-in cell_count on purpose (see BatteryCurve's
    # docstring) - every board pairing must come from --cell-count.
    assert curve.cell_count is None

    lut.validate_curve(curve)
    resolved = lut.resolve_cell_count(curve, cell_count)
    block = lut.pack_block(curve, resolved)

    decoded = lut.unpack_block(block)
    assert decoded["magic_ok"] and decoded["schema_ok"] and decoded["crc_ok"]
    assert decoded["cell_count"] == cell_count
    assert decoded["breakpoint_count"] == 11  # 100%->0% in 10% steps, per floatwheel's own table

    # 100% breakpoint should be a 4.2V/cell charge voltage at this cell_count.
    top = decoded["breakpoints"][0]
    assert top.percent_tenths == 1000
    assert top.voltage_tenths == round(4200 * cell_count / 100)


# Curves not from floatwheel's task.c (lygte-info.dk measurements and
# community-provided), so no 4.2V-top guarantee - their 100% point is a
# resting/light-load full-charge voltage. Packed at a real floatwheel board
# pairing where one exists, else a typical 20S.
_COMMUNITY_CURVES = [
    ("vtc6.json", 15),  # PintV, XRV
    ("p42a.json", 18),  # GTV
    ("p42a.json", 20),  # ADV
    ("p28a.json", 20),
    ("rs50.json", 22),  # Floatwheel Atom
    ("p45b.json", 20),
    ("p50b.json", 20),
]


@pytest.mark.parametrize("filename,cell_count", _COMMUNITY_CURVES)
def test_community_curve_loads_and_packs(filename, cell_count):
    curve = lut.load_curve(os.path.join(_EXAMPLES_DIR, filename))
    assert curve.cell_count is None

    decoded = lut.unpack_block(lut.pack_block(curve, cell_count))
    assert decoded["magic_ok"] and decoded["schema_ok"] and decoded["crc_ok"]
    assert decoded["breakpoint_count"] == 11
    assert decoded["breakpoints"][0].percent_tenths == 1000


@pytest.mark.parametrize("filename", sorted({f for f, _ in _COMMUNITY_CURVES} | {"s50s.json", "dg40.json"}))
def test_reference_curve_has_11_descending_breakpoints(filename):
    path = os.path.join(_EXAMPLES_DIR, filename)
    curve = lut.load_curve(path)
    assert len(curve.breakpoints) == 11
    voltages = [bp.cell_mv for bp in curve.breakpoints]
    assert voltages == sorted(voltages, reverse=True)
    percents = [bp.percent for bp in curve.breakpoints]
    assert percents == [100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0]
