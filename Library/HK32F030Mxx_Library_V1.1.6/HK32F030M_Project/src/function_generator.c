/*
 * Copyright (c) 2024-2025, Mitchell White <mitchell.n.white@gmail.com>
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
#include <stddef.h>
#include "tiny_math.h"
#include "function_generator.h"

/**
 * @brief Quarter-wave sine lookup table.
 *
 * sine_lut[i] = round(sin(i * 90/64 degrees) * 32767), i.e. Q15 amplitude
 * for angles 0..89.99 degrees. Combined with quadrant mirroring/negation in
 * tiny_sin_bam(), this reconstructs a full sine cycle from one quarter,
 * trading a little angular resolution (~1.4 degrees/step) for a table 1/4
 * the size of storing all 256 steps - imperceptible for LED "breathing"
 * animations running over multi-second periods.
 */
static const int16_t sine_lut[64] = {
    0,     804,   1608,  2410,  3212,  4011,  4808,  5602,
    6393,  7179,  7962,  8739,  9512,  10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
};

/**
 * @brief Sine of a BAM angle (0-65535 = one full turn), as Q15.
 *
 * @param angle BAM angle.
 * @return Q15 value in [-32768, 32767] representing [-1.0, ~1.0].
 */
static int16_t tiny_sin_bam(uint16_t angle)
{
    uint8_t quadrant = (uint8_t)(angle >> 14);
    uint16_t quadrant_angle = angle & 0x3FFFU;
    int16_t magnitude;

    if ((quadrant & 1U) != 0U)
    {
        quadrant_angle = (uint16_t)(0x3FFFU - quadrant_angle);
    }
    magnitude = sine_lut[quadrant_angle >> 8];

    return (quadrant >= 2U) ? (int16_t)(-magnitude) : magnitude;
}

