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
#include <math.h>
#include "tiny_math.h"
#include "function_generator.h"

static const float TWO_PI = 2.0f * M_PI;

/**
 * @brief Initializes a function generator
 *
 * This function initializes a function generator structure with the specified
 * waveform type, period, sample rate, and value range. The function also sets
 * flags for repeating and inverting the waveform, if requested.
 *
 * @param fg Pointer to the function generator structure to initialize
 * @param type The waveform type (sine, square, sawtooth)
 * @param period_ms The period of the waveform in milliseconds
 * @param sample_rate_ms The sample rate in milliseconds
 * @param min_value The minimum value of the waveform
 * @param max_value The maximum value of the waveform
 * @param flags Flags to configure waveform properties (e.g., repeat, invert)
 */
lcm_status_t function_generator_init(function_generator_t *fg, const waveform_t type,
                                     const float period_ms, const float sample_rate_ms,
                                     const float min_value, const float max_value,
                                     const uint8_t flags, const uint16_t sequence)
{
    // Check for null pointer
    if (fg == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    // Check for invalid parameters
    if (period_ms <= 0.0f || sample_rate_ms <= 0.0f || min_value > max_value)
    {
        return LCM_ERROR_INVALID_PARAM;
    }

    fg->type = type;
    fg->increment = TWO_PI / (period_ms / (float)sample_rate_ms);
    fg->repeat = (flags & FG_FLAG_REPEAT) != 0;
    fg->inverse = (flags & FG_FLAG_INVERT) != 0;
    fg->phase = 0.0f;
    fg->sequence = sequence;

    // Set waveform specific parameters
    function_generator_update_range(fg, min_value, max_value);
    if (type == FUNCTION_GENERATOR_SQUARE || type == FUNCTION_GENERATOR_SINE)
    {
        fg->phase = 3.0f * M_PI / 2.0f; // Start at 270 degrees
    }

    return LCM_SUCCESS;
}

lcm_status_t function_generator_update_range(function_generator_t *fg, const float min_value,
                                             const float max_value)
{
    if (fg != NULL)
    {
        fg->scale = (max_value - min_value) / 2.0f;
        fg->offset = (min_value + max_value) / 2.0f;
        return LCM_SUCCESS;
    }

    return LCM_ERROR_NULL_POINTER;
}

/**
 * @brief Increments the phase of a function generator and wraps it within [0,
 * 2*PI]
 *
 * This function increments the phase of a function generator based on the
 * frequency and sample rate. If the phase exceeds 2*PI, it wraps back to 0 if
 * the repeat flag is set, or returns false if it is not.
 *
 * @param fg Pointer to the function generator structure
 * @param repeat Whether to wrap the phase back to 0 if it exceeds 2*PI
 * @return true if successful, false if end of wave is reached
 */
lcm_status_t function_generator_increment_phase(function_generator_t *fg, const bool repeat)
{
    if (fg == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    // Increment the phase based on frequency and sample rate
    fg->phase += fg->increment;

    // Wrap phase to stay within [0, 2*PI]
    if (repeat)
    {
        if (fg->phase >= TWO_PI)
        {
            fg->phase -= TWO_PI;
        }
    }
    else if (fg->phase >= TWO_PI)
    {
        // If not repeating, return stop iteration
        fg->phase = TWO_PI;
    }

    return LCM_SUCCESS;
}

/**
 * @brief Calculates a waveform sample based on the current phase.
 *
 * This function calculates a sample value from the function generator
 * based on the provided phase and waveform type. The sample is
 * normalized between -1 and 1 and then scaled and offset to fit
 * the desired output range specified in the function generator
 * structure. If the inversion flag is set, the sample is inverted.
 *
 * @param phase The current phase of the waveform in radians.
 * @param fg Pointer to the function generator structure containing
 *           waveform type, scaling, and offset information.
 * @param sample Pointer to the float where the calculated sample
 *               will be stored.
 * @return LCM_SUCCESS if the calculation is successful,
 *         LCM_ERROR_NULL_POINTER if any pointer is NULL,
 *         and LCM_ERROR_INVALID_PARAM if the phase is out of bounds.
 */
lcm_status_t calculate_sample(const float phase, const function_generator_t *fg, float *sample)
{
    float normalized_sample = 0.0f;

    if (fg == NULL || sample == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    if (phase > TWO_PI || phase < 0.0f)
    {
        return LCM_ERROR_INVALID_PARAM;
    }

    switch (fg->type)
    {
    case FUNCTION_GENERATOR_SINE:
        normalized_sample = tiny_sinf(phase);
        break;
    case FUNCTION_GENERATOR_SQUARE:
        normalized_sample = (phase < M_PI) ? -1.0f : 1.0f;
        break;
    case FUNCTION_GENERATOR_SAWTOOTH:
        normalized_sample = (phase / M_PI) - 1.0f;
        break;
    case FUNCTION_GENERATOR_SEQUENCE: {
        uint16_t step = (uint16_t)(phase / (TWO_PI / 16));
        if (fg->sequence & (1 << (15 - step)))
        {
            normalized_sample = 1.0f;
        }
        else
        {
            normalized_sample = -1.0f;
        }
        break;
    }
    default:
        return LCM_ERROR_INVALID_PARAM;
    }

    // Apply inversion if requested
    if (fg->inverse)
    {
        normalized_sample = -normalized_sample;
    }

    // Map the normalized sample (-1 to 1) to the desired range [min_value,
    // max_value]
    *sample = fg->scale * normalized_sample + fg->offset;

    // Special case for end of non-repeating wave
    if (phase >= TWO_PI && fg->repeat == false)
    {
        return LCM_STOP_ITERATION;
    }

    return LCM_SUCCESS;
}

/**
 * @brief Retrieves the next sample from the function generator
 *
 * This function retrieves the next sample from the function generator and
 * stores it in the sample pointer. If the end of the wave is reached, the
 * function will return false.
 *
 * @param fg Pointer to the function generator structure
 * @param sample Pointer to the float to store the sample in
 * @return true if successful, false if end of wave is reached
 */
lcm_status_t function_generator_next_sample(function_generator_t *fg, float *sample)
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

/**
 * @brief Retrieves a sample from the function generator at an offset
 *
 * This function retrieves a sample from the function generator at an offset
 * and stores it in the sample pointer. If the offset is larger than the length
 * of the wave, the function will return false.
 *
 * @param fg Pointer to the function generator structure
 * @param sample Pointer to the float to store the sample in
 * @param offset Offset in the wave to peek
 * @return true if successful, false if offset is larger than the wave length
 */
lcm_status_t function_generator_peek_sample(const function_generator_t *fg, float *sample,
                                            const uint16_t offset)
{
    if (fg == NULL || sample == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    float future_phase = fg->phase + (fg->increment * offset);

    if (future_phase >= TWO_PI)
    {
        // If repeating, wrap phase to stay within [0, 2*PI],
        // otherwise set phase to 2*PI (max value)
        if (fg->repeat)
        {
            future_phase = tiny_fmodf(future_phase, TWO_PI);
        }
        else
        {
            future_phase = TWO_PI;
        }
    }
    return calculate_sample(future_phase, fg, sample);
}

/**
 * @brief Initializes the function generator with an initial sample.
 *
 * This function sets the initial sample for the function generator and performs
 * necessary checks and calculations to ensure the sample is valid and properly
 * normalized.
 *
 * @param fg Pointer to the function generator structure.
 * @param sample The initial sample value to be set.
 * @return lcm_status_t Returns LCM_SUCCESS if the initialization is successful,
 *         otherwise returns an error code:
 *         - LCM_ERROR_NULL_POINTER if the fg pointer is NULL.
 *         - LCM_ERROR_INVALID_PARAM if the scale is zero or the normalized
 * value is outside the range [-1, 1].
 */
lcm_status_t function_generator_initial_sample(function_generator_t *fg, const float sample)
{
    if (fg == NULL)
    {
        return LCM_ERROR_NULL_POINTER;
    }

    // Do not divide by zero
    if (fg->scale == 0.0f)
    {
        return LCM_ERROR_INVALID_PARAM;
    }

    float normalized_value = (sample - fg->offset) / fg->scale;

    // Apply inversion if requested
    if (fg->inverse)
    {
        normalized_value = -normalized_value;
    }

    // Check if normalized value is within [-1, 1]
    if (normalized_value > 1.0f || normalized_value < -1.0f)
    {
        return LCM_ERROR_INVALID_PARAM;
    }

    switch (fg->type)
    {
    case FUNCTION_GENERATOR_SAWTOOTH:
        fg->phase = (normalized_value + 1.0f) * M_PI;
        break;
    default:
        return LCM_ERROR_INVALID_PARAM;
    }

    return LCM_SUCCESS;
}