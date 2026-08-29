"""Mirrors Library/HK32F030Mxx_Library_V1.1.6/HK32F030M_Project/inc/battery_lut.h.

Kept in sync by tests/test_constants_sync.py, which parses the real header
and asserts every value here matches it - there's no build-time codegen
tying these together, so that test is what catches drift.
"""
import struct

BATTERY_LUT_MAGIC = 0x42434C41  # "ALCB"
BATTERY_LUT_SCHEMA_VERSION = 2  # v2 adds r_int_milliohms
BATTERY_LUT_MAX_BREAKPOINTS = 11  # floatwheel's own reference curves use 11 points, 100%->0% in 10% steps
BATTERY_LUT_FLASH_ADDR = 0x08003FC0

# struct battery_lut_payload_t, little-endian (Cortex-M0 is configured
# ELITTLE), no compiler-inserted padding:
#   uint32_t magic
#   uint8_t  schema_version
#   uint8_t  cell_count
#   uint8_t  breakpoint_count
#   uint8_t  reserved0
#   { uint16_t voltage_tenths; uint16_t percent_tenths; } breakpoints[11]
#   uint16_t r_int_milliohms
#   uint16_t reserved2
PAYLOAD_STRUCT_FORMAT = "<I4B" + "HH" * BATTERY_LUT_MAX_BREAKPOINTS + "HH"
PAYLOAD_SIZE = struct.calcsize(PAYLOAD_STRUCT_FORMAT)  # 56 with MAX_BREAKPOINTS == 11

# struct battery_lut_block_t = payload + uint16_t crc16 + uint16_t reserved1
BLOCK_STRUCT_FORMAT = PAYLOAD_STRUCT_FORMAT + "HH"
BLOCK_SIZE = struct.calcsize(BLOCK_STRUCT_FORMAT)  # 60

# Reserved flash region is rounded up from BLOCK_SIZE - see
# Project/MDK5/battery_lut.sct.
RESERVED_REGION_SIZE = 0x40  # 64

# CRC16-CCITT (XModem variant: poly 0x1021, init 0x0000) matching
# Library/HK32F030Mxx_Library_V1.1.6/HK32F030M_Project/src/crc16_ccitt.c
# exactly. NOT the more common 0xFFFF-init "CCITT-FALSE" variant.
CRC16_POLY = 0x1021
CRC16_INIT = 0x0000
