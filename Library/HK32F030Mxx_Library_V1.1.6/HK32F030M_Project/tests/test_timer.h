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
#ifndef TEST_TIMER_H
#define TEST_TIMER_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "event_queue.h"
#include "mock_event_queue.h"
#include "timer.h"

/**
 * @brief Test setup function to initialize the timer module.
 *
 * This function is used in the testing framework to initialize the
 * timer module before running each test. It resets the mocked event
 * queue and calls timer_init(). It also verifies that the timer module
 * subscribes to the EVENT_SYS_TICK event.
 */
int test_timer_setup(void **state)
{
    // Reset event queue
    event_queue_init();

    expect_value(subscribe_event, event, EVENT_SYS_TICK);
    expect_any(subscribe_event, callback);
    timer_init();
    return 0;
}

/**
 * @brief Callback function used for testing the timer module.
 *
 * This function is used in tests to verify that a timer callback has been
 * called. It simply calls function_called() to signal that the callback has
 * been called.
 */
void test_timer_callback(uint32_t system_tick)
{
    function_called();
}

/**
 * @brief Tests that set_timer correctly sets a timer and
 *        timer_active_count correctly returns the active timer count.
 *
 * @param[in] state The test state.
 */
void test_set_timer(void **state)
{
    (void)state;
    timer_id_t timer_id = set_timer(1000, test_timer_callback, false);
    assert_int_equal(timer_id, 1);
    assert_int_equal(timer_active_count(), 1);
    assert_true(is_timer_active(timer_id));

    // Cancel the timer
    assert_int_equal(cancel_timer(timer_id), LCM_SUCCESS);

    assert_false(is_timer_active(timer_id));
    assert_int_equal(timer_active_count(), 0);
}

/**
 * @brief Tests that set_timer can be used to reset a timer, and that
 *        is_timer_repeating and get_timer_remaining correctly return the
 *        state of the timer.
 *
 * This test verifies that set_timer can be used to update the timeout period
 * of an active timer, and that the timer's repetition state is correctly
 * returned by is_timer_repeating. It also verifies that get_timer_remaining
 * correctly returns the timeout period of the timer.
 */
void test_reset_timer(void **state)
{
    (void)state;
    timer_id_t timer_id = set_timer(1000, test_timer_callback, false);
    assert_int_equal(timer_id, 1);
    assert_int_equal(timer_active_count(), 1);
    assert_true(is_timer_active(timer_id));
    assert_false(is_timer_repeating(timer_id));
    assert_int_equal(get_timer_remaining(timer_id), 1000);

    // Reset the timer
    timer_id = set_timer(2000, test_timer_callback, true);
    assert_int_equal(timer_id, 1);
    assert_int_equal(timer_active_count(), 1);
    assert_true(is_timer_active(timer_id));
    assert_true(is_timer_repeating(timer_id));
    assert_int_equal(get_timer_remaining(timer_id), 2000);
}

/**
 * @brief Tests that a timer can be triggered.
 *
 * This test verifies that a timer can be triggered and that the timer's
 * callback is called. It also verifies that the timer is no longer active
 * after it has been triggered.
 */
void test_timer_trigger(void **state)
{
    (void)state;
    event_data_t data = {0};
    data.system_tick = 0;

    timer_id_t timer_id = set_timer(2, test_timer_callback, false);
    assert_int_equal(get_timer_remaining(timer_id), 2);

    // Expect first tick to decrement the timer counter
    event_queue_call_mocked_callback(EVENT_SYS_TICK, &data);
    assert_int_equal(get_timer_remaining(timer_id), 1);

    // Expect second tick to trigger callback
    expect_function_call(test_timer_callback);
    event_queue_call_mocked_callback(EVENT_SYS_TICK, &data);

    // Should no longer be active
    assert_false(is_timer_active(timer_id));
    assert_int_equal(timer_active_count(), 0);
}

/**
 * @brief Tests the behavior of a repeating timer.
 *
 * This test verifies that a timer set to repeat triggers its callback
 * at the expected interval and remains active with its countdown reset.
 * It ensures that the timer reactivates after the callback is triggered,
 * maintaining its repeat functionality.
 *
 * @param[in] state The test state.
 */
