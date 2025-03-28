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
#ifndef TEST_BOARD_MODE_H
#define TEST_BOARD_MODE_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "board_mode.h"
#include "footpads.h"
#include "mock_event_queue.h"
#include "mock_timer.h"
#include "config.h"

/**
 * @brief Initializes the board mode for testing
 *
 * This setup function is used in the testing framework to initialize the
 * board mode before running each test. It resets the mocks for the event
 * queue and timer, and then calls board_mode_init(). It also verifies that
 * the default state of the board mode is off.
 */
int board_mode_setup(void **state)
{
    (void)state;

    // Reset event queue and timer
    event_queue_init();
    timer_init();

    // Board mode subscribes to a lot of events
    expect_value(subscribe_event, event, EVENT_BUTTON_UP);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_SHUTDOWN);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_BOOT);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_MODE_CONFIG);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_RPM_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_EMERGENCY_FAULT);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_FOOTPAD_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_VESC_ALIVE);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_DUTY_CYCLE_CHANGED);
    expect_any(subscribe_event, callback);

    board_mode_init();

    // Default state is off
    assert_int_equal(board_mode_get(), BOARD_MODE_OFF);
    assert_int_equal(board_submode_get(), BOARD_SUBMODE_UNDEFINED);
    return 0;
}

/**
 * @brief Tests that the board mode transitions to idle mode
 *
 * This test verifies that the board mode transitions from the off state to
 * the booting state when a button down event is received, and then transitions
 * to the idle mode when the booting timer expires.
 *
 * This test also verifies that when the board is in the booting mode, other
 * events such as button up, footpad changed, and RPM changed do not trigger
 * any state transitions.
 */
void board_mode_to_idle(void)
{
    // On button down, transition to booting
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    board_mode_event_data_t expected_state = {0};
    expected_state.mode = BOARD_MODE_BOOTING;
    expected_state.submode = BOARD_SUBMODE_UNDEFINED;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    event_data_t null_event_data = {0};

    event_queue_call_mocked_callback(EVENT_COMMAND_BOOT, &null_event_data);

    assert_int_equal(board_mode_get(), BOARD_MODE_BOOTING);
    assert_int_equal(board_submode_get(), BOARD_SUBMODE_UNDEFINED);

    // The board is now in booting mode, no other events should trigger anything
    event_queue_call_mocked_callback(EVENT_BUTTON_UP, &null_event_data);
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &null_event_data);
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &null_event_data);

    // Except the VESC_ALIVE event, which takes us to idle mode
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Expect an idle timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_queue_call_mocked_callback(EVENT_VESC_ALIVE, &null_event_data);

    assert_int_equal(board_mode_get(), BOARD_MODE_IDLE);
    assert_int_equal(board_submode_get(), BOARD_SUBMODE_IDLE_ACTIVE);
}

/**
 * @brief Tests the initialization of the board mode.
 *
 * This test verifies that the board mode is properly initialized to the
 * off state. It is done in the setup and doesn't need to do anything else.
 */
void test_board_mode_init(void **state)
{
    (void)state;
    // Testing is done in the setup
}

/**
 * @brief Tests the transition of board mode from booting to idle.
 *
 * This test verifies that when the board mode changes from booting, it
 * correctly transitions to idle mode. It simulates the necessary events and
 * timer callbacks to ensure the correct state change occurs.
 */
void test_board_mode_boot(void **state)
{
    (void)state;
    board_mode_to_idle();
}

/**
 * @brief Tests the auto shutdown of the board mode.
 *
 * This test verifies that when the board mode is idle and no events occur, the
 * board mode will transition to the off state after a period of inactivity.
 * It simulates the necessary events and timer callbacks to ensure the correct
 * state change occurs.
 */
