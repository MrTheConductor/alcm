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
 * @file battery_lut.c
 * @brief Implementation of the patchable battery LUT module
 *
 * Only the validity flag is cached in RAM - the block itself is read
 * straight out of flash on every lookup via battery_lut_hw_get_block(),
 * since a RAM copy would cost 60 bytes this project can't spare.
 */
#include <stddef.h>

#include "battery_lut.h"
#include "battery_lut_hw.h"
#include "crc16_ccitt.h"

static bool_t battery_lut_valid = false;

/**
 * @brief Validates a battery LUT block.
 */
bool_t battery_lut_validate_block(const battery_lut_block_t *block)
{
    bool_t valid = true;

    if (block == NULL)
    {
        valid = false;
    }
    else if (block->payload.magic != BATTERY_LUT_MAGIC)
    {
        valid = false;
    }
    else if (block->payload.schema_version != BATTERY_LUT_SCHEMA_VERSION)
    {
        valid = false;
    }
    else if (block->payload.cell_count == 0U)
    {
        valid = false;
    }
    else if (block->payload.breakpoint_count < 2U ||
             block->payload.breakpoint_count > BATTERY_LUT_MAX_BREAKPOINTS)
    {
        valid = false;
    }
    else if (block->crc16 !=
             crc16_ccitt((const uint8_t *)&block->payload, sizeof(block->payload)))
    {
        valid = false;
    }
    else
    {
        // Breakpoints must be strictly descending in voltage and
        // non-increasing in percent - battery_lut_interpolate()'s math is
        // unsigned and relies on this to avoid an underflow-wrapped result.
        for (uint8_t i = 1U; i < block->payload.breakpoint_count; i++)
        {
            const battery_lut_breakpoint_t *prev = &block->payload.breakpoints[i - 1U];
            const battery_lut_breakpoint_t *curr = &block->payload.breakpoints[i];

            if (curr->voltage_tenths >= prev->voltage_tenths ||
                curr->percent_tenths > prev->percent_tenths)
            {
                valid = false;
                break;
            }
        }
    }

    return valid;
}

/**
 * @brief Interpolates a state-of-charge percentage from a whole-pack voltage.
 */
int16_t battery_lut_interpolate(const battery_lut_block_t *block, uint16_t voltage_tenths)
{
    const battery_lut_breakpoint_t *bp = block->payload.breakpoints;
    uint8_t n = block->payload.breakpoint_count;
    int16_t percent = (int16_t)bp[n - 1U].percent_tenths;

    if (voltage_tenths >= bp[0].voltage_tenths)
    {
        percent = (int16_t)bp[0].percent_tenths;
    }
    else if (voltage_tenths <= bp[n - 1U].voltage_tenths)
    {
        percent = (int16_t)bp[n - 1U].percent_tenths;
    }
    else
    {
        for (uint8_t i = 1U; i < n; i++)
        {
            if (voltage_tenths >= bp[i].voltage_tenths)
            {
                uint32_t span_v = (uint32_t)bp[i - 1U].voltage_tenths - bp[i].voltage_tenths;
                uint32_t span_pct = (uint32_t)bp[i - 1U].percent_tenths - bp[i].percent_tenths;
                uint32_t delta_v = (uint32_t)voltage_tenths - bp[i].voltage_tenths;

                percent = (int16_t)(bp[i].percent_tenths + (delta_v * span_pct) / span_v);
                break;
            }
        }
    }

    return percent;
}

/**
 * @brief Initializes the battery LUT module.
 */
lcm_status_t battery_lut_init(void)
{
    battery_lut_valid = battery_lut_validate_block(battery_lut_hw_get_block());

    return LCM_SUCCESS;
}

/**
 * @brief Returns whether a valid, patched LUT block is present.
 */
bool_t battery_lut_is_valid(void)
{
    return battery_lut_valid;
}

/**
 * @brief Looks up the state of charge for a whole-pack voltage.
 */
int16_t battery_lut_get_percent(uint16_t voltage_tenths)
{
    return battery_lut_interpolate(battery_lut_hw_get_block(), voltage_tenths);
}

/**
 * @brief Computes an IR-drop (load-sag) compensation delta.
 */
int16_t battery_lut_compensate_voltage(const battery_lut_block_t *block, int32_t current_centiamps)
{
    uint32_t magnitude;
    int32_t compensation_tenths;

    // Sign/magnitude split so this only ever needs unsigned division -
    // signed division isn't otherwise linked into this firmware, and the
    // flash budget is tight enough that it's worth avoiding on purpose
    // rather than picking it up incidentally.
    magnitude = (uint32_t)((current_centiamps < 0) ? -current_centiamps : current_centiamps);

    // V_drop(V) = I(A) * R(ohm) = (centiamps/100) * (milliohms/1000).
    // In tenths of a volt: *10, so overall divide by 100*1000/10 = 10000.
    compensation_tenths = (int32_t)((magnitude * (uint32_t)block->payload.r_int_milliohms) / 10000U);

    if (current_centiamps < 0)
    {
        compensation_tenths = -compensation_tenths;
    }

    return (int16_t)compensation_tenths;
}

/**
 * @brief Computes an IR-drop compensation delta for the current
 * flash-resident block.
 */
int16_t battery_lut_get_voltage_compensation(int32_t current_centiamps)
{
    return battery_lut_compensate_voltage(battery_lut_hw_get_block(), current_centiamps);
}
