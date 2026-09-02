"""Erases and flashes a .hex file to an ALCM board via pyocd + ST-Link.

Ported from floatwheel's own flash.bat/flash.sh (floatwheel/LCM/), which
proved this exact pyocd + HK32 CMSIS-pack combination works over the same
ST-Link setup ALCM's own Keil project is configured for (Keil's own
"Download" only flashes the current project's build output, not an
arbitrary external .hex, which is why this exists as a separate path).

Needs, alongside this file: HKMicroChip.HK32F030xMxx_DFP.1.0.17.pack (the
HK32-specific flash algorithm, since this chip isn't in pyocd's built-in
target list) and pyocd.yaml (just points pyocd at that pack).
"""
from __future__ import annotations

import os
import shutil
import subprocess

TARGET = "hk32f030mf4p6"
PACK_FILENAME = "HKMicroChip.HK32F030xMxx_DFP.1.0.17.pack"
CONFIG_FILENAME = "pyocd.yaml"


class FlashError(Exception):
    pass


def _tool_dir() -> str:
    return os.path.dirname(os.path.abspath(__file__))


def check_prerequisites() -> None:
    """Raises FlashError with a clear message if pyocd or its supporting
    files aren't available - mirrors flash.bat/flash.sh's own upfront
    checks, so a setup problem is caught before anything touches the board."""
    if shutil.which("pyocd") is None:
        raise FlashError(
            "pyocd not found on PATH. Install it with:\n"
            "  python -m pip install --upgrade pyocd==0.34.3"
        )

    pack_path = os.path.join(_tool_dir(), PACK_FILENAME)
    if not os.path.isfile(pack_path):
        raise FlashError(f"{PACK_FILENAME} not found next to this script ({_tool_dir()})")

    config_path = os.path.join(_tool_dir(), CONFIG_FILENAME)
    if not os.path.isfile(config_path):
        raise FlashError(f"{CONFIG_FILENAME} not found next to this script ({_tool_dir()})")


def erase(runner=subprocess.run):
    """Erases the chip's flash. Raises FlashError if pyocd reports failure."""
    config_path = os.path.join(_tool_dir(), CONFIG_FILENAME)
    result = runner(["pyocd", "erase", "-c", "-t", TARGET, "--config", config_path])
    if result.returncode != 0:
        raise FlashError(f"pyocd erase failed (exit code {result.returncode})")


def load(hex_path: str, runner=subprocess.run):
    """Flashes hex_path onto the board. Raises FlashError if pyocd reports
    failure."""
    config_path = os.path.join(_tool_dir(), CONFIG_FILENAME)
    result = runner(["pyocd", "load", hex_path, "-t", TARGET, "--config", config_path])
    if result.returncode != 0:
        raise FlashError(f"pyocd load failed (exit code {result.returncode})")
