#!/usr/bin/env python3
"""Patches a battery voltage-to-SoC lookup table into an ALCM .hex file.

ALCM ships one firmware image for every rider; by default there's no LUT
patched in and battery percentage comes straight from the VESC's own
calculation. This tool binary-patches a rider-specific curve directly into
the distributed .hex file, in the flash region ALCM's firmware reserves
for exactly this purpose - it never touches the firmware source or
requires a rebuild.

Usage:
    battery_lut_patch.py patch --input ALCM.hex --output ALCM_patched.hex --curve my_pack.json
    battery_lut_patch.py verify --input ALCM_patched.hex
    battery_lut_patch.py clear --input ALCM_patched.hex --output ALCM_default.hex
"""
from __future__ import annotations

import argparse
import sys

import intel_hex
import lut
from constants import BATTERY_LUT_FLASH_ADDR, BATTERY_LUT_MAX_BREAKPOINTS, RESERVED_REGION_SIZE


def cmd_patch(args: argparse.Namespace) -> int:
    curve = lut.load_curve(args.curve)
    try:
        cell_count = lut.resolve_cell_count(curve, args.cell_count)
        r_int_milliohms = lut.resolve_r_int_milliohms(curve, args.r_int_milliohms)
        block = lut.pack_block(curve, cell_count, r_int_milliohms)
    except lut.LutError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if len(block) > RESERVED_REGION_SIZE:
        print(
            f"error: packed block is {len(block)} bytes, reserved region is only "
            f"{RESERVED_REGION_SIZE} bytes",
            file=sys.stderr,
        )
        return 1
    block = block.ljust(RESERVED_REGION_SIZE, b"\x00")

    lines = intel_hex.read_lines(args.input)
    try:
        result = intel_hex.patch_region(lines, BATTERY_LUT_FLASH_ADDR, block)
    except intel_hex.IntelHexError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    intel_hex.write_lines(args.output, result.lines)

    print(f"Patched {len(curve.breakpoints)}-point curve, {cell_count}S pack, into {args.output}")
    for bp in curve.breakpoints:
        print(f"  {bp.cell_mv:6.0f}mV/cell -> {bp.percent:5.1f}%")
    if r_int_milliohms:
        print(f"  IR compensation: {r_int_milliohms}m ohm pack resistance")
    else:
        print("  IR compensation: disabled (r_int_milliohms=0)")
    return 0


def cmd_verify(args: argparse.Namespace) -> int:
    lines = intel_hex.read_lines(args.input)
    block_bytes = intel_hex.read_region(lines, BATTERY_LUT_FLASH_ADDR, RESERVED_REGION_SIZE)
    decoded = lut.unpack_block(block_bytes)

    if not decoded["magic_ok"] or not decoded["schema_ok"] or not decoded["crc_ok"]:
        print(f"{args.input}: unpatched (or invalid) - falls back to the VESC's own battery_level")
        if decoded["magic_ok"] and not decoded["crc_ok"]:
            print("  (magic number present but CRC doesn't match - looks corrupt, not just unpatched)")
        return 0

    print(f"{args.input}: patched, {decoded['cell_count']}S pack, "
          f"{decoded['breakpoint_count']} breakpoints:")
    for bp in decoded["breakpoints"]:
        print(f"  {bp.voltage_tenths/10:6.1f}V -> {bp.percent_tenths/10:5.1f}%")
    if decoded["r_int_milliohms"]:
        print(f"  IR compensation: {decoded['r_int_milliohms']}m ohm pack resistance")
    else:
        print("  IR compensation: disabled")
    return 0


def cmd_clear(args: argparse.Namespace) -> int:
    lines = intel_hex.read_lines(args.input)
    try:
        result = intel_hex.patch_region(
            lines, BATTERY_LUT_FLASH_ADDR, b"\x00" * RESERVED_REGION_SIZE
        )
    except intel_hex.IntelHexError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    intel_hex.write_lines(args.output, result.lines)
    print(f"Cleared the battery LUT block in {args.output} - reverts to the VESC's own battery_level")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    p_patch = sub.add_parser("patch", help="patch a curve into a .hex file")
    p_patch.add_argument("--input", required=True, help="the stock ALCM .hex to patch")
    p_patch.add_argument("--output", required=True, help="where to write the patched .hex")
    p_patch.add_argument("--curve", required=True, help=f"a .json or .csv curve file (max {BATTERY_LUT_MAX_BREAKPOINTS} breakpoints)")
    p_patch.add_argument("--cell-count", type=int, default=None,
                          help="series cell count for your pack (e.g. 15 for 15S) - required unless the curve file already specifies one")
    p_patch.add_argument("--r-int-milliohms", type=int, default=None,
                          help="whole-pack internal resistance in milliohms, for IR-drop (load-sag) compensation - optional, defaults to 0 (disabled) unless the curve file specifies one")
    p_patch.set_defaults(func=cmd_patch)

    p_verify = sub.add_parser("verify", help="show what curve (if any) is patched into a .hex file")
    p_verify.add_argument("--input", required=True)
    p_verify.set_defaults(func=cmd_verify)

    p_clear = sub.add_parser("clear", help="remove a patched curve, reverting to VESC passthrough")
    p_clear.add_argument("--input", required=True)
    p_clear.add_argument("--output", required=True)
    p_clear.set_defaults(func=cmd_clear)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
