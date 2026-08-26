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
#ifndef FUNCTION_GENERATOR_H
#define FUNCTION_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "lcm_types.h"

#define FG_FLAG_NONE 0x00   // No special options
#define FG_FLAG_INVERT 0x01 // Invert the waveform
#define FG_FLAG_REPEAT 0x02 // Repeat the waveform

/**
 * @brief Q16.16 signed fixed-point value.
 *
 * function_generator_t is shared verbatim across several different value
 * domains (brightness fractions, hue in degrees, scan position), so its
 * scale/offset/sample values use one generic fixed-point type; callers
 * narrow the result to their own domain-native type immediately after
 * calling one of the sample functions below.
 */
typedef int32_t fixed16_t;

/**
 * @brief Converts a compile-time literal to fixed16_t.
 *
 * Only safe to use with literal/constant-expression arguments - the whole
 * point is that the multiply is folded away at compile time, so this never
 * generates a runtime float instruction. Do not call this with a variable.
 */
#define FIXED16(x) ((fixed16_t)((x) * 65536.0))

/**
 * @brief Narrows a Q16.16 fraction in [0.0, 1.0] to a scale8()-ready uint8_t.
 *
 * Clamps defensively so a slightly-out-of-range fixed16_t (rounding, or an
 * animation whose range brushes the boundary) can't wrap instead of
 * saturating.
 */
static inline uint8_t fixed16_to_frac8(fixed16_t x)
{
    if (x <= 0)
    {
        return 0U;
    }
    if (x >= FIXED16(1.0))
    {
        return 255U;
    }
    return (uint8_t)((x * 255) >> 16);
}

/**
 * @brief Widens a scale8()-domain uint8_t fraction [0,255] back to Q16.16.
 *
 * The divisor is a compile-time literal (255), so this compiles to an
 * inline multiply-by-reciprocal - no runtime division library call.
 */
static inline fixed16_t frac8_to_fixed16(uint8_t frac)
{
    return (fixed16_t)(((uint32_t)frac * 65536U) / 255U);
}

/**
 * @brief Narrows a Q16.16 "whole degrees" sample to a uint16_t, clamped to
 * [0, 359].
 */
static inline uint16_t fixed16_to_degrees(fixed16_t x)
{
    int32_t degrees = x >> 16;
    if (degrees < 0)
    {
        return 0U;
    }
    if (degrees > 359)
    {
        return 359U;
    }
    return (uint16_t)degrees;
}

/**
 * @brief Widens whole degrees to a Q16.16 fixed16_t (exact, no division).
 */
static inline fixed16_t degrees_to_fixed16(uint16_t degrees)
{
    return (fixed16_t)degrees << 16;
}

/**
 * @brief Function generator types
 */
typedef enum
{
    FUNCTION_GENERATOR_SINE,     // Sine wave
    FUNCTION_GENERATOR_SQUARE,   // Square wave
    FUNCTION_GENERATOR_SAWTOOTH, // Sawtooth wave
    FUNCTION_GENERATOR_SEQUENCE  // Arbitrary sequence
} waveform_t;

/**
 * @brief Structure to represent a function generator
 *
 * phase/increment use BAM (Binary Angle Measurement): a uint16_t where
 * 0-65535 represents one full turn (0-2*PI). This is private to the engine
 * and never crosses the public API. Wraparound for repeating waveforms is
 * free (unsigned overflow); non-repeating waveforms saturate at 0xFFFF
 * instead of wrapping, which is the "end of wave" sentinel checked by
 * function_generator_increment_phase().
 */
typedef struct
{
    fixed16_t scale;   // Scale factor for the wave
    fixed16_t offset;  // Offset for the wave
    uint16_t sequence; // 16-bit mask for sequence
    uint16_t increment; // BAM phase increment per sample
    uint16_t phase;     // BAM phase, [0, 65535]
    bool repeat;       // Whether wave repeats
    bool inverse;      // Whether to reverse the direction of the wave
    waveform_t type;   // Function generator type
} function_generator_t;

/**
 * @brief Initialize the function generator
 *
 * @param fg Pointer to the function generator structure
 * @param type Type of waveform to generate
 * @param period_ms Period of the waveform in milliseconds
 * @param sample_rate_ms Sample rate in milliseconds
 * @param min_value Minimum value of the waveform
 * @param max_value Maximum value of the waveform
 * @param flags Flags to control waveform generation
 * @param sequence Sequence of values for FUNCTION_GENERATOR_SEQUENCE
 * @return lcm_status_t Status of the initialization
 */
lcm_status_t function_generator_init(function_generator_t *fg, const waveform_t type,
                                     const uint32_t period_ms, const uint32_t sample_rate_ms,
                                     const fixed16_t min_value, const fixed16_t max_value,
                                     const uint8_t flags, const uint16_t sequence);

/**
 * @brief Updates the range of the function generator.
 *
 * @param fg Pointer to the function generator structure.
 * @param min_value The minimum value to set for the function generator.
 * @param max_value The maximum value to set for the function generator.
 * @return lcm_status_t Status of the operation.
 */
lcm_status_t function_generator_update_range(function_generator_t *fg, const fixed16_t min_value,
                                             const fixed16_t max_value);

/**
 * @brief Generate the next sample of the waveform
 *
 * @param fg Pointer to the function generator structure
 * @param sample Pointer to store the generated sample
 * @return lcm_status_t Status of the sample generation
 */
lcm_status_t function_generator_next_sample(function_generator_t *fg, fixed16_t *sample);

/**
 * @brief Peek at a future sample of the waveform without advancing the
 * generator
 *
 * @param fg Pointer to the function generator structure
 * @param sample Pointer to store the peeked sample
 * @param offset Offset in samples from the current position
 * @return lcm_status_t Status of the peek operation
 */
lcm_status_t function_generator_peek_sample(const function_generator_t *fg, fixed16_t *sample,
                                            const uint16_t offset);

/**
 * @brief Get the initial sample of the waveform
 *
 * @param fg Pointer to the function generator structure
 * @param sample Initial sample value
 * @return lcm_status_t Status of the operation
 */
lcm_status_t function_generator_initial_sample(function_generator_t *fg, const fixed16_t sample);

#endif /* FUNCTION_GENERATOR_H */