void test_timer_repeating(void **state)
{
    (void)state;

    event_data_t data = {0};
    data.system_tick = 0;

    timer_id_t timer_id = set_timer(2, test_timer_callback, true);
    assert_true(is_timer_repeating(timer_id));      // Should be repeating
    assert_false(is_timer_repeating(timer_id + 1)); // Should not be repeating
    event_queue_call_mocked_callback(EVENT_SYS_TICK, &data);

    // Second tick should trigger callback
    expect_function_call(test_timer_callback);
    event_queue_call_mocked_callback(EVENT_SYS_TICK, &data);

    // Should still be active and countdown reset
    assert_true(is_timer_active(timer_id));
    assert_int_equal(timer_active_count(), 1);
    assert_int_equal(get_timer_remaining(timer_id), 2);
    assert_int_equal(get_timer_remaining(timer_id + 1), 0);
}

static timer_id_t test_timer_set_timer_in_callback_timer_id = 0;
static timer_id_t test_timer_cancel_repeating_timer_in_callback_timer_id = 0;

void test_timer_set_timer_in_callback_callback(uint32_t system_tick)
{
    function_called();
    set_timer(100, test_timer_set_timer_in_callback_callback, false);
}

void test_timer_cancel_repeating_timer_in_callback_callback(uint32_t system_tick)
{
    function_called();
    cancel_timer(test_timer_cancel_repeating_timer_in_callback_timer_id);
}

/**
 * @brief Tests setting a new timer within a callback.
 *
 * This test verifies that a new timer can be set within a timer callback,
 * and that the new timer is active with the correct timeout value.
 * It ensures that the callback function is called and the timer's state
 * is updated accordingly.
 *
 * @param[in] state The test state.
 */
void test_timer_set_timer_in_callback(void **state)
{
    (void)state;

    event_data_t data = {0};
    data.system_tick = 0;

    test_timer_set_timer_in_callback_timer_id =
        set_timer(1, test_timer_set_timer_in_callback_callback, false);

    expect_function_call(test_timer_set_timer_in_callback_callback);
    event_queue_call_mocked_callback(EVENT_SYS_TICK, &data);

    assert_true(is_timer_active(test_timer_set_timer_in_callback_timer_id));
    assert_int_equal(timer_active_count(), 1);
    assert_int_equal(get_timer_remaining(test_timer_set_timer_in_callback_timer_id), 100);
}

void test_timer_cancel_repeating_timer_in_callback(void **state)
{
    (void)state;

    event_data_t data = {0};
    data.system_tick = 0;

    test_timer_cancel_repeating_timer_in_callback_timer_id =
        set_timer(1, test_timer_cancel_repeating_timer_in_callback_callback, true);

    expect_function_call(test_timer_cancel_repeating_timer_in_callback_callback);
    event_queue_call_mocked_callback(EVENT_SYS_TICK, &data);

    assert_false(is_timer_active(test_timer_cancel_repeating_timer_in_callback_timer_id));
    assert_int_equal(timer_active_count(), 0);
}

/**
 * @brief Tests that the maximum number of timers can be used.
 *
 * This test verifies that the maximum number of timers can be used, and
 * that trying to create one more timer will result in an overflow error.
 *
 * @param[in] state The test state.
 */
void test_timer_overflow(void **state)
{
    (void)state;
    uint8_t max_timers = get_max_timers();

    // Create max_timers timers
    for (uint8_t i = 0; i < max_timers; i++)
    {
        timer_id_t timer_id = set_timer(1000, (void *)(i + 1), false);
        assert_true(is_timer_active(timer_id));
        assert_int_equal(timer_active_count(), i + 1);
    }

    // Create one more timer
    expect_value(fault, fault, EMERGENCY_FAULT_OVERFLOW);
    timer_id_t timer_id = set_timer(1000, (void *)(max_timers + 1), false);
    assert_int_equal(timer_id, INVALID_TIMER_ID);
    assert_int_equal(timer_active_count(), max_timers);
}

void test_timer_test_cancel_invalid_timer(void **state)
{
    (void)state;
    timer_id_t timer_id = 0;
    assert_int_equal(cancel_timer(timer_id), LCM_ERROR);

    timer_id = 100;
    assert_int_equal(cancel_timer(timer_id), LCM_ERROR);
}

const struct CMUnitTest timer_tests[] = {
    cmocka_unit_test_setup(test_set_timer, test_timer_setup),
    cmocka_unit_test_setup(test_reset_timer, test_timer_setup),
    cmocka_unit_test_setup(test_timer_trigger, test_timer_setup),
    cmocka_unit_test_setup(test_timer_repeating, test_timer_setup),
    cmocka_unit_test_setup(test_timer_set_timer_in_callback, test_timer_setup),
    cmocka_unit_test_setup(test_timer_cancel_repeating_timer_in_callback, test_timer_setup),
    cmocka_unit_test_setup(test_timer_overflow, test_timer_setup),
    cmocka_unit_test_setup(test_timer_test_cancel_invalid_timer, test_timer_setup),
};
#endif
