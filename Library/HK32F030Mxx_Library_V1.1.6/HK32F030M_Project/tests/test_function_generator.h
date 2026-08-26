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
#ifndef TEST_FUNCTION_GENERATOR_H
#define TEST_FUNCTION_GENERATOR_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include "function_generator_internal.h"
#include "tiny_math.h"

void test_function_generator_init_null_ptr(void **state)
{
    (void)state;

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_init(NULL, FUNCTION_GENERATOR_SINE,
                                                                     0, 0, 0, 0, FG_FLAG_NONE, 0));
}

void test_function_generator_init_invalid_period(void **state)
{
    (void)state;
    function_generator_t fg;

    assert_int_equal(LCM_ERROR_INVALID_PARAM,
                     function_generator_init(&fg, FUNCTION_GENERATOR_SINE, 0, 100, FIXED16(0.0),
                                             FIXED16(1.0), FG_FLAG_NONE, 0));
}

void test_function_generator_init_invalid_sample_rate(void **state)
{
    (void)state;
    function_generator_t fg;

    assert_int_equal(LCM_ERROR_INVALID_PARAM,
                     function_generator_init(&fg, FUNCTION_GENERATOR_SINE, 100, 0, FIXED16(0.0),
                                             FIXED16(1.0), FG_FLAG_NONE, 0));
}

void test_function_generator_init(void **state)
{
    (void)state;
    function_generator_t fg;

    assert_int_equal(LCM_SUCCESS,
                     function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000, 10,
                                             FIXED16(0.0), FIXED16(1.0), FG_FLAG_REPEAT, 0));

    assert_int_equal(FUNCTION_GENERATOR_SAWTOOTH, fg.type);
    assert_int_equal(FIXED16(0.5), fg.scale);
    assert_int_equal(FIXED16(0.5), fg.offset);
    // BAM increment: (65536 * sample_rate_ms) / period_ms, truncating integer divide
    assert_int_equal(655, fg.increment);
    assert_int_equal(0, fg.phase); // SAWTOOTH doesn't get the 270-degree start
    assert_int_equal(true, fg.repeat);
    assert_int_equal(false, fg.inverse);
}

void test_function_generator_peek_null(void **state)
{
    (void)state;
    function_generator_t fg;
    fixed16_t sample;

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_peek_sample(NULL, &sample, 0));

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_peek_sample(&fg, NULL, 0));
}

void test_function_generator_peek(void **state)
{
    (void)state;
    function_generator_t fg;
    fixed16_t sample;

    // period=800, sample_rate=100 -> increment=8192 exactly (65536/8), so 8
    // steps land exactly at the end of the wave with no truncation error -
    // deliberately chosen so the expected values below are exact, not
    // approximate (unlike the old float period=1000/rate=100 combo, whose
    // BAM increment truncates to 6553, one integer BAM unit short of eight
    // even steps - see test_function_generator_next_sample_non_repeat for
    // where that kind of combo's exact boundary now lands instead).
    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 800,
                                                          100, FIXED16(0.0), FIXED16(1.0),
                                                          FG_FLAG_NONE, 0));

    // This should be a linearly increasing value between 0 and 1
    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 0));
    assert_int_equal(0, sample);

    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 1));
    assert_int_equal(8192, sample); // 0.125

    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 7));
    assert_int_equal(57344, sample); // 0.875

    assert_int_equal(LCM_STOP_ITERATION, function_generator_peek_sample(&fg, &sample, 8));
    assert_int_equal(65535, sample); // ~1.0 (Q15's asymmetric range tops out one below FIXED16(1.0))
}

void test_function_generator_next_sample_null(void **state)
{
    (void)state;
    function_generator_t fg;
    fixed16_t sample;

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_next_sample(NULL, &sample));

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_next_sample(&fg, NULL));
}

void test_function_generator_next_sample_repeat(void **state)
{
    (void)state;
    function_generator_t fg;
    fixed16_t sample = 0;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SINE, 1000, 99,
                                                          FIXED16(0.0), FIXED16(1.0), FG_FLAG_REPEAT,
                                                          0));

    for (uint8_t i = 0; i < 20; i++)
    {
        assert_int_equal(LCM_SUCCESS, function_generator_next_sample(&fg, &sample));
    }
}