void test_board_mode_auto_shutdown(void **state)
{
    (void)state;
    board_mode_to_idle();

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    board_mode_event_data_t expected_state = {0};
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_DEFAULT;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Expect an ACTIVE->DEFAULT timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    // Trigger the timer callback to transition to default mode (assume timer_id
    // 2)
    call_timer_callback(1, 1000);

    assert_int_equal(board_mode_get(), BOARD_MODE_IDLE);
    assert_int_equal(board_submode_get(), BOARD_SUBMODE_IDLE_DEFAULT);

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_DOZING;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Expect a DEFAULT->DOZING timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    // Trigger the timer callback to transition to shutting down mode (assume
    // timer_id 3)
    call_timer_callback(2, 1000);

    assert_int_equal(board_mode_get(), BOARD_MODE_IDLE);
    assert_int_equal(board_submode_get(), BOARD_SUBMODE_IDLE_DOZING);

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_SHUTTING_DOWN;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Expect a DOZING->SHUTTING_DOWN timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    // Trigger the timer callback to transition to shutting down mode (assume
    // timer_id 4)
    call_timer_callback(3, 1000);

    assert_int_equal(board_mode_get(), BOARD_MODE_IDLE);
    assert_int_equal(board_submode_get(), BOARD_SUBMODE_IDLE_SHUTTING_DOWN);

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_OFF;
    expected_state.submode = BOARD_SUBMODE_UNDEFINED;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Trigger the timer callback to transition to shutting down mode (assume
    // timer_id 5)
    call_timer_callback(4, 1000);

    assert_int_equal(board_mode_get(), BOARD_MODE_OFF);
    assert_int_equal(board_submode_get(), BOARD_SUBMODE_UNDEFINED);
}

void test_board_mode_command_shutdown(void **state)
{
    (void)state;
    event_data_t event_data = {0};
    board_mode_to_idle();

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    board_mode_event_data_t expected_state = {0};
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_SHUTTING_DOWN;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Expect a SHUTTING_DOWN timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_queue_call_mocked_callback(EVENT_COMMAND_SHUTDOWN, &event_data);

    // Normally, the timer will fire and the board will shut down. We already
    // tested this in the auto shutdown test, so we don't need to repeat it.

    // If the user releases the button, the shutdown should be aborted
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Expect an IDLE_ACTIVE timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_queue_call_mocked_callback(EVENT_BUTTON_UP, &event_data);
}

void step_on_board(void)
{
    // In idle mode, footpad events should transition to riding mode
    board_mode_event_data_t expected_state = {0};
    event_data_t footpad_event_data = {0};
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_RIDING;
    expected_state.submode = BOARD_SUBMODE_RIDING_STOPPED;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // The code will check the vesc_serial parameters
    will_return(vesc_serial_get_duty_cycle, 0.0f);
    will_return(vesc_serial_get_rpm, 0);

    // The code will also disable the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);

    // Step on the footpad
    footpad_event_data.footpads_state = RIGHT_FOOTPAD | LEFT_FOOTPAD;
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &footpad_event_data);
}

void step_off_board(void)
{
    // From active riding, stepping off should return to idle mode
    board_mode_event_data_t expected_state = {0};
    event_data_t footpad_event_data = {0};
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // The code will check the RPM
    will_return(vesc_serial_get_rpm, 0);

    // Going to idle mode, so expect an idle timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    // Step off the footpad
    footpad_event_data.footpads_state = NONE_FOOTPAD;
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &footpad_event_data);
}

void enable_config_mode()
{
    board_mode_event_data_t expected_state = {0};
    event_data_t config_event_data = {0};
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_CONFIG;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Config mode will disable the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);

    // Step off the footpad
    config_event_data.enable = true;
    event_queue_call_mocked_callback(EVENT_COMMAND_MODE_CONFIG, &config_event_data);
}

void trigger_emergency_fault()
{
    board_mode_event_data_t expected_state = {0};
    event_data_t fault_event_data = {0};
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_FAULT;
    expected_state.submode = BOARD_SUBMODE_UNDEFINED;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // Fault mode will disable the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);

    // Step off the footpad
    fault_event_data.emergency_fault = EMERGENCY_FAULT_DIVIDE_BY_ZERO;
    event_queue_call_mocked_callback(EVENT_EMERGENCY_FAULT, &fault_event_data);
}

void test_board_mode_footpads(void **state)
{
    (void)state;
    event_data_t footpad_event_data = {0};
    // Before going to idle, the footpads don't do anything
    footpad_event_data.footpads_state = RIGHT_FOOTPAD;
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &footpad_event_data);

    // Transition to idle
    board_mode_to_idle();

    // Step on
    step_on_board();

    // Step off
    step_off_board();

    // Enable config mode
    enable_config_mode();

    // In config mode, the footpads are used as controls, so no state change
    footpad_event_data.footpads_state = RIGHT_FOOTPAD;
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &footpad_event_data);
}

