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

    assert_int_equal(
        LCM_ERROR_INVALID_PARAM,
        function_generator_init(&fg, FUNCTION_GENERATOR_SINE, 0, 100, 0.0f, 1.0f, FG_FLAG_NONE, 0));
}

void test_function_generator_init_invalid_sample_rate(void **state)
{
    (void)state;
    function_generator_t fg;

    assert_int_equal(
        LCM_ERROR_INVALID_PARAM,
        function_generator_init(&fg, FUNCTION_GENERATOR_SINE, 100, 0, 0.0f, 1.0f, FG_FLAG_NONE, 0));
}

void test_function_generator_init(void **state)
{
    (void)state;
    function_generator_t fg;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          10, 0.0f, 1.0f, FG_FLAG_REPEAT, 0));

    assert_int_equal(FUNCTION_GENERATOR_SAWTOOTH, fg.type);
    assert_float_equal(0.5f, fg.scale, 0.0f);
    assert_float_equal(0.5f, fg.offset, 0.0f);
    assert_float_equal(0.0628f, fg.increment, 0.0001f);
    assert_float_equal(0.0f, fg.phase, 0.0f);
    assert_int_equal(true, fg.repeat);
    assert_int_equal(false, fg.inverse);
}

void test_function_generator_peek_null(void **state)
{
    (void)state;
    function_generator_t fg;
    float sample;

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_peek_sample(NULL, &sample, 0));

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_peek_sample(&fg, NULL, 0));
}

void test_function_generator_peek(void **state)
{
    (void)state;
    function_generator_t fg;
    float sample;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          100, 0.0f, 1.0f, FG_FLAG_NONE, 0));

    // This should be a linearly increasing value between 0 and 1
    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 0));

    assert_float_equal(0.0f, sample, 0.0f);

    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 1));

    assert_float_equal(0.1f, sample, 0.0f);

    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 9));

    assert_float_equal(0.9f, sample, 0.0f);

    assert_int_equal(LCM_STOP_ITERATION, function_generator_peek_sample(&fg, &sample, 10));

    assert_float_equal(1.0f, sample, 0.0f);
}

void test_function_generator_next_sample_null(void **state)
{
    (void)state;
    function_generator_t fg;
    float sample;

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_next_sample(NULL, &sample));

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_next_sample(&fg, NULL));
}

void test_function_generator_next_sample_repeat(void **state)
{
    (void)state;
    function_generator_t fg;
    float sample = 0.0f;
    float previous_sample = 0.0f;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SINE, 1000, 99,
                                                          0.0f, 1.0f, FG_FLAG_REPEAT, 0));

    for (uint8_t i = 0; i < 20; i++)
    {
        assert_int_equal(LCM_SUCCESS, function_generator_next_sample(&fg, &sample));

        // assert_float_not_equal(previous_sample, sample, 0.0f);
        previous_sample = sample;
    }
}

void test_function_generator_next_sample_non_repeat(void **state)
{
    (void)state;
    function_generator_t fg;
    float sample;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          100, 0.0f, 1.0f, FG_FLAG_NONE, 0));

    for (uint8_t i = 0; i < 10; i++)
    {
        assert_int_equal(LCM_SUCCESS, function_generator_next_sample(&fg, &sample));

        assert_float_equal(0.1f * i, sample, 0.0f);
    }

    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_float_equal(1.0f, sample, 0.0f);

    // Should get the same result if we try this again
    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_float_equal(1.0f, sample, 0.0f);
}

void test_function_generator_next_sample_non_repeat_inverted(void **state)
{
    (void)state;
    function_generator_t fg;
    float sample;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          100, 0.0f, 1.0f, FG_FLAG_INVERT, 0));

    for (uint8_t i = 0; i < 10; i++)
    {
        assert_int_equal(LCM_SUCCESS, function_generator_next_sample(&fg, &sample));

        assert_float_equal(1.0f - (0.1f * i), sample, 0.0f);
    }

    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_float_equal(0.0f, sample, 0.0f);

    // Should get the same result if we try this again
    assert_int_equal(LCM_STOP_ITERATION, function_generator_next_sample(&fg, &sample));
    assert_float_equal(0.0f, sample, 0.0f);
}

void test_function_generator_initial_sample_invalid_params(void **state)
{
    (void)state;
    function_generator_t fg;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          100, 0.0f, 1.0f, FG_FLAG_NONE, 0));

    assert_int_equal(LCM_ERROR_NULL_POINTER, function_generator_initial_sample(NULL, 0.0f));

    assert_int_equal(LCM_ERROR_INVALID_PARAM, function_generator_initial_sample(&fg, 1.1f));

    assert_int_equal(LCM_ERROR_INVALID_PARAM, function_generator_initial_sample(&fg, -0.1f));

    fg.scale = 0.0f;
    assert_int_equal(LCM_ERROR_INVALID_PARAM, function_generator_initial_sample(&fg, 0.0f));
}

void test_function_generator_initial_sample(void **state)
{
    (void)state;
    function_generator_t fg;
    float sample;

    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          100, 0.0f, 1.0f, FG_FLAG_NONE, 0));

    assert_int_equal(LCM_SUCCESS, function_generator_initial_sample(&fg, 0.5f));

    assert_float_equal(fg.phase, M_PI, 0.00001);
    assert_int_equal(LCM_SUCCESS, function_generator_peek_sample(&fg, &sample, 1));

    assert_float_equal(0.6f, sample, 0.0f);

    // Make sure inverted works too
    assert_int_equal(LCM_SUCCESS, function_generator_init(&fg, FUNCTION_GENERATOR_SAWTOOTH, 1000,
                                                          100, 0.0f, 1.0f, FG_FLAG_INVERT, 0));

    assert_int_equal(LCM_SUCCESS, function_generator_initial_sample(&fg, 0.0f));

    assert_float_equal(fg.phase, 2 * M_PI, 0.00001);
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