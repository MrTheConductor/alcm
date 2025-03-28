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
#ifndef TEST_HEADLIGHTS_H
#define TEST_HEADLIGHTS_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "board_mode.h"
#include "headlights.h"
#include "headlights_hw.h"
#include "mock_event_queue.h"
#include "mock_timer.h"
#include "settings.h"

/**
 * @brief Setup function for headlights tests
 *
 * This setup function is used in the testing framework to initialize the
 * headlights module before running each test. It sets up the expected calls to
 * headlights_hw_set_direction() and subscribe_event().
 */
int headlights_setup(void **state)
{
    // Reset event queue and timer
    event_queue_init();
    timer_init();

    // Set up settings
    settings_init();
    settings_t *settings = settings_get();
    settings->enable_headlights = true;
    settings->headlight_brightness = 1.0f;

    expect_function_call(headlights_hw_init);
    expect_value(headlights_hw_enable, enable, true);
    expect_value(headlights_hw_set_direction, direction, HEADLIGHTS_DIRECTION_NONE);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_RPM_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_TOGGLE_LIGHTS);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_CONTEXT_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_SETTINGS_CHANGED);
    expect_any(subscribe_event, callback);

    headlights_init();
    return 0;
}

void test_headlights_boot(void **state)
{
    (void)state; // Unused

    uint16_t brightness = HEADLIGHTS_HW_MAX_BRIGHTNESS;

    // When the board boots up, turn on the headlights and set direction to
    // forward
    expect_value(headlights_hw_set_direction, direction, HEADLIGHTS_DIRECTION_FORWARD);
    expect_value(headlights_hw_set_brightness, brightness, brightness);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_BOOTING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;

    // Add mocks
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    will_return(headlights_hw_get_direction, HEADLIGHTS_DIRECTION_FORWARD);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

void test_headlights_riding(void **state)
{
    (void)state; // Unused

    uint16_t brightness = HEADLIGHTS_HW_MAX_BRIGHTNESS;

    // In riding mode, set full brightness
    expect_value(headlights_hw_set_brightness, brightness, brightness);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_RIDING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;

    // Add mocks
    will_return(board_mode_get, BOARD_MODE_RIDING);
    will_return(headlights_hw_get_direction, HEADLIGHTS_DIRECTION_FORWARD);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Simulate forward movement
    will_return(vesc_serial_get_rpm, 1000);
    will_return(headlights_hw_get_direction, HEADLIGHTS_DIRECTION_FORWARD);
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &data);

    // Simulate change in direction
    will_return(vesc_serial_get_rpm, -1000);
    will_return(headlights_hw_get_direction, HEADLIGHTS_DIRECTION_FORWARD);
    will_return(headlights_hw_get_brightness, 1000);

    // Expect a timer to be set for transition
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &data);
}

void test_headlights_idle_active(void **state)
{
    (void)state; // Unused

    uint16_t brightness = HEADLIGHTS_HW_MAX_BRIGHTNESS;

    // In in idle active mode set full brightness
    expect_value(headlights_hw_set_brightness, brightness, brightness);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;

    // Add mocks
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);
    will_return(headlights_hw_get_direction, HEADLIGHTS_DIRECTION_FORWARD);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

void test_headlights_idle_default(void **state)
{
    (void)state; // Unused

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_DEFAULT;

    // Add mocks
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DEFAULT);
    will_return(headlights_hw_get_brightness, HEADLIGHTS_HW_MAX_BRIGHTNESS);

    // Expect animation timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

const struct CMUnitTest headlights_tests[] = {
    cmocka_unit_test_setup(test_headlights_boot, headlights_setup),
    cmocka_unit_test_setup(test_headlights_riding, headlights_setup),
    cmocka_unit_test_setup(test_headlights_idle_active, headlights_setup),
    cmocka_unit_test_setup(test_headlights_idle_default, headlights_setup),
};

#endif // TEST_HEADLIGHTS_H