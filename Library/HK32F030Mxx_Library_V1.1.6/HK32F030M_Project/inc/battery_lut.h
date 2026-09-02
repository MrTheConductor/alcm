/*
 * Copyright (c) 2024-2026, Mitchell White <mitchell.n.white@gmail.com>
 *
 * This file is part of Advanced LCM (ALCM) project.
 *
 * ALCM is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ALCM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with ALCM. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file battery_lut.h
 * @brief Optional, field-patchable voltage-to-state-of-charge lookup table
 *
 * ALCM ships one firmware image for every rider. By default no lookup
 * table is patched into flash, and battery percentage comes straight from
 * the VESC's own calculation, unchanged. Riders who want a curve
 * calibrated to their specific pack's chemistry use the
 * tools/battery_lut_patch Python tool to binary-patch a table directly
 * into the distributed .hex file before flashing - firmware never writes
 * this block itself.
 *
 * The same block optionally also carries the pack's internal resistance
 * (r_int_milliohms) for IR-drop (load-sag) compensation - see
 * battery_lut_compensate_voltage(). 0/unset simply disables it.
 *
 * The patched block lives at a fixed flash address (BATTERY_LUT_FLASH_ADDR)
 * reserved by the linker scatter file (Project/MDK5/battery_lut.sct). It is
 * validated (magic number, schema version, CRC16, breakpoint monotonicity)
 * once at boot; if anything doesn't check out, the module reports itself
 * invalid and vesc_serial.c falls back to the VESC's own battery_level,
 * unchanged.
 */
#ifndef BATTERY_LUT_H
#define BATTERY_LUT_H

#include "lcm_types.h"

#define BATTERY_LUT_MAGIC ((uint32_t)0x42434C41u) // "ALCB"
#define BATTERY_LUT_SCHEMA_VERSION ((uint8_t)2u) // v2 adds r_int_milliohms
#define BATTERY_LUT_MAX_BREAKPOINTS ((uint8_t)11u) // floatwheel's own reference curves (task.c CheckPowerLevel) use 11 points, 100%->0% in 10% steps
#define BATTERY_LUT_FLASH_ADDR ((uint32_t)0x08003FC0u)

/**
 * @brief A single voltage/state-of-charge breakpoint.
 */
typedef struct
{
    uint16_t voltage_tenths; // Whole-pack voltage, tenths of a volt, descending order in the table
    uint16_t percent_tenths; // State of charge, tenths of a percent (0-1000)
} battery_lut_breakpoint_t;  // 4 bytes

/**
 * @brief The CRC-covered contents of the patchable block.
 */
typedef struct
{
    uint32_t magic;
    uint8_t schema_version;
    uint8_t cell_count;       // Informational/sanity bound only - voltage_tenths is already whole-pack
    uint8_t breakpoint_count; // Number of valid entries in breakpoints[], 2..BATTERY_LUT_MAX_BREAKPOINTS
    uint8_t reserved0;        // Keeps breakpoints[] 4-byte aligned
    battery_lut_breakpoint_t breakpoints[BATTERY_LUT_MAX_BREAKPOINTS];
    uint16_t r_int_milliohms; // Whole-pack internal resistance, milliohms. 0 = IR compensation disabled.
    uint16_t reserved2;       // Keeps the struct a multiple of 4 bytes (its own alignment requirement)
} battery_lut_payload_t; // 56 bytes with BATTERY_LUT_MAX_BREAKPOINTS == 11 - this is exactly what the CRC covers

/**
 * @brief The full patchable block, as it sits in flash.
 */
typedef struct
{
    battery_lut_payload_t payload;
    uint16_t crc16;     // crc16_ccitt(&payload, sizeof(payload))
    uint16_t reserved1; // Reserved for a future schema field
} battery_lut_block_t;  // 60 bytes - reserved flash region (Project/MDK5/battery_lut.sct) is rounded up to 64

/**
 * @brief Validates a battery LUT block.
 *
 * Checks the magic number, schema version, cell count, CRC16, and that the
 * breakpoints are strictly descending in voltage and non-increasing in
 * percent. The monotonicity check exists because battery_lut_interpolate()'s
 * math is unsigned - a corrupt table that happens to still pass its CRC,
 * with an out-of-order voltage or an increasing percent between adjacent
 * breakpoints, would underflow-wrap to a huge value rather than fail safely.
 *
 * @param block Pointer to the block to validate.
 * @return true if the block is valid and safe to use for interpolation.
 */
bool_t battery_lut_validate_block(const battery_lut_block_t *block);

/**
 * @brief Interpolates a state-of-charge percentage from a whole-pack voltage.
 *
 * Caller must have already validated the block with
 * battery_lut_validate_block(). Voltages at or above the top breakpoint
 * clamp to its percentage; voltages at or below the bottom breakpoint
 * clamp to its percentage.
 *
 * @param block Pointer to a validated block.
 * @param voltage_tenths Whole-pack voltage, tenths of a volt.
 * @return State of charge, tenths of a percent (0-1000).
 */
int16_t battery_lut_interpolate(const battery_lut_block_t *block, uint16_t voltage_tenths);

/**
 * @brief Initializes the battery LUT module.
 *
 * Reads the flash-resident block (via battery_lut_hw_get_block()) and
 * validates it once, caching only the pass/fail result.
 */
lcm_status_t battery_lut_init(void);

/**
 * @brief Returns whether a valid, patched LUT block is present.
 */
bool_t battery_lut_is_valid(void);

/**
 * @brief Looks up the state of charge for a whole-pack voltage.
 *
 * Only meaningful when battery_lut_is_valid() is true - callers must check
 * that first.
 *
 * @param voltage_tenths Whole-pack voltage, tenths of a volt.
 * @return State of charge, tenths of a percent (0-1000).
 */
int16_t battery_lut_get_percent(uint16_t voltage_tenths);

/**
 * @brief Computes an IR-drop (load-sag) compensation delta.
 *
 * Under load, terminal voltage sags below open-circuit voltage by
 * roughly I*R_int; this returns that delta (V_ocv - V_term, in tenths of
 * a volt) so the caller can add it back before feeding the LUT -
 * positive while discharging (terminal voltage is corrected upward),
 * negative during regen braking (corrected back down). Returns 0 if the
 * block's r_int_milliohms is 0 (compensation disabled/unset).
 *
 * @param block Pointer to a validated block.
 * @param current_centiamps Battery current, hundredths of an amp,
 *        positive while discharging. Caller is responsible for clamping
 *        this to a sane range first (this function does not).
 * @return Compensation delta, tenths of a volt.
 */
int16_t battery_lut_compensate_voltage(const battery_lut_block_t *block, int32_t current_centiamps);

/**
 * @brief Computes an IR-drop compensation delta for the current
 * flash-resident block.
 *
 * Only meaningful when battery_lut_is_valid() is true - callers must
 * check that first, same contract as battery_lut_get_percent().
 *
 * @param current_centiamps Battery current, hundredths of an amp,
 *        positive while discharging.
 * @return Compensation delta, tenths of a volt.
 */
int16_t battery_lut_get_voltage_compensation(int32_t current_centiamps);

#endif
