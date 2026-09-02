"""Asserts constants.py hasn't drifted from the real C header. There's no
build-time codegen tying the two together (see constants.py's docstring),
so this test is what catches it if someone changes one without the other.
"""
import os
import re

import constants

_HEADER_RELATIVE_PATH = os.path.join(
    "Library",
    "HK32F030Mxx_Library_V1.1.6",
    "HK32F030M_Project",
    "inc",
    "battery_lut.h",
)

_DEFINE_RE = re.compile(
    r"#define\s+(\w+)\s+\(\(uint\d+_t\)\s*(0[xX][0-9A-Fa-f]+|\d+)u?\)"
)


def _repo_root():
    # tools/battery_lut_patch/tests/ -> tools/battery_lut_patch/ -> tools/ -> repo root
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(here, "..", "..", ".."))


def _parse_header_defines():
    path = os.path.join(_repo_root(), _HEADER_RELATIVE_PATH)
    with open(path) as f:
        text = f.read()

    values = {}
    for match in _DEFINE_RE.finditer(text):
        name, value = match.groups()
        values[name] = int(value, 0)
    return values


def test_header_defines_are_found():
    values = _parse_header_defines()
    # If this is empty, the regex above no longer matches the header's
    # actual formatting - fix the regex, don't just let this test vanish.
    assert "BATTERY_LUT_MAGIC" in values


def test_constants_match_header():
    values = _parse_header_defines()
    assert values["BATTERY_LUT_MAGIC"] == constants.BATTERY_LUT_MAGIC
    assert values["BATTERY_LUT_SCHEMA_VERSION"] == constants.BATTERY_LUT_SCHEMA_VERSION
    assert values["BATTERY_LUT_MAX_BREAKPOINTS"] == constants.BATTERY_LUT_MAX_BREAKPOINTS
    assert values["BATTERY_LUT_FLASH_ADDR"] == constants.BATTERY_LUT_FLASH_ADDR
