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
#ifndef TEST_HYSTERESIS_H
#define TEST_HYSTERESIS_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "hysteresis.h"

static void test_hysteresis_init_null(void **state)
{
    assert_int_equal(LCM_ERROR, hysteresis_init(NULL, 100, 50));
}

static void test_hysteresis_init_invalid_thresholds(void **state)
{
    hysteresis_t h = {0};

    assert_int_equal(LCM_ERROR, hysteresis_init(&h, 50, 100));
    assert_int_equal(STATE_ERROR, h.state);
}

static void test_hysteresis_init_equal_thresholds(void **state)
{
    hysteresis_t h = {0};

    // set == reset is a valid (if degenerate) boundary - only strictly-less
    // is rejected.
    assert_int_equal(LCM_SUCCESS, hysteresis_init(&h, 100, 100));
    assert_int_equal(STATE_RESET, h.state);
    assert_int_equal(100, h.set_threshold);
    assert_int_equal(100, h.reset_threshold);
}

static void test_hysteresis_init_valid(void **state)
{
    hysteresis_t h = {0};

    assert_int_equal(LCM_SUCCESS, hysteresis_init(&h, 100, 50));
    assert_int_equal(STATE_RESET, h.state);
    assert_int_equal(100, h.set_threshold);
    assert_int_equal(50, h.reset_threshold);
}

static void test_apply_hysteresis_null(void **state)
{
    assert_int_equal(STATE_ERROR, apply_hysteresis(NULL, 0));
}

static void test_apply_hysteresis_error_state_passthrough(void **state)
{
    hysteresis_t h = {0};
    h.state = STATE_ERROR;
    h.set_threshold = 100;
    h.reset_threshold = 50;

    // From STATE_ERROR, apply_hysteresis never transitions - it just
    // reports the (broken) state back, regardless of value.
    assert_int_equal(STATE_ERROR, apply_hysteresis(&h, 0));
    assert_int_equal(STATE_ERROR, apply_hysteresis(&h, 1000));
    assert_int_equal(STATE_ERROR, h.state);
}

static void test_apply_hysteresis_set_boundary(void **state)
{
    hysteresis_t h = {0};
    hysteresis_init(&h, 100, 50);

    // One below the set threshold: stays RESET
    assert_int_equal(STATE_RESET, apply_hysteresis(&h, 99));
    assert_int_equal(STATE_RESET, h.state);

    // Exactly at the set threshold: transitions to SET (>=)
    assert_int_equal(STATE_SET, apply_hysteresis(&h, 100));
    assert_int_equal(STATE_SET, h.state);
}

static void test_apply_hysteresis_reset_boundary(void **state)
{
    hysteresis_t h = {0};
    hysteresis_init(&h, 100, 50);
    apply_hysteresis(&h, 100); // drive to SET first

    // Exactly at the reset threshold: stays SET (strict <)
    assert_int_equal(STATE_SET, apply_hysteresis(&h, 50));
    assert_int_equal(STATE_SET, h.state);

    // One below the reset threshold: transitions to RESET
    assert_int_equal(STATE_RESET, apply_hysteresis(&h, 49));
    assert_int_equal(STATE_RESET, h.state);
}

static void test_apply_hysteresis_holds_between_thresholds(void **state)
{
    hysteresis_t h = {0};
    hysteresis_init(&h, 100, 50);

    // Starting in RESET, a value strictly between the thresholds doesn't
    // set (only >= set_threshold does from RESET).
    assert_int_equal(STATE_RESET, apply_hysteresis(&h, 75));

    apply_hysteresis(&h, 100); // drive to SET

    // Starting in SET, the same in-between value doesn't reset (only
    // < reset_threshold does from SET) - this is the whole point of
    // hysteresis: the band between thresholds is "sticky".
    assert_int_equal(STATE_SET, apply_hysteresis(&h, 75));
}

static const struct CMUnitTest hysteresis_tests[] = {
    cmocka_unit_test(test_hysteresis_init_null),
    cmocka_unit_test(test_hysteresis_init_invalid_thresholds),
    cmocka_unit_test(test_hysteresis_init_equal_thresholds),
    cmocka_unit_test(test_hysteresis_init_valid),
    cmocka_unit_test(test_apply_hysteresis_null),
    cmocka_unit_test(test_apply_hysteresis_error_state_passthrough),
    cmocka_unit_test(test_apply_hysteresis_set_boundary),
    cmocka_unit_test(test_apply_hysteresis_reset_boundary),
    cmocka_unit_test(test_apply_hysteresis_holds_between_thresholds),
};

#endif // TEST_HYSTERESIS_H
