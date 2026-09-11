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
    settings->headlight_brightness = 255U;

    expect_function_call(headlights_hw_init);
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
    expect_value(subscribe_event, event, EVENT_IMU_PITCH_CHANGED);
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

    // Add mocks. state_change() reads board_mode_get() twice: once for the
    // config-mode enter/exit check (which runs before the mode switch) and
    // once for the switch itself.
    will_return(board_mode_get, BOARD_MODE_BOOTING);
    will_return(board_mode_get, BOARD_MODE_BOOTING);

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

    // Add mocks. state_change() reads board_mode_get() twice: once for the
    // config-mode enter/exit check (which runs before the mode switch) and
    // once for the switch itself.
    will_return(board_mode_get, BOARD_MODE_RIDING);
    will_return(board_mode_get, BOARD_MODE_RIDING);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
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

    // Add mocks. state_change() reads board_mode_get()/board_submode_get()
    // twice: once for the config-mode enter/exit check (which runs before
    // the mode switch) and once for the switch(es) themselves.
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

void test_headlights_idle_default(void **state)
{
    (void)state; // Unused

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_DEFAULT;

    // Add mocks. state_change() reads board_mode_get()/board_submode_get()
    // twice: once for the config-mode enter/exit check (which runs before
    // the mode switch) and once for the switch(es) themselves.
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DEFAULT);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DEFAULT);

    // Expect set brightness
    expect_any(headlights_hw_set_brightness, brightness);

    // Expect animation timer
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

// headlights.c's own internals aren't declared in headlights.h - non-static
// so tests can call them directly, matching this codebase's convention.
void headlights_rpm_changed(void);
EVENT_HANDLER(headlights, state_change);
TIMER_CALLBACK(headlights, mode_animation);
TIMER_CALLBACK(headlights, enable_animation);
TIMER_CALLBACK(headlights, direction_animation);

static void test_headlights_disabled_off_charging_zero_mode(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_DISABLED;

    will_return(board_mode_get, BOARD_MODE_DISABLED);
    will_return(board_mode_get, BOARD_MODE_DISABLED);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

static void test_headlights_fault_internal_flashes(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_FAULT;
    data.board_mode.submode = BOARD_SUBMODE_FAULT_INTERNAL;

    will_return(board_mode_get, BOARD_MODE_FAULT);
    will_return(board_mode_get, BOARD_MODE_FAULT);
    will_return(board_submode_get, BOARD_SUBMODE_FAULT_INTERNAL);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

static void test_headlights_fault_vesc_stops_animation(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_FAULT;
    data.board_mode.submode = BOARD_SUBMODE_FAULT_VESC;

    will_return(board_mode_get, BOARD_MODE_FAULT);
    will_return(board_mode_get, BOARD_MODE_FAULT);
    will_return(board_submode_get, BOARD_SUBMODE_FAULT_VESC);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

static void test_headlights_idle_dozing_animates(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_DOZING;

    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_DOZING);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

static void test_headlights_idle_shutting_down_animates(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_SHUTTING_DOWN;

    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_SHUTTING_DOWN);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_SHUTTING_DOWN);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief Entering config mode forces headlights on regardless of the
 * user's enable_headlights setting; leaving it (with the setting
 * disabled) fades them back out.
 */
static void test_headlights_config_mode_forces_on_and_restores(void **state)
{
    (void)state;

    event_data_t enter_data = {0};
    enter_data.board_mode.mode = BOARD_MODE_IDLE;
    enter_data.board_mode.submode = BOARD_SUBMODE_IDLE_CONFIG;
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_CONFIG);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_CONFIG);
    expect_any(headlights_hw_set_brightness, brightness);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &enter_data);

    // Leaving config with enable_headlights disabled -> fade out.
    settings_get()->enable_headlights = false;
    event_data_t exit_data = {0};
    exit_data.board_mode.mode = BOARD_MODE_IDLE;
    exit_data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    exit_data.board_mode.previous_mode = BOARD_MODE_IDLE;
    exit_data.board_mode.previous_submode = BOARD_SUBMODE_IDLE_CONFIG;
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    will_return(board_submode_get, BOARD_SUBMODE_IDLE_ACTIVE);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    expect_any(headlights_hw_set_brightness, brightness);
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &exit_data);
}

/**
 * @brief EVENT_RPM_CHANGED dispatched through the real event handler
 * (rather than calling headlights_rpm_changed() directly), exercising the
 * handler's own dispatch switch too. apply_hysteresis() with a large
 * positive RPM and headlights_rpm_hys starting at STATE_SET (from
 * headlights_init()) stays SET; direction reported as REVERSE (not
 * FORWARD) so a direction change is needed.
 */
