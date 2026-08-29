import json

import intel_hex
from battery_lut_patch import main
from constants import BATTERY_LUT_FLASH_ADDR, RESERVED_REGION_SIZE


def _make_fixture_lines(fill=0xFF, record_len=16, num_records=4):
    """A synthetic hex image whose data records exactly cover
    [BATTERY_LUT_FLASH_ADDR, BATTERY_LUT_FLASH_ADDR + RESERVED_REGION_SIZE)
    - the real address the CLI patches, unlike test_intel_hex.py's fixture
    which uses an arbitrary base since it exercises patch_region()/
    read_region() directly rather than going through the CLI."""
    base = BATTERY_LUT_FLASH_ADDR
    lines = [intel_hex._build_record(0, 0x04, ((base >> 16) & 0xFFFF).to_bytes(2, "big"))]
    for i in range(num_records):
        addr = (base & 0xFFFF) + i * record_len
        lines.append(intel_hex._build_record(addr, 0x00, bytes([fill]) * record_len))
    lines.append(intel_hex._build_record(0, 0x01, b""))
    return lines


def _write_fixture_hex(path):
    lines = _make_fixture_lines(fill=0xFF, record_len=16, num_records=4)
    assert 4 * 16 == RESERVED_REGION_SIZE
    intel_hex.write_lines(str(path), lines)


def _write_curve(path):
    doc = {
        "cell_count": 10,
        "breakpoints": [
            {"cell_mv": 4200, "percent": 100.0},
            {"cell_mv": 3700, "percent": 50.0},
            {"cell_mv": 3000, "percent": 0.0},
        ],
    }
    path.write_text(json.dumps(doc))


def test_patch_then_verify_round_trip(tmp_path, capsys):
    stock = tmp_path / "stock.hex"
    patched = tmp_path / "patched.hex"
    curve_path = tmp_path / "curve.json"
    _write_fixture_hex(stock)
    _write_curve(curve_path)

    rc = main(["patch", "--input", str(stock), "--output", str(patched), "--curve", str(curve_path)])
    assert rc == 0

    rc = main(["verify", "--input", str(patched)])
    assert rc == 0
    out = capsys.readouterr().out
    assert "patched, 10S pack, 3 breakpoints" in out
    assert "42.0V" in out and "100.0%" in out
    assert "30.0V" in out and "0.0%" in out


def test_patch_with_r_int_milliohms_round_trip(tmp_path, capsys):
    stock = tmp_path / "stock.hex"
    patched = tmp_path / "patched.hex"
    curve_path = tmp_path / "curve.json"
    _write_fixture_hex(stock)
    _write_curve(curve_path)

    rc = main(["patch", "--input", str(stock), "--output", str(patched),
               "--curve", str(curve_path), "--r-int-milliohms", "120"])
    assert rc == 0
    assert "120m ohm" in capsys.readouterr().out

    rc = main(["verify", "--input", str(patched)])
    assert rc == 0
    assert "120m ohm" in capsys.readouterr().out


def test_patch_without_r_int_milliohms_reports_disabled(tmp_path, capsys):
    stock = tmp_path / "stock.hex"
    patched = tmp_path / "patched.hex"
    curve_path = tmp_path / "curve.json"
    _write_fixture_hex(stock)
    _write_curve(curve_path)

    rc = main(["patch", "--input", str(stock), "--output", str(patched), "--curve", str(curve_path)])
    assert rc == 0
    assert "IR compensation: disabled" in capsys.readouterr().out

    rc = main(["verify", "--input", str(patched)])
    assert rc == 0
    assert "IR compensation: disabled" in capsys.readouterr().out


def test_verify_reports_unpatched(tmp_path, capsys):
    stock = tmp_path / "stock.hex"
    _write_fixture_hex(stock)

    rc = main(["verify", "--input", str(stock)])
    assert rc == 0
    out = capsys.readouterr().out
    assert "unpatched" in out


def test_clear_reverts_to_unpatched(tmp_path, capsys):
    stock = tmp_path / "stock.hex"
    patched = tmp_path / "patched.hex"
    cleared = tmp_path / "cleared.hex"
    curve_path = tmp_path / "curve.json"
    _write_fixture_hex(stock)
    _write_curve(curve_path)

    main(["patch", "--input", str(stock), "--output", str(patched), "--curve", str(curve_path)])
    rc = main(["clear", "--input", str(patched), "--output", str(cleared)])
    assert rc == 0

    lines = intel_hex.read_lines(str(cleared))
    region = intel_hex.read_region(lines, BATTERY_LUT_FLASH_ADDR, RESERVED_REGION_SIZE)
    assert region == b"\x00" * RESERVED_REGION_SIZE


def test_patch_rejects_too_many_breakpoints(tmp_path):
    stock = tmp_path / "stock.hex"
    patched = tmp_path / "patched.hex"
    curve_path = tmp_path / "curve.json"
    _write_fixture_hex(stock)

    doc = {
        "cell_count": 10,
        "breakpoints": [{"cell_mv": 4200 - i * 50, "percent": 100 - i * 5} for i in range(15)],
    }
    curve_path.write_text(json.dumps(doc))

    rc = main(["patch", "--input", str(stock), "--output", str(patched), "--curve", str(curve_path)])
    assert rc == 1