lcm_status_t function_generator_init(function_generator_t *fg, const waveform_t type,
                                     const uint32_t period_ms, const uint32_t sample_rate_ms,
                                     const fixed16_t min_value, const fixed16_t max_value,
                                     const uint8_t flags, const uint16_t sequence)
{
    // Check for null pointer
    if (fg == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    // Check for invalid parameters
    if (period_ms == 0U || sample_rate_ms == 0U || min_value > max_value)
    {
        return LCM_ERROR_INVALID_PARAM;
    }

    fg->type = type;
    // BAM increment per sample: one full turn (65536) divided across
    // (period_ms / sample_rate_ms) samples. Both operands are unsigned
    // durations, so this is a free unsigned divide (already linked in for
    // other reasons elsewhere in the firmware).
    fg->increment = (uint16_t)(((uint32_t)65536U * sample_rate_ms) / period_ms);
    fg->repeat = (flags & FG_FLAG_REPEAT) != 0;
    fg->inverse = (flags & FG_FLAG_INVERT) != 0;
    fg->phase = 0U;
    fg->sequence = sequence;

    // Set waveform specific parameters
    function_generator_update_range(fg, min_value, max_value);
    if (type == FUNCTION_GENERATOR_SQUARE || type == FUNCTION_GENERATOR_SINE)
    {
        fg->phase = 0xC000U; // Start at 270 degrees (270/360 * 65536)
    }

    return LCM_SUCCESS;
}

lcm_status_t function_generator_update_range(function_generator_t *fg, const fixed16_t min_value,
                                             const fixed16_t max_value)
{
    if (fg != NULL)
    {
        fg->scale = (max_value - min_value) / 2;
        fg->offset = (min_value + max_value) / 2;
        return LCM_SUCCESS;
    }

    return LCM_ERROR_NULL_POINTER;
}

/**
 * @brief Increments the phase of a function generator and wraps it within
 * [0, 65535] (one full BAM turn)
 *
 * @param fg Pointer to the function generator structure
 * @param repeat Whether to wrap the phase back to 0 if it exceeds one turn
 * @return true if successful, false if end of wave is reached
 */
lcm_status_t function_generator_increment_phase(function_generator_t *fg, const bool repeat)
{
    uint32_t next;

    if (fg == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    next = (uint32_t)fg->phase + fg->increment;

    if (repeat)
    {
        // Free wraparound: truncating a uint32_t sum of two uint16_t values
        // to uint16_t is exactly mod 65536.
        fg->phase = (uint16_t)next;
    }
    else if (next >= 0x10000U)
    {
        // End of a non-repeating wave: saturate at the sentinel instead of
        // wrapping, checked by calculate_sample() below.
        fg->phase = 0xFFFFU;
    }
    else
    {
        fg->phase = (uint16_t)next;
    }

    return LCM_SUCCESS;
}

/**
 * @brief Calculates a waveform sample based on the given phase.
 *
 * @param phase BAM phase, [0, 65535].
 * @param fg Pointer to the function generator structure.
 * @param sample Pointer to the fixed16_t where the calculated sample will
 *               be stored.
 * @return LCM_SUCCESS if the calculation is successful,
 *         LCM_ERROR_NULL_POINTER if any pointer is NULL,
 *         LCM_STOP_ITERATION if this was the last sample of a
 *         non-repeating wave.
 */
lcm_status_t calculate_sample(const uint16_t phase, const function_generator_t *fg, fixed16_t *sample)
{
    int16_t normalized_sample = 0;
    int32_t fixed16_wide;

    if (fg == NULL || sample == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    switch (fg->type)
    {
    case FUNCTION_GENERATOR_SINE:
        normalized_sample = tiny_sin_bam(phase);
        break;
    case FUNCTION_GENERATOR_SQUARE:
        normalized_sample = (phase < 0x8000U) ? INT16_MIN : INT16_MAX;
        break;
    case FUNCTION_GENERATOR_SAWTOOTH:
        normalized_sample = (int16_t)(phase - 0x8000U);
        break;
    case FUNCTION_GENERATOR_SEQUENCE: {
        uint16_t step = (uint16_t)(phase >> 12); // phase / (65536 / 16)
        if (fg->sequence & (1U << (15U - step)))
        {
            normalized_sample = INT16_MAX;
        }
        else
        {
            normalized_sample = INT16_MIN;
        }
        break;
    }
    default:
        return LCM_ERROR_INVALID_PARAM;
    }

    // Widen Q15 to Q16.16 (shift left by 1) before inverting/multiplying -
    // normalized_sample's minimum (INT16_MIN) has no positive int16_t
    // counterpart, so negating it as int16_t would overflow. The widened
    // range has ample headroom (-65536 negates cleanly to 65536).
    fixed16_wide = (int32_t)normalized_sample << 1;

    // Apply inversion if requested
    if (fg->inverse)
    {
        fixed16_wide = -fixed16_wide;
    }

    // Map the normalized sample ([-1,1]) to [min_value, max_value]
    *sample = fixed_mul16(fg->scale, fixed16_wide) + fg->offset;

    // Special case for end of non-repeating wave
    if (phase == 0xFFFFU && fg->repeat == false)
    {
        return LCM_STOP_ITERATION;
    }

    return LCM_SUCCESS;
}

lcm_status_t function_generator_next_sample(function_generator_t *fg, fixed16_t *sample)
{
    lcm_status_t result = LCM_SUCCESS;

    if (fg == NULL || sample == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    result = calculate_sample(fg->phase, fg, sample);
    if (result == LCM_SUCCESS)
    {
        return function_generator_increment_phase(fg, fg->repeat);
    }

    return result;
}

lcm_status_t function_generator_peek_sample(const function_generator_t *fg, fixed16_t *sample,
                                            const uint16_t offset)
{
    uint32_t future_phase;

    if (fg == NULL || sample == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    future_phase = (uint32_t)fg->phase + ((uint32_t)fg->increment * offset);

    if (fg->repeat)
    {
        // AND-mask is an exact mod-65536, regardless of how many turns
        // future_phase has crossed.
        future_phase &= 0xFFFFU;
    }
    else if (future_phase >= 0x10000U)
    {
        future_phase = 0xFFFFU;
    }

    return calculate_sample((uint16_t)future_phase, fg, sample);
}

lcm_status_t function_generator_initial_sample(function_generator_t *fg, const fixed16_t sample)
{
    int32_t normalized_value;

    if (fg == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    // Do not divide by zero
    if (fg->scale == 0)
    {
        return LCM_ERROR_INVALID_PARAM;
    }

    // normalized = (sample - offset) / scale, as a Q16.16 ratio in [-1, 1]
    normalized_value = (int32_t)(((int64_t)(sample - fg->offset) << 16) / fg->scale);

    // Apply inversion if requested
    if (fg->inverse)
    {
        normalized_value = -normalized_value;
    }

    // Check if normalized value is within [-1, 1]
    if (normalized_value > FIXED16(1.0) || normalized_value < FIXED16(-1.0))
    {
        return LCM_ERROR_INVALID_PARAM;
    }

    switch (fg->type)
    {
    case FUNCTION_GENERATOR_SAWTOOTH:
        // Inverse of the forward sawtooth mapping (phase - 0x8000 -> Q15
        // normalized, widened by <<1 to Q16.16 in calculate_sample()).
        fg->phase = (uint16_t)(0x8000 + (normalized_value >> 1));
        break;
    default:
        return LCM_ERROR_INVALID_PARAM;
    }

    return LCM_SUCCESS;
}