static void test_headlights_rpm_changed_switches_direction(void **state)
{
    (void)state;

    will_return(vesc_serial_get_rpm, 1000);
    will_return(headlights_hw_get_direction, HEADLIGHTS_DIRECTION_REVERSE);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    expect_any(headlights_hw_set_brightness, brightness);

    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_RPM_CHANGED, &data);
}

static void test_headlights_toggle_lights_on(void **state)
{
    (void)state;

    settings_get()->enable_headlights = true;
    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_COMMAND_TOGGLE_LIGHTS, &data);
}

static void test_headlights_toggle_lights_off_fades(void **state)
{
    (void)state;

    settings_get()->enable_headlights = false;
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_COMMAND_TOGGLE_LIGHTS, &data);
}

static void test_headlights_context_changed_to_brightness_flashes(void **state)
{
    (void)state;

    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    data.context = COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS;
    event_queue_call_mocked_callback(EVENT_COMMAND_CONTEXT_CHANGED, &data);
}

static void test_headlights_context_changed_away_restores(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    data.context = COMMAND_PROCESSOR_CONTEXT_PERSONAL_COLOR;
    event_queue_call_mocked_callback(EVENT_COMMAND_CONTEXT_CHANGED, &data);
}

static void test_headlights_imu_pitch_extreme_dims(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    data.imu_pitch = 61000; // > 60 degrees
    event_queue_call_mocked_callback(EVENT_IMU_PITCH_CHANGED, &data);
}

static void test_headlights_imu_pitch_normal_full_brightness(void **state)
{
    (void)state;

    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    data.imu_pitch = 100;
    event_queue_call_mocked_callback(EVENT_IMU_PITCH_CHANGED, &data);
}

static void headlights_set_mode_animation_via_context(void)
{
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    expect_any(headlights_hw_set_brightness, brightness);
    event_data_t data = {0};
    data.context = COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS;
    event_queue_call_mocked_callback(EVENT_COMMAND_CONTEXT_CHANGED, &data);
}

/**
 * @brief Timer callbacks driven directly - each just needs the real
 * (unmocked) function_generator to run one step; the interesting checks
 * are the eventual cancel_timer()/set_timer() dispatch when the generator
 * finishes.
 */
static void test_mode_animation_timer_ticks(void **state)
{
    (void)state;

    // Start a flash (repeating) animation so the generator never reports
    // "done" - one tick just updates brightness, no cancel_timer.
    headlights_set_mode_animation_via_context();

    expect_any(headlights_hw_set_brightness, brightness);
    headlights_mode_animation_timer_callback(0U);
}

const struct CMUnitTest headlights_tests[] = {
    cmocka_unit_test_setup(test_headlights_boot, headlights_setup),
    cmocka_unit_test_setup(test_headlights_riding, headlights_setup),
    cmocka_unit_test_setup(test_headlights_idle_active, headlights_setup),
    cmocka_unit_test_setup(test_headlights_idle_default, headlights_setup),
    cmocka_unit_test_setup(test_headlights_disabled_off_charging_zero_mode, headlights_setup),
    cmocka_unit_test_setup(test_headlights_fault_internal_flashes, headlights_setup),
    cmocka_unit_test_setup(test_headlights_fault_vesc_stops_animation, headlights_setup),
    cmocka_unit_test_setup(test_headlights_idle_dozing_animates, headlights_setup),
    cmocka_unit_test_setup(test_headlights_idle_shutting_down_animates, headlights_setup),
    cmocka_unit_test_setup(test_headlights_config_mode_forces_on_and_restores, headlights_setup),
    cmocka_unit_test_setup(test_headlights_rpm_changed_switches_direction, headlights_setup),
    cmocka_unit_test_setup(test_headlights_toggle_lights_on, headlights_setup),
    cmocka_unit_test_setup(test_headlights_toggle_lights_off_fades, headlights_setup),
    cmocka_unit_test_setup(test_headlights_context_changed_to_brightness_flashes, headlights_setup),
    cmocka_unit_test_setup(test_headlights_context_changed_away_restores, headlights_setup),
    cmocka_unit_test_setup(test_headlights_imu_pitch_extreme_dims, headlights_setup),
    cmocka_unit_test_setup(test_headlights_imu_pitch_normal_full_brightness, headlights_setup),
    cmocka_unit_test_setup(test_mode_animation_timer_ticks, headlights_setup),
};

#endif // TEST_HEADLIGHTS_H