void test_function_generator_next_sample_non_repeat(void **state)
{
    (void)state;
    function_generator_t fg;
    fixed16_t sample;
    // period=800, sample_rate=100 -> exact 8-step ramp, see test_function_generator_peek
    const fixed16_t expected[8] = {0, 8192, 16384, 24576, 32768, 40960, 49152, 57344};

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 800,
                                                          100, FIXED16(0.0), FIXED16(1.0),
                                                          FG_FLAG_NONE, 0));

    for (uint8_t i = 0; i < 8; i++)
    {
        assert_int_equal(LCM_SUCCESS, function_generator_next_sample(&fg, &sample));
        assert_int_equal(expected[i], sample);
    }

    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_int_equal(65535, sample);

    // Should get the same result if we try this again
    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_int_equal(65535, sample);
}

void test_function_generator_next_sample_non_repeat_inverted(void **state)
{
    (void)state;
    function_generator_t fg;
    fixed16_t sample;
    const fixed16_t expected[8] = {65536, 57344, 49152, 40960, 32768, 24576, 16384, 8192};

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 800,
                                                          100, FIXED16(0.0), FIXED16(1.0),
                                                          FG_FLAG_INVERT, 0));

    for (uint8_t i = 0; i < 8; i++)
    {
        assert_int_equal(LCM_SUCCESS, function_generator_next_sample(&fg, &sample));
        assert_int_equal(expected[i], sample);
    }

    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_int_equal(1, sample);

    // Should get the same result if we try this again
    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_int_equal(1, sample);
}

void test_function_generator_initial_sample_invalid_params(void **state)
{
    (void)state;
    function_generator_t fg;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          100, FIXED16(0.0), FIXED16(1.0),
                                                          FG_FLAG_NONE, 0));

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_initial_sample(NULL, FIXED16(0.0)));

    assert_int_equal(LCM_ERROR_INVALID_PARAM,
                     function_generator_initial_sample(&fg, FIXED16(1.1)));

    assert_int_equal(LCM_ERROR_INVALID_PARAM,
                     function_generator_initial_sample(&fg, FIXED16(-0.1)));

    fg.scale = 0;
    assert_int_equal(LCM_ERROR_INVALID_PARAM, function_generator_initial_sample(&fg, FIXED16(0.0)));
}

void test_function_generator_initial_sample(void **state)
{
    (void)state;
    function_generator_t fg;
    fixed16_t sample;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 800,
                                                          100, FIXED16(0.0), FIXED16(1.0),
                                                          FG_FLAG_NONE, 0));

    assert_int_equal(LCM_SUCCESS, function_generator_initial_sample(&fg, FIXED16(0.5)));

    assert_int_equal(0x8000, fg.phase); // Halfway through the BAM turn
    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 1));
    assert_int_equal(40960, sample); // 0.625

    // Make sure inverted works too
    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 800,
                                                          100, FIXED16(0.0), FIXED16(1.0),
                                                          FG_FLAG_INVERT, 0));

    assert_int_equal(LCM_SUCCESS, function_generator_initial_sample(&fg, FIXED16(0.0)));

    assert_int_equal(0, fg.phase);
}

void test_function_generator_increment_phase_null(void **state)
{
    (void)state;

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_increment_phase(NULL, true));
}

const struct CMUnitTest function_generator_tests[] = {
    cmocka_unit_test(test_function_generator_init_null_ptr),
    cmocka_unit_test(test_function_generator_init_invalid_period),
    cmocka_unit_test(test_function_generator_init_invalid_sample_rate),
    cmocka_unit_test(test_function_generator_init),
    cmocka_unit_test(test_function_generator_peek_null),
    cmocka_unit_test(test_function_generator_peek),
    cmocka_unit_test(test_function_generator_next_sample_null),
    cmocka_unit_test(test_function_generator_next_sample_repeat),
    cmocka_unit_test(test_function_generator_next_sample_non_repeat),
    cmocka_unit_test(test_function_generator_next_sample_non_repeat_inverted),
    cmocka_unit_test(test_function_generator_initial_sample_invalid_params),
    cmocka_unit_test(test_function_generator_initial_sample),
    cmocka_unit_test(test_function_generator_increment_phase_null),
};
#endif
