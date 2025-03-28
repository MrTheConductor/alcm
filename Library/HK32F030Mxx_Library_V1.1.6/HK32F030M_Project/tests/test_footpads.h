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
#ifndef TEST_FOOTPADS_H
#define TEST_FOOTPADS_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "board_mode.h"
#include "event_queue.h"
#include "footpads.h"
#include "lcm_types.h"
#include "mock_event_queue.h"
#include "mock_timer.h"

int footpads_setup(void **state)
{
    (void)state;

    // Reset event queue and timer
    event_queue_init();
    timer_init();

    expect_function_call(footpads_hw_init);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);

    footpads_init();
    return 0;
}

void go_to_idle(void)
{
    // Expect timer to be set, don't care about parameters
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_DEFAULT;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

void test_footpads_init(void **state)
{
    (void)state;
}

void assert_left_footpad(void)
{
    // Set hardware to return a left footpad
    will_return(footpads_hw_get_left, 3.0f);
    will_return(footpads_hw_get_right, 0.0f);

    // Expect a footpad event
    expect_value(event_queue_push, event, EVENT_FOOTPAD_CHANGED);

    footpads_state_t expected_state = LEFT_FOOTPAD;
    expect_check(event_queue_push, data, validate_footpads_state, (uintmax_t)&expected_state);

    // trigger the timer
    call_timer_callback(1, 100);

    // check the state
    assert_int_equal(footpads_get_state(), LEFT_FOOTPAD);
}

void assert_right_footpad(void)
{
    // Set hardware to return a left footpad
    will_return(footpads_hw_get_left, 0.0f);
    will_return(footpads_hw_get_right, 3.0f);

    // Expect a footpad event
    expect_value(event_queue_push, event, EVENT_FOOTPAD_CHANGED);

    footpads_state_t expected_state = RIGHT_FOOTPAD;
    expect_check(event_queue_push, data, validate_footpads_state, (uintmax_t)&expected_state);

    // trigger the timer
    call_timer_callback(1, 100);

    // check the state
    assert_int_equal(footpads_get_state(), RIGHT_FOOTPAD);
}

void assert_both_footpads(void)
{
    // Set hardware to return a left footpad
    will_return(footpads_hw_get_left, 3.0f);
    will_return(footpads_hw_get_right, 3.0f);

    // Expect a footpad event
    expect_value(event_queue_push, event, EVENT_FOOTPAD_CHANGED);

    footpads_state_t expected_state = RIGHT_FOOTPAD | LEFT_FOOTPAD;
    expect_check(event_queue_push, data, validate_footpads_state, (uintmax_t)&expected_state);

    // trigger the timer
    call_timer_callback(1, 100);

    // check the state
    assert_int_equal(footpads_get_state(), RIGHT_FOOTPAD | LEFT_FOOTPAD);
}

void release_footpads(void)
{
    // Set hardware to return a left footpad
    will_return(footpads_hw_get_left, 0.0f);
    will_return(footpads_hw_get_right, 0.0f);

    // Expect a footpad event
    expect_value(event_queue_push, event, EVENT_FOOTPAD_CHANGED);

    footpads_state_t expected_state = NONE_FOOTPAD;
    expect_check(event_queue_push, data, validate_footpads_state, (uintmax_t)&expected_state);

    // trigger the timer
    call_timer_callback(1, 100);

    // check the state
    assert_int_equal(footpads_get_state(), NONE_FOOTPAD);
}

void test_left_footpad(void **state)
{
    (void)state;

    go_to_idle();
    assert_left_footpad();
    release_footpads();
}

void test_right_footpad(void **state)
{
    (void)state;

    go_to_idle();
    assert_right_footpad();
    release_footpads();
}

void test_both_footpads(void **state)
{
    (void)state;

    go_to_idle();
    assert_both_footpads();
    release_footpads();
}

void test_footpads_timer_cancel(void **state)
{
    // When the board mode is changed, the timer should be cancelled
    (void)state;

    go_to_idle();

    // Going to riding mode should just check that the timer is active
    will_return(is_timer_active, true);
    expect_value(is_timer_active, timer_id, 1);
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_RIDING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Going back to idle should also be okay
    will_return(is_timer_active, true);
    expect_value(is_timer_active, timer_id, 1);
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Going to charging mode should cancel the timer
    will_return(is_timer_active, true);
    expect_value(is_timer_active, timer_id, 1);
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    data.board_mode.mode = BOARD_MODE_CHARGING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

const struct CMUnitTest footpads_tests[] = {
    cmocka_unit_test_setup(test_footpads_init, footpads_setup),
    cmocka_unit_test_setup(test_left_footpad, footpads_setup),
    cmocka_unit_test_setup(test_right_footpad, footpads_setup),
    cmocka_unit_test_setup(test_both_footpads, footpads_setup),
    cmocka_unit_test_setup(test_footpads_timer_cancel, footpads_setup),
};

#endif // TEST_FOOTPADS_H
