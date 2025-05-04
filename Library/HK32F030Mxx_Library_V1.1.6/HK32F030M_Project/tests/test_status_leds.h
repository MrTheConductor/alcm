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
#ifndef TEST_STATUS_LEDS_H
#define TEST_STATUS_LEDS_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "status_leds.h"
#include "status_leds_hw.h"
#include "settings.h"
#include "mock_event_queue.h"
#include "mock_animations.h"
#include "mock_status_leds_hw.h"
#include "timer.h"
#include "config.h"

static settings_t *settings = NULL; // Global variable to hold the settings for testing

int validate_status_leds_buffer(const status_leds_color_t *expected_buffer,
                                const status_leds_color_t *actual_buffer)
{
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        assert_int_equal(expected_buffer[i].r, actual_buffer[i].r);
        assert_int_equal(expected_buffer[i].g, actual_buffer[i].g);
        assert_int_equal(expected_buffer[i].b, actual_buffer[i].b);
    }

    return 1;
}

int test_status_leds_setup(void **state)
{
    // Reset event queue and timer
    event_queue_init();
    timer_init();

    // Initialize settings
    settings_init();
    settings = settings_get();
    settings->status_brightness = 1.0f;
    settings->enable_status_leds = true;
    settings->personal_color = 123.0f;

    expect_any(status_leds_hw_init, buffer);
    expect_function_call(status_leds_hw_init);
    expect_value(status_leds_hw_set_brightness, brightness, 1.0f);
    expect_function_call(stop_animation);

    status_leds_color_t expected_buffer[STATUS_LEDS_COUNT] = {0};
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        expected_buffer[i].r = 0x00;
        expected_buffer[i].g = 0x00;
        expected_buffer[i].b = 0x00;
    }

    expect_function_call(status_leds_hw_refresh);
    expect_value(status_leds_hw_enable, enable, true);
    expect_value(hsl_to_rgb, h, settings->personal_color);
    expect_value(hsl_to_rgb, s, SATURATION_DEFAULT);
    expect_value(hsl_to_rgb, l, LIGHTNESS_DEFAULT);
    expect_any(hsl_to_rgb, color);
    expect_function_call(hsl_to_rgb);

    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_FOOTPAD_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BATTERY_LEVEL_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_TOGGLE_LIGHTS);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_TOGGLE_BEEPER);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_CONTEXT_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_SETTINGS_CHANGED);
    expect_any(subscribe_event, callback);

    status_leds_init();
    validate_status_leds_buffer(expected_buffer, mock_status_leds_hw_get_buffer());

    return 0;
}

/**
 * @brief Test that the status LEDs are off when the board is turned off.
 *
 * This test ensures that when the board is turned off, the status LEDs are
 * correctly turned off as well. It verifies that the status LEDs are off
 * using the mocked status LEDs hardware module.
 */
static void test_status_leds_off(void **state)
{
    // Turn on all the LEDs for setup
    status_leds_color_t expected_buffer[STATUS_LEDS_COUNT] = {0};
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        expected_buffer[i].r = 0xFF;
        expected_buffer[i].g = 0xFF;
        expected_buffer[i].b = 0xFF;
    }

    status_leds_color_t color = {0};
    color.r = 0xFF;
    color.g = 0xFF;
    color.b = 0xFF;
    status_leds_set_color(&color, 0, STATUS_LEDS_COUNT - 1);
    expect_function_call(status_leds_hw_refresh);
    status_leds_refresh();
    validate_status_leds_buffer(expected_buffer, mock_status_leds_hw_get_buffer());

    // Set the board mode to OFF
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_OFF;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;

    // All LEDs should be off
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        expected_buffer[i].r = 0x00;
        expected_buffer[i].g = 0x00;
        expected_buffer[i].b = 0x00;
    }

    will_return(board_mode_get, BOARD_MODE_OFF);

    expect_function_call(stop_animation);
    expect_function_call(status_leds_hw_refresh);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
    validate_status_leds_buffer(expected_buffer, mock_status_leds_hw_get_buffer());
}