void test_board_mode_duty_cycle(void **state)
{
    (void)state;
    event_data_t duty_cycle_event_data = {0};
    board_mode_event_data_t expected_state = {0};

    // Transition to idle
    board_mode_to_idle();

    // Duty cyle shouldn't do anything in idle mode
    duty_cycle_event_data.duty_cycle = 0.5f;
    event_queue_call_mocked_callback(EVENT_DUTY_CYCLE_CHANGED, &duty_cycle_event_data);

    // Step on board
    step_on_board();

    // Low duty cycle shouldn't do anything
    will_return(vesc_serial_get_duty_cycle, 10.0f);
    will_return(vesc_serial_get_rpm, 8);

    event_queue_call_mocked_callback(EVENT_DUTY_CYCLE_CHANGED, &duty_cycle_event_data);

    // High duty cycle should trigger warning
    will_return(vesc_serial_get_duty_cycle, 85.0f);
    will_return(vesc_serial_get_rpm, 8);

    // Riding mode will disable the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_RIDING;
    expected_state.submode = BOARD_SUBMODE_RIDING_WARNING;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    event_queue_call_mocked_callback(EVENT_DUTY_CYCLE_CHANGED, &duty_cycle_event_data);

    // Higher duty cycle should trigger danger
    will_return(vesc_serial_get_duty_cycle, 95.0f);
    will_return(vesc_serial_get_rpm, 8);

    // Riding mode will disable the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_RIDING;
    expected_state.submode = BOARD_SUBMODE_RIDING_DANGER;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    event_queue_call_mocked_callback(EVENT_DUTY_CYCLE_CHANGED, &duty_cycle_event_data);

    // Slowing down should go back to warning
    will_return(vesc_serial_get_duty_cycle, 84.0f);
    will_return(vesc_serial_get_rpm, 8);

    // Riding mode will disable the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);

    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_RIDING;
    expected_state.submode = BOARD_SUBMODE_RIDING_WARNING;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    event_queue_call_mocked_callback(EVENT_DUTY_CYCLE_CHANGED, &duty_cycle_event_data);
}

void test_board_mode_emergency_fault(void **state)
{
    (void)state;

    // Trigger a fault
    trigger_emergency_fault();
}

void move_board_forward(void)
{
    // In idle mode, RPM changes should transition to riding mode
    board_mode_event_data_t expected_state = {0};
    event_data_t rpm_event_data = {0};
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_RIDING;
    expected_state.submode = BOARD_SUBMODE_RIDING_SLOW;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // The code will check the vesc_serial parameters
    will_return(vesc_serial_get_duty_cycle, 0.0f);
    will_return(vesc_serial_get_rpm, 100U);

    // The code will also disable the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);

    // Move board forward
    rpm_event_data.rpm = 100;
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &rpm_event_data);
}

void stop_riding(void)
{
    // In idle mode, RPM changes should transition to riding mode
    board_mode_event_data_t expected_state = {0};
    event_data_t rpm_event_data = {0};
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_IDLE;
    expected_state.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // The code will check the footpads
    will_return(footpads_get_state, NONE_FOOTPAD);

    // Going to idle mode, so expect an idle timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    // Stop riding
    rpm_event_data.rpm = 0;
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &rpm_event_data);
}

void test_board_mode_rpm(void **state)
{
    (void)state;
    event_data_t rpm_event_data = {0};
    board_mode_event_data_t expected_state = {0};

    // RPM changes shouldn't do anything in off state
    rpm_event_data.rpm = 100;
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &rpm_event_data);

    // Transition to idle
    board_mode_to_idle();

    // move the board forward
    move_board_forward();

    // RPM updates shouldn't do change any states while moving
    rpm_event_data.rpm = -150;
    will_return(vesc_serial_get_duty_cycle, 0.0f);
    will_return(vesc_serial_get_rpm, -150);
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &rpm_event_data);

    // Unless we go faster, at normal speed
    expect_value(event_queue_push, event, EVENT_BOARD_MODE_CHANGED);
    expected_state.mode = BOARD_MODE_RIDING;
    expected_state.submode = BOARD_SUBMODE_RIDING_NORMAL;
    expect_check(event_queue_push, data, validate_board_mode_event_data,
                 (uintmax_t)&expected_state);

    // The code will check the vesc_serial parameters
    will_return(vesc_serial_get_duty_cycle, 50.0f);
    will_return(vesc_serial_get_rpm, SLOW_RPM_THRESHOLD + 100);

    // Setting RPM checks the idle timer
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);

    rpm_event_data.rpm = SLOW_RPM_THRESHOLD + 100;
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &rpm_event_data);

    // stop board
    stop_riding();
}

const struct CMUnitTest board_mode_tests[] = {
    cmocka_unit_test_setup(test_board_mode_init, board_mode_setup),
    cmocka_unit_test_setup(test_board_mode_boot, board_mode_setup),
    cmocka_unit_test_setup(test_board_mode_auto_shutdown, board_mode_setup),
    cmocka_unit_test_setup(test_board_mode_command_shutdown, board_mode_setup),
    cmocka_unit_test_setup(test_board_mode_footpads, board_mode_setup),
    cmocka_unit_test_setup(test_board_mode_emergency_fault, board_mode_setup),
    cmocka_unit_test_setup(test_board_mode_duty_cycle, board_mode_setup),
    cmocka_unit_test_setup(test_board_mode_rpm, board_mode_setup),
};

#endif
