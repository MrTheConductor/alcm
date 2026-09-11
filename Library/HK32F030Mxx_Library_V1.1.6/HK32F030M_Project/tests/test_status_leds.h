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
    settings->status_brightness = 255U; // 1.0
    settings->enable_status_leds = true;
    settings->personal_color = 123U;

    expect_any(status_leds_hw_init, buffer);
    expect_function_call(status_leds_hw_init);
    expect_value(status_leds_hw_set_brightness, brightness, 255U);
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
    expect_value(hsl_to_rgb, h, degrees_to_fixed16(settings->personal_color));
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
    expect_value(subscribe_event, event, EVENT_DUTY_CYCLE_CHANGED);
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

    // state_changed() reads board_mode_get() twice: once for the config-mode
    // check (which must run even when status LEDs are disabled) and once
    // inside update_display()'s own dispatch.
    will_return(board_mode_get, BOARD_MODE_OFF);
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

void expect_fill_animation(void);
void expect_scan_animation(void);

static void test_status_leds_boot(void **state)
{
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_BOOTING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    // state_changed() reads board_mode_get() twice: once for the config-mode
    // check (which must run even when status LEDs are disabled) and once
    // inside update_display()'s own dispatch.
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    will_return(board_mode_get, BOARD_MODE_BOOTING);

    // Disable boot animation
    settings->boot_animation = ANIMATION_OPTION_NONE;

    // Expect a fade to black
    expect_any(fade_animation_setup, buffer);
    expect_value(fade_animation_setup, period, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT);
    expect_value(fade_animation_setup, callback, NULL);
    expect_function_call(fade_animation_setup);
    will_return(fade_animation_setup, 1U);
    will_return(vesc_serial_get_imu_roll, 0);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Set boot animation to rainbow mirror
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    settings->boot_animation = ANIMATION_OPTION_RAINBOW_MIRROR;
    will_return(vesc_serial_get_imu_roll, 0);
    expect_fill_animation();

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Any non-mode change event should not affect the boot animation
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &data);

    will_return(board_mode_get, BOARD_MODE_BOOTING);
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);
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

static void test_status_leds_fault(void **state)
{
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_FAULT;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    // state_changed() reads board_mode_get() twice: once for the config-mode
    // check (which must run even when status LEDs are disabled) and once
    // inside update_display()'s own dispatch. board_submode_get() is only
    // read once, by status_leds_handle_fault(), since mode != IDLE short-
    // circuits the config-mode check before it reads submode.
    will_return(board_mode_get, BOARD_MODE_FAULT);
    will_return(board_mode_get, BOARD_MODE_FAULT);
    will_return(board_submode_get, BOARD_SUBMODE_UNDEFINED);

    // Expect a fill animation
    expect_fill_animation();
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Any non-mode change event should not affect the fault animation
    will_return(board_mode_get, BOARD_MODE_FAULT);
    will_return(board_mode_get, BOARD_MODE_FAULT);
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &data);

    will_return(board_mode_get, BOARD_MODE_FAULT);
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

    // Events should not affect the fade animation. state_changed()
    // unconditionally checks board_mode_get() (to keep the config-mode and
    // locked indicators visible even when status LEDs are otherwise
    // disabled), so every event routed there needs a mocked return here
    // too. Since mode is IDLE, the config-mode check also reads submode;
    // it's not CONFIG here, so leds stay off and update_display() (and its
    // own board_mode_get()/board_submode_get() reads) never runs.
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);
    event_queue_call_mocked_callback(EVENT_FOOTPAD_CHANGED, &data);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);
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
    will_return(vesc_serial_get_battery_level, 900); // 90.0%

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
    // state_changed() reads board_mode_get()/board_submode_get() twice:
    // once for the config-mode check (which must run even when status LEDs
    // are disabled) and once inside update_display()'s own dispatch.
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
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
    will_return(vesc_serial_get_imu_roll, 0);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Status LEDs should stay off, even if battery changes
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);

    // Set dozing animation to rainbow (fill animation)
    settings->dozing_animation = ANIMATION_OPTION_RAINBOW_MIRROR;
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    will_return(vesc_serial_get_imu_roll, 0);
    expect_fill_animation();
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Animation keeps running, even if battery changes
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    event_queue_call_mocked_callback(EVENT_BATTERY_LEVEL_CHANGED, &data);
}