/**
 * @brief Test setting colors for specific ranges of status LEDs.
 *
 * This test verifies that the status LEDs are correctly set to specified colors
 * for given index ranges. It checks the following scenarios:
 *
 * 1. Set LEDs 2 to 4 to red and verify the buffer is updated correctly.
 * 2. Set LEDs 7 to 9 to green and verify the buffer is updated correctly.
 * 3. Set LED 0 to blue and verify the buffer is updated correctly.
 *
 * For each scenario, it uses the mocked status LEDs hardware module to
 * validate that the buffer reflects the expected colors.
 *
 * @param state Unused parameter required by the cmocka framework.
 */
static void test_status_leds_set_color(void **state)
{
    // [X] [X] [R] [R] [R] [X] [X] [X] [X] [X]
    status_leds_color_t expected_buffer[STATUS_LEDS_COUNT] = {0};
    for (uint8_t i = 2; i < 5; i++)
    {
        expected_buffer[i].r = 0xFF;
        expected_buffer[i].g = 0x00;
        expected_buffer[i].b = 0x00;
    }

    status_leds_color_t color = {0};
    color.r = 0xFF;
    assert_int_equal(LCM_SUCCESS, status_leds_set_color(&color, 2, 4));
    expect_function_call(status_leds_hw_refresh);
    status_leds_refresh();
    validate_status_leds_buffer(expected_buffer, mock_status_leds_hw_get_buffer());

    // [X] [X] [R] [R] [R] [X] [X] [G] [G] [G]
    for (uint8_t i = 7; i < 10; i++)
    {
        expected_buffer[i].r = 0x00;
        expected_buffer[i].g = 0xFF;
        expected_buffer[i].b = 0x00;
    }
    color.r = 0x00;
    color.g = 0xFF;
    assert_int_equal(LCM_SUCCESS, status_leds_set_color(&color, 7, 9));
    expect_function_call(status_leds_hw_refresh);
    status_leds_refresh();
    validate_status_leds_buffer(expected_buffer, mock_status_leds_hw_get_buffer());

    // [B] [X] [R] [R] [R] [X] [X] [G] [G] [G]
    for (uint8_t i = 0; i < 1; i++)
    {
        expected_buffer[i].r = 0x00;
        expected_buffer[i].g = 0x00;
        expected_buffer[i].b = 0xFF;
    }
    color.r = 0x00;
    color.g = 0x00;
    color.b = 0xFF;
    assert_int_equal(LCM_SUCCESS, status_leds_set_color(&color, 0, 0));
    expect_function_call(status_leds_hw_refresh);
    status_leds_refresh();
    validate_status_leds_buffer(expected_buffer, mock_status_leds_hw_get_buffer());

    // Test invalid range
    assert_int_equal(LCM_ERROR, status_leds_set_color(&color, 4, 2));
    assert_int_equal(LCM_ERROR, status_leds_set_color(NULL, 0, 0));
    assert_int_equal(LCM_ERROR, status_leds_set_color(NULL, 0, STATUS_LEDS_COUNT));
}

void expect_fill_animation(void)
{
    expect_any(fill_animation_setup, buffer);
    expect_any(fill_animation_setup, color_mode);
    expect_any(fill_animation_setup, brightness_mode);
    expect_any(fill_animation_setup, fill_mode);
    expect_value(fill_animation_setup, first_led, 0U);
    expect_value(fill_animation_setup, last_led, STATUS_LEDS_COUNT - 1U);
    expect_any(fill_animation_setup, hue_min);
    expect_any(fill_animation_setup, hue_max);
    expect_any(fill_animation_setup, color_speed);
    expect_any(fill_animation_setup, brightness_min);
    expect_any(fill_animation_setup, brightness_max);
    expect_any(fill_animation_setup, brightness_speed);
    expect_any(fill_animation_setup, brightness_sequence);
    expect_any(fill_animation_setup, rgb);
    expect_function_call(fill_animation_setup);
    will_return(fill_animation_setup, 1U);
}

void expect_scan_animation(void)
{
    expect_any(scan_animation_setup, buffer);
    expect_any(scan_animation_setup, direction);
    expect_any(scan_animation_setup, color_mode);
    expect_any(scan_animation_setup, movement_speed);
    expect_any(scan_animation_setup, sigma);
    expect_any(scan_animation_setup, hue_min);
    expect_any(scan_animation_setup, hue_max);
    expect_any(scan_animation_setup, color_speed);
    expect_any(scan_animation_setup, scan_start);
    expect_any(scan_animation_setup, scan_end);
    expect_any(scan_animation_setup, init_mu);
    expect_any(scan_animation_setup, rgb);
    expect_function_call(scan_animation_setup);
    will_return(scan_animation_setup, 1U);
}


