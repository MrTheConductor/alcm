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
#ifndef TEST_POWER_H
#define TEST_POWER_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "board_mode.h"
#include "event_queue.h"
#include "mock_event_queue.h"
#include "power.h"
#include "power_hw.h"

/**
 * @brief Tests that power_init() sets the power and charge states to off, and
 *        subscribes to the board mode changed event.
 */
void test_power_init(void **state)
{
    (void)state; // Unused

    // Setup
    event_queue_init();

    // Expectations
    expect_function_call(power_hw_init);
    expect_value(power_hw_set_power, power_hw, POWER_HW_OFF);
    expect_value(power_hw_set_charge, power_hw, POWER_HW_OFF);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);

    // Run the function under test
    power_init();
}

/**
 * @brief Tests that the power is set to on when the board mode is set to
 *        BOARD_MODE_BOOTING.
 *
 * This test ensures that when the board mode changes to BOARD_MODE_BOOTING,
 * the power hardware is correctly set to the on state. It verifies that
 * the power initialization and event subscription occur, and that the
 * power state is set to on as expected.
 */
void test_power_on(void **state)
{
    (void)state; // Unused

    // Setup
    event_queue_init();

    expect_function_call(power_hw_init);
    expect_value(power_hw_set_power, power_hw, POWER_HW_OFF);
    expect_value(power_hw_set_charge, power_hw, POWER_HW_OFF);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(power_hw_set_power, power_hw, POWER_HW_ON);

    // Call init to setup callback
    power_init();

    // Run the function under test
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_BOOTING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief Tests that the power is set to off when the board mode is set to
 *        BOARD_MODE_OFF.
 *
 * This test ensures that when the board mode changes to BOARD_MODE_OFF,
 * the power hardware is correctly set to the off state. It verifies that
 * the power initialization and event subscription occur, and that the
 * power state is set to off as expected.
 */
void test_power_off(void **state)
{
    (void)state; // Unused

    // Setup
    event_queue_init();

    expect_function_call(power_hw_init);
    expect_value(power_hw_set_power, power_hw, POWER_HW_OFF);
    expect_value(power_hw_set_charge, power_hw, POWER_HW_OFF);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(power_hw_set_power, power_hw, POWER_HW_OFF);

    // Call init to setup callback
    power_init();

    // Run the function under test
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_OFF;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief Tests that the power remains on in all other board modes.
 *
 * This test ensures that the power is not turned off when the board mode is
 * set to any other value other than BOARD_MODE_OFF. This is important to
 * ensure that the power remains available to the system even when the
 * board is not in use.
 */
void test_power_remains_on_in_all_other_states(void **state)
{
    (void)state; // Unused

    // Setup
    event_queue_init();

    expect_function_call(power_hw_init);
    expect_value(power_hw_set_power, power_hw, POWER_HW_OFF);
    expect_value(power_hw_set_charge, power_hw, POWER_HW_OFF);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(power_hw_set_power, power_hw, POWER_HW_ON);

    // Call init to setup callback
    power_init();

    // Run the function under test
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_BOOTING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    data.board_mode.mode = BOARD_MODE_UNKNOWN;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    data.board_mode.mode = BOARD_MODE_RIDING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    data.board_mode.mode = BOARD_MODE_CHARGING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    data.board_mode.mode = BOARD_MODE_FAULT;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

void test_power_raise_error_on_invalid_event(void **state)
{
    (void)state; // Unused

    // Reset event queue
    event_queue_init();

    expect_function_call(power_hw_init);
    expect_value(power_hw_set_power, power_hw, POWER_HW_OFF);
    expect_value(power_hw_set_charge, power_hw, POWER_HW_OFF);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(fault, fault, EMERGENCY_FAULT_INVALID_EVENT);

    // Call init to setup callback
    power_init();

    // Run the function under test
    event_data_t data = {0};
    event_queue_test_bad_event(EVENT_BOARD_MODE_CHANGED, EVENT_NULL, &data);
}

const struct CMUnitTest power_tests[] = {
    cmocka_unit_test(test_power_init),
    cmocka_unit_test(test_power_on),
    cmocka_unit_test(test_power_off),
    cmocka_unit_test(test_power_remains_on_in_all_other_states),
    cmocka_unit_test(test_power_raise_error_on_invalid_event),
};

#endif // TEST_POWER_H