// Not declared in status_leds.h - non-static so tests can call them
// directly, matching this codebase's convention. Called directly, these
// don't involve board_mode_get()/board_submode_get() at all.
uint16_t status_leds_start_animation_option(animation_option_t option);
void display_battery(int16_t battery_level);
void display_duty_cycle(int16_t duty_cycle);
void display_footpad(footpads_state_t footpad);

/**
 * @brief Loops status_leds_start_animation_option() through every option
 * not already exercised by the boot/dozing tests above, to raise
 * status_leds.c's overall branch coverage cheaply - each option just
 * needs to dispatch to the right underlying animation-setup call.
 */
static void test_start_animation_option_dispatches_every_option(void **state)
{
    // ENABLE_IMU_EVENTS is on, so every call reads the IMU roll first,
    // regardless of which animation option is requested.
    will_return(vesc_serial_get_imu_roll, 0);
    expect_scan_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_RAINBOW_SCAN);

#ifdef ENABLE_KNIGHT_RIDER_ANIMATION
    will_return(vesc_serial_get_imu_roll, 0);
    expect_scan_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_KNIGHT_RIDER);
#endif

    will_return(vesc_serial_get_imu_roll, 0);
    expect_fill_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_RAINBOW_BAR);

#ifdef ENABLE_THE_FUZZ_ANIMATION
    will_return(vesc_serial_get_imu_roll, 0);
    expect_fill_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_THE_FUZZ);
#endif

#ifdef ENABLE_FIRE_ANIMATION
    will_return(vesc_serial_get_imu_roll, 0);
    expect_any(fire_animation_setup, buffer);
    expect_function_call(fire_animation_setup);
    will_return(fire_animation_setup, 1U);
    status_leds_start_animation_option(ANIMATION_OPTION_FIRE);
#endif

#ifdef ENABLE_EXPANDING_PULSE_ANIMATION
    will_return(vesc_serial_get_imu_roll, 0);
    expect_scan_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_EXPANDING_PULSE);
#endif

    will_return(vesc_serial_get_imu_roll, 0);
    expect_fill_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_120_SCROLL);

#ifdef ENABLE_IMPLODING_PULSE_ANIMATION
    will_return(vesc_serial_get_imu_roll, 0);
    expect_scan_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_IMPLODING_PULSE);
#endif

    will_return(vesc_serial_get_imu_roll, 0);
    expect_fill_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_COMPLEMENTARY_WAVE);

    will_return(vesc_serial_get_imu_roll, 0);
    expect_scan_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_PERSONAL_SCAN);

    will_return(vesc_serial_get_imu_roll, 0);
    expect_scan_animation();
    status_leds_start_animation_option(ANIMATION_OPTION_FLOATWHEEL_CLASSIC);

    // Fade-to-black path (also covers ANIMATION_OPTION_NONE elsewhere).
    will_return(vesc_serial_get_imu_roll, 0);
    expect_any(fade_animation_setup, buffer);
    expect_value(fade_animation_setup, period, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT);
    expect_value(fade_animation_setup, callback, NULL);
    expect_function_call(fade_animation_setup);
    will_return(fade_animation_setup, 1U);
    status_leds_start_animation_option(ANIMATION_OPTION_NONE);

    // Out-of-range option -> fault.
    will_return(vesc_serial_get_imu_roll, 0);
    expect_value(fault, fault, EMERGENCY_FAULT_INVALID_STATE);
    status_leds_start_animation_option(ANIMATION_OPTION_COUNT);
}

/**
 * @brief display_battery()'s three bands: normal (white gauge), low
 * (orange gauge), and critical (red flash, only re-started if not
 * already the active animation).
 */
