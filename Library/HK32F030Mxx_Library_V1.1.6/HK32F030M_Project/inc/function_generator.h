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
 */
typedef struct
{
    float scale;       // Scale factor for the wave
    float offset;      // Offset for the wave
    uint16_t sequence; // 16-bit mask for sequence
    float increment;   // Increment for the wave
    float phase;       // Phase offset in radians
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
                                     const float period_ms, const float sample_rate_ms,
                                     const float min_value, const float max_value,
                                     const uint8_t flags, const uint16_t sequence);

/**
 * @brief Updates the range of the function generator.
 *
 * This function sets the minimum and maximum values for the function generator.
 *
 * @param fg Pointer to the function generator structure.
 * @param min_value The minimum value to set for the function generator.
 * @param max_value The maximum value to set for the function generator.
 * @return lcm_status_t Status of the operation.
 */
lcm_status_t function_generator_update_range(function_generator_t *fg, const float min_value,
                                             const float max_value);

/**
 * @brief Generate the next sample of the waveform
 *
 * @param fg Pointer to the function generator structure
 * @param sample Pointer to store the generated sample
 * @return lcm_status_t Status of the sample generation
 */
lcm_status_t function_generator_next_sample(function_generator_t *fg, float *sample);

/**
 * @brief Peek at a future sample of the waveform without advancing the
 * generator
 *
 * @param fg Pointer to the function generator structure
 * @param sample Pointer to store the peeked sample
 * @param offset Offset in samples from the current position
 * @return lcm_status_t Status of the peek operation
 */
lcm_status_t function_generator_peek_sample(const function_generator_t *fg, float *sample,
                                            const uint16_t offset);

/**
 * @brief Get the initial sample of the waveform
 *
 * @param fg Pointer to the function generator structure
 * @param sample Initial sample value
 * @return lcm_status_t Status of the operation
 */
lcm_status_t function_generator_initial_sample(function_generator_t *fg, const float sample);

#endif /* FUNCTION_GENERATOR_H */