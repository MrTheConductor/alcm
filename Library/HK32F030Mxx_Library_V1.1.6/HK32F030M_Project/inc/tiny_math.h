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
#ifndef TINY_MATH_H
#define TINY_MATH_H

#include <stdint.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CLAMP(value, min, max) ((value < min) ? min : ((value > max) ? max : value))

/**
 * @brief Generates a pseudo-random number within a specified range.
 *
 * This function uses a Linear Congruential Generator (LCG) algorithm to
 * produce a pseudo-random number between the specified minimum and maximum
 * values (inclusive). The state of the generator is updated with each call.
 *
 * @param state Pointer to the current state of the pseudo-random number generator.
 *              This value should be initialized before the first call and will be
 *              updated with each subsequent call.
 * @param min The minimum value of the generated pseudo-random number (inclusive).
 * @param max The maximum value of the generated pseudo-random number (inclusive).
 * @return A pseudo-random number between min and max (inclusive).
 *         Returns 0 if the state pointer is NULL.
 */
static inline uint8_t prng(uint8_t *state, uint8_t min, uint8_t max)
{
    if (state == NULL)
    {
        return 0;
    }

    // LCG: Xn+1 = (A * Xn +C ) mod M
    *state = (*state * 197 + 23);
    return min + (*state % (max - min + 1));
}

static inline uint8_t qadd8(uint8_t a, uint8_t b)
{
    uint8_t sum = a + b;
    if (sum < a)
    {
        return 255;
    }
    return sum;
}

static inline uint8_t qsub8(uint8_t a, uint8_t b)
{
    if (a < b)
    {
        return 0;
    }
    return a - b;
}

/**
 * @brief Scales i by scale/255 (0-255 = 0.0-1.0).
 *
 * Uses the standard scale+1 rounding trick (as in FastLED) rather than a
 * plain (i*scale)>>8: with a plain floor, scale8(255,255) is 254, not 255,
 * so chaining several scale8() calls (e.g. multiple brightness factors)
 * never quite reaches full scale even when every factor is "1.0". The +1
 * makes scale=255 an exact identity (scale8(i,255)==i for all i).
 */
static inline uint8_t scale8(uint8_t i, uint8_t scale)
{
    return ((uint16_t)i * ((uint16_t)scale + 1U)) >> 8;
}

/**
 * @brief Multiplies two Q16.16 signed fixed-point values.
 *
 * Cortex-M0 has no hardware widening multiply, so the 64-bit intermediate
 * here is done in software (once, shared across every caller) - still far
 * cheaper than the soft-float library it replaces.
 *
 * @param a First Q16.16 operand.
 * @param b Second Q16.16 operand.
 * @return The Q16.16 product of a and b.
 */
static inline int32_t fixed_mul16(int32_t a, int32_t b)
{
    return (int32_t)(((int64_t)a * (int64_t)b) >> 16);
}

#endif
