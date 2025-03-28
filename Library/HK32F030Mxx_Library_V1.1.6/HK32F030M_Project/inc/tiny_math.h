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

#include <math.h>
#include <stdint.h>

#define M_PI 3.141592653589793f

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CLAMP(value, min, max) ((value < min) ? min : ((value > max) ? max : value))

/**
 * @brief Calculate the remainder of division of two floating-point numbers,
 * x/y.
 *
 * Mimics the behaviour of the C standard library function fmodf(), but avoids
 * linking against the full math library. This implementation is not as
 * efficient, but is suitable for very small programs where code size is
 * critical.
 *
 * @param x The dividend
 * @param y The divisor
 * @return The remainder of division of x/y
 */
static inline float tiny_fmodf(float x, float y)
{
    // Don't divide by zero
    if (y == 0.0f)
    {
        return 0.0f;
    }

    // Integer truncation mimics floor
    return x - y * (int)(x / y);
}

/**
 * @brief Approximate the value of e^x (base e exponential) as a float.
 *
 * This is a very rough approximation of the exponential function, taken from
 * Schraudolph's paper. It is not meant to be used for high-precision
 * calculations, but is suitable for small programs where code size is critical.
 *
 * @param x The value to calculate the exponential of.
 * @return The approximate value of e^x.
 */
static inline float tiny_expf(float x)
{
    // Magic constant from Schraudolph's paper (approximates 2^x)
    union {
        uint32_t i;
        float f;
    } u;
    u.i = (uint32_t)(12102203 * x + 1065353216); // 2^x approximation
    uint32_t m = (u.i >> 7) & 0xFFFF;            // 16-bit mantissa
    // Correction
    u.i += ((((((((1277 * m) >> 14) + 14825) * m) >> 14) - 79749) * m) >> 11) - 626;
    return u.f;
}

/**
 * @brief Approximate the sine of a given angle in radians.
 *
 * This function computes an approximation of the sine of the input angle using
 * a 5th-degree polynomial. The input angle is first normalized to the range
 * [-π, π] to ensure better approximation accuracy. A correction factor is
 * applied to improve the accuracy of the approximation.
 *
 * @param x The angle in radians for which to calculate the sine.
 * @return The approximate sine of the input angle.
 */
static inline float tiny_sinf(float x)
{
    // Bring x into the range [-π, π]
    while (x > M_PI)
        x -= 2.0f * M_PI;
    while (x < -M_PI)
        x += 2.0f * M_PI;

    // Approximate sin(x) using a 5th-degree polynomial
    const float B = 4.0f / M_PI;
    const float C = -4.0f / (M_PI * M_PI);
    const float P = 0.225f; // Correction factor for better accuracy

    float y = B * x + C * x * fabsf(x);
    return P * (y * fabsf(y) - y) + y; // Correction to improve accuracy
}

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

static inline uint8_t scale8(uint8_t i, uint8_t scale)
{
    return ((uint16_t)i * (uint16_t)scale) >> 8;
}

#endif