static void test_status_leds_boot(void **state)
{
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_BOOTING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    will_return(board_mode_get, BOARD_MODE_BOOTING);

    // Disable boot animation
    settings->boot_animation = ANIMATION_OPTION_NONE;

    // Expect a fade to black
    expect_any(fade_animation_setup, buffer);
    expect_value(fade_animation_setup, period, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT);
    expect_value(fade_animation_setup, callback, NULL);
    expect_function_call(fade_animation_setup);
    will_return(fade_animation_setup, 1U);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Set boot animation to float wheel classic
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    settings->boot_animation = ANIMATION_OPTION_FLOATWHEEL_CLASSIC;
    expect_scan_animation();

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Any non-mode change event should not affect the boot animation
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &data);

    will_return(board_mode_get, BOARD_MODE_BOOTING);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);
}

static void test_status_leds_fault(void **state)
{
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_FAULT;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    will_return(board_mode_get, BOARD_MODE_FAULT);

    // Expect a fill animation
    expect_fill_animation();
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Any non-mode change event should not affect the fault animation
    will_return(board_mode_get, BOARD_MODE_FAULT);
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &data);

    will_return(board_mode_get, BOARD_MODE_FAULT);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);
}

static void test_status_leds_toggle(void **state)
{
    event_data_t data = {0};
    status_leds_color_t expected_buffer[STATUS_LEDS_COUNT] = {0};

    // Turn off the LEDs
    settings->enable_status_leds = false;

    // Expect a fade to black
    expect_any(fade_animation_setup, buffer);
    expect_value(fade_animation_setup, period, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT);
    expect_not_value(fade_animation_setup, callback, NULL);
    expect_function_call(fade_animation_setup);
    will_return(fade_animation_setup, 1U);

    event_queue_call_mocked_callback(EVENT_COMMAND_TOGGLE_LIGHTS, &data);

    // Events should not affect the fade animation
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &data);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);

    // Simulate animation completed
    expect_function_call(stop_animation);

    // All LEDs should be off
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        expected_buffer[i].r = 0x00;
        expected_buffer[i].g = 0x00;
        expected_buffer[i].b = 0x00;
    }
    expect_function_call(status_leds_hw_refresh);
    expect_value(status_leds_hw_enable, enable, false);

    fade_animation_callback();
    validate_status_leds_buffer(expected_buffer, mock_status_leds_hw_get_buffer());

    // Turn on the LEDs
    settings->enable_status_leds = true;

    // Expect the LEDs to be turned on
    expect_value(status_leds_hw_enable, enable, true);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);
    will_return(footpads_get_state, NONE_FOOTPAD);
    will_return(vesc_serial_get_battery_level, 90.0f);

    // Expect stop animation
    expect_function_call(stop_animation);
    expect_scan_animation();

    event_queue_call_mocked_callback(EVENT_COMMAND_TOGGLE_LIGHTS, &data);
}

static void test_status_leds_idle_dozing(void **state)
{
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_DOZING;
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);

    // Disable dozing animation
    settings->dozing_animation = ANIMATION_OPTION_NONE;

    // Expect a fade to black
    expect_any(fade_animation_setup, buffer);
    expect_value(fade_animation_setup, period, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT);
    expect_value(fade_animation_setup, callback, NULL);
    expect_function_call(fade_animation_setup);
    will_return(fade_animation_setup, 1U);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Status LEDs should stay off, even if battery changes
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);

    // Set dozing animation to rainbow (fill animation)
    settings->dozing_animation = ANIMATION_OPTION_RAINBOW_MIRROR;
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    expect_fill_animation();
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Animation keeps running, even if battery changes
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);
}

const struct CMUnitTest status_leds_tests[] = {
    cmocka_unit_test_setup(test_status_leds_off, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_set_color, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_boot, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_fault, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_toggle, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_idle_dozing, test_status_leds_setup),
};

#endif // TEST_STATUS_LEDS_H