static void test_display_battery_bands(void **state)
{
    // Normal: white gauge bar. display_gauge_bar() always stops any
    // current animation before starting the scan.
    expect_function_call(stop_animation);
    expect_scan_animation();
    display_battery(900); // 90.0%, above LOW_BATTERY_THRESHOLD

    // Low: orange gauge bar (same call shape, different color - not
    // independently observable via the mock, so just confirm it doesn't
    // crash/misdispatch).
    expect_function_call(stop_animation);
    expect_scan_animation();
    display_battery(100); // 10.0%, at/below LOW_BATTERY_THRESHOLD (150)

    // Critical, animation not already running: starts a new flash.
    // get_animation_id()/stop_animation() use expect_function_call_any()
    // here rather than expect_function_call(), since mixing this test's
    // first-ever use of get_animation_id into the same strict
    // cross-symbol call-ordering queue as the many stop_animation/
    // scan_animation_setup/fill_animation_setup calls above isn't worth
    // the fragility - the return value and the fact that it's called are
    // what matter for this branch, not its exact position.
    expect_function_call_any(get_animation_id);
    will_return(get_animation_id, 0U);
    expect_function_call_any(stop_animation);
    expect_any(fill_animation_setup, buffer);
    expect_any(fill_animation_setup, color_mode);
    expect_any(fill_animation_setup, brightness_mode);
    expect_any(fill_animation_setup, fill_mode);
    expect_any(fill_animation_setup, first_led);
    expect_any(fill_animation_setup, last_led);
    expect_any(fill_animation_setup, hue_min);
    expect_any(fill_animation_setup, hue_max);
    expect_any(fill_animation_setup, color_speed);
    expect_any(fill_animation_setup, brightness_min);
    expect_any(fill_animation_setup, brightness_max);
    expect_any(fill_animation_setup, brightness_speed);
    expect_any(fill_animation_setup, brightness_sequence);
    expect_any(fill_animation_setup, rgb);
    expect_function_call(fill_animation_setup);
    will_return(fill_animation_setup, 42U);
    display_battery(30); // 3.0%, at/below CRITICAL_BATTERY_THRESHOLD (50)

    // Critical again, animation already running (matches the id just
    // returned above) - must NOT restart it. get_animation_id was already
    // registered as expect_function_call_any() above, which covers any
    // number of calls for the rest of this test - registering it again
    // here would leave that duplicate permanently unconsumed instead.
    will_return(get_animation_id, 42U);
    display_battery(30);
}

static void test_display_duty_cycle_bands(void **state)
{
    // Below the danger threshold: green gauge.
    expect_function_call(stop_animation);
    expect_scan_animation();
    display_duty_cycle(500); // 50.0%

    // At/above the danger threshold: red gauge.
    expect_function_call(stop_animation);
    expect_scan_animation();
    display_duty_cycle(950); // 95.0%
}

static void test_display_footpad_variants(void **state)
{
    expect_function_call(stop_animation);
    expect_function_call(status_leds_hw_refresh);
    display_footpad(LEFT_FOOTPAD);

    expect_function_call(stop_animation);
    expect_function_call(status_leds_hw_refresh);
    display_footpad(RIGHT_FOOTPAD);

    expect_function_call(stop_animation);
    expect_function_call(status_leds_hw_refresh);
    display_footpad(LEFT_FOOTPAD | RIGHT_FOOTPAD);

    // No footpad case (default: in the switch) - still clears and
    // refreshes, just doesn't add any color on top.
    expect_function_call(stop_animation);
    expect_function_call(status_leds_hw_refresh);
    display_footpad(NONE_FOOTPAD);
}

/**
 * @brief EVENT_HANDLER(status_leds, command)'s TOGGLE_BEEPER branch.
 */
static void test_command_toggle_beeper(void **state)
{
    // enable_beep currently true (test_status_leds_setup default is
    // unset/false actually - force it explicitly for clarity).
    settings->enable_beep = true;

    event_data_t data = {0};
    // !enable_beep is false -> no-op branch.
    event_queue_call_mocked_callback(EVENT_COMMAND_TOGGLE_BEEPER, &data);

    // !enable_beep is true -> fade to red then disable.
    settings->enable_beep = false;
    expect_any(fade_animation_setup, buffer);
    expect_value(fade_animation_setup, period, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT);
    expect_not_value(fade_animation_setup, callback, NULL);
    expect_function_call(fade_animation_setup);
    will_return(fade_animation_setup, 1U);
    event_queue_call_mocked_callback(EVENT_COMMAND_TOGGLE_BEEPER, &data);
}

const struct CMUnitTest status_leds_tests[] = {
    cmocka_unit_test_setup(test_status_leds_off, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_set_color, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_boot, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_fault, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_toggle, test_status_leds_setup),
    cmocka_unit_test_setup(test_status_leds_idle_dozing, test_status_leds_setup),
    cmocka_unit_test_setup(test_start_animation_option_dispatches_every_option, test_status_leds_setup),
    cmocka_unit_test_setup(test_display_battery_bands, test_status_leds_setup),
    cmocka_unit_test_setup(test_display_duty_cycle_bands, test_status_leds_setup),
    cmocka_unit_test_setup(test_display_footpad_variants, test_status_leds_setup),
    cmocka_unit_test_setup(test_command_toggle_beeper, test_status_leds_setup),
};

#endif // TEST_STATUS_LEDS_H
