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
#ifndef TEST_COMMAND_PROCESSOR_H
#define TEST_COMMAND_PROCESSOR_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "command_processor.h"
#include "event_queue.h"
#include "board_mode.h"
#include "footpads.h"
#include "settings.h"
#include "mock_event_queue.h"
#include "mock_timer.h"
#include "mock_settings.h"

// Not declared in command_processor.h - non-static so tests can call them
// directly, matching this codebase's convention for internal-but-linkable
// helpers. command_processor_adjust_setting() is deliberately NOT declared
// here since its adjustment enum is private to the .c file - it's always
// exercised indirectly through context_handler()/the event handlers below.
void command_processor_set_context(command_processor_context_t context);
void command_processor_one_button_navigation(event_type_t event, uint8_t count);
void command_processor_context_handler(event_type_t event, const event_data_t *data);
void command_processor_default_handler(event_type_t event, const event_data_t *data);
EVENT_HANDLER(command_processor, button);
EVENT_HANDLER(command_processor, board_mode);

/**
 * @brief Every test gets a fresh event queue, timer, settings, and a freshly
 * command_processor_init()'d module (current_context reset to DEFAULT).
 * repeat_timer_id/command_processor_fg/animation_setting are NOT reset by
 * init() - tests that care always establish those themselves (e.g. an
 * INCREASE before a STOP) rather than relying on cross-test state.
 */
static int cp_setup(void **state)
{
    (void)state;

    event_queue_init();
    timer_init();
    settings_init(); // mock: resets the fake settings struct to defaults

    expect_value(subscribe_event, event, EVENT_BUTTON_CLICK);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BUTTON_HOLD);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BUTTON_UP);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_FOOTPAD_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);

    assert_int_equal(LCM_SUCCESS, command_processor_init());
    return 0;
}

static void expect_push(event_type_t event)
{
    expect_value(event_queue_push, event, event);
    expect_any(event_queue_push, data);
}

static event_data_t click_count_event(uint8_t count)
{
    event_data_t data = {0};
    data.click_count = count;
    return data;
}

static event_data_t footpad_event(footpads_state_t state)
{
    event_data_t data = {0};
    data.footpads_state = state;
    return data;
}

// ---------------------------------------------------------------------
// command_processor_init()
// ---------------------------------------------------------------------

static void test_init_null_settings_returns_error(void **state)
{
    (void)state;

    mock_settings_force_null_once();
    // No subscribe_event calls expected - init bails out before subscribing.
    assert_int_equal(LCM_ERROR, command_processor_init());
}

// ---------------------------------------------------------------------
// command_processor_one_button_navigation()
// ---------------------------------------------------------------------

static void test_navigation_click_one_advances_context(void **state)
{
    (void)state;

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    expect_push(EVENT_COMMAND_ACK);
    command_processor_one_button_navigation(EVENT_BUTTON_CLICK, 1);
}

static void test_navigation_click_two_wraps_from_zero(void **state)
{
    (void)state;

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS); // index 0
    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    expect_push(EVENT_COMMAND_ACK);
    // From context 0, click-click wraps around to COUNT-1.
    command_processor_one_button_navigation(EVENT_BUTTON_CLICK, 2);
}

static void test_navigation_click_two_decrements(void **state)
{
    (void)state;

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_BOOT_ANIMATION); // index 3
    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    expect_push(EVENT_COMMAND_ACK);
    command_processor_one_button_navigation(EVENT_BUTTON_CLICK, 2);
}

static void test_navigation_click_default_nacks(void **state)
{
    (void)state;

    expect_push(EVENT_COMMAND_NACK);
    command_processor_one_button_navigation(EVENT_BUTTON_CLICK, 5);
}

static void test_navigation_hold_one_requests_config(void **state)
{
    (void)state;

    expect_value(event_queue_push, event, EVENT_COMMAND_MODE_CONFIG);
    expect_any(event_queue_push, data);
    command_processor_one_button_navigation(EVENT_BUTTON_HOLD, 1);
}

static void test_navigation_hold_other_nacks(void **state)
{
    (void)state;

    expect_push(EVENT_COMMAND_NACK);
    command_processor_one_button_navigation(EVENT_BUTTON_HOLD, 2);
}

static void test_navigation_unhandled_event_is_noop(void **state)
{
    (void)state;

    // No expectations set up at all - anything unexpected fails the test.
    command_processor_one_button_navigation(EVENT_FOOTPAD_CHANGED, 0);
}

// ---------------------------------------------------------------------
// command_processor_adjust_setting() via context_handler() - STOP branch
// ---------------------------------------------------------------------

static void test_stop_with_no_active_timer_is_noop(void **state)
{
    (void)state;

    event_data_t hold_data = click_count_event(2);
    event_data_t up_data = {0};

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &hold_data); // INCREASE

    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false); // timer already expired on its own
    command_processor_context_handler(EVENT_BUTTON_UP, &up_data); // STOP: sees inactive, does NOT cancel
}

static void test_stop_cancels_active_timer(void **state)
{
    (void)state;

    event_data_t hold_data = click_count_event(2);
    event_data_t up_data = {0};

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &hold_data); // INCREASE

    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    command_processor_context_handler(EVENT_BUTTON_UP, &up_data); // STOP: cancels
}

// ---------------------------------------------------------------------
// command_processor_adjust_setting() - the context switch
// ---------------------------------------------------------------------

static void test_adjust_brightness_increase(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2); // INCREASE

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_value(set_timer, timeout, 50U); // BRIGHTNESS_INCREMENT_MS
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_adjust_status_bar_brightness_decrease(void **state)
{
    (void)state;

    event_data_t data = click_count_event(3); // DECREASE

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS);
    expect_value(set_timer, timeout, 50U);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_adjust_personal_color_increase(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2); // INCREASE

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_PERSONAL_COLOR);
    expect_value(set_timer, timeout, 20U); // COLOR_INCREMENT_MS
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_adjust_personal_color_decrease(void **state)
{
    (void)state;

    event_data_t data = click_count_event(3); // DECREASE

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_PERSONAL_COLOR);
    expect_value(set_timer, timeout, 20U);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

/**
 * @brief Animation contexts synchronously fire the animation_repeat timer
 * callback once (ACK + SETTINGS_CHANGED) before arming the repeat timer.
 * boot_animation starts at ANIMATION_OPTION_NONE (0) per the mock's
 * defaults, so INCREASE must land on the next option (1).
 */
static void test_adjust_boot_animation_increase(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2); // INCREASE

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_BOOT_ANIMATION);
    expect_push(EVENT_COMMAND_ACK);
    expect_push(EVENT_COMMAND_SETTINGS_CHANGED);
    expect_value(set_timer, timeout, 1000U); // ANIMATION_INCREMENT_MS
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);

    assert_int_equal(ANIMATION_OPTION_NONE + 1, settings_get()->boot_animation);
}

/**
 * @brief Same as above but DECREASE from NONE (0), which must wrap around
 * to ANIMATION_OPTION_COUNT - 1 rather than underflowing.
 */
static void test_adjust_idle_animation_decrease_wraps(void **state)
{
    (void)state;

    event_data_t data = click_count_event(3); // DECREASE

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_IDLE_ANIMATION);
    expect_push(EVENT_COMMAND_ACK);
    expect_push(EVENT_COMMAND_SETTINGS_CHANGED);
    expect_value(set_timer, timeout, 1000U);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);

    assert_int_equal(ANIMATION_OPTION_COUNT - 1, settings_get()->idle_animation);
}

static void test_adjust_dozing_animation_dispatches(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_DOZING_ANIMATION);
    expect_push(EVENT_COMMAND_ACK);
    expect_push(EVENT_COMMAND_SETTINGS_CHANGED);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_adjust_shutdown_animation_dispatches(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_SHUTDOWN_ANIMATION);
    expect_push(EVENT_COMMAND_ACK);
    expect_push(EVENT_COMMAND_SETTINGS_CHANGED);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_adjust_riding_animation_dispatches(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_RIDING_ANIMATION);
    expect_push(EVENT_COMMAND_ACK);
    expect_push(EVENT_COMMAND_SETTINGS_CHANGED);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

/**
 * @brief current_context == DEFAULT falls into adjust_setting()'s switch
 * default: case - a true no-op, nothing pushed, no timer started.
 */
static void test_adjust_default_context_is_noop(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2);

    // cp_setup() already leaves current_context == DEFAULT.
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

// ---------------------------------------------------------------------
// command_processor_context_handler()
// ---------------------------------------------------------------------

static void test_context_handler_click_delegates_to_navigation(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    expect_push(EVENT_COMMAND_ACK);
    command_processor_context_handler(EVENT_BUTTON_CLICK, &data);
}

static void test_context_handler_footpad_none_stops(void **state)
{
    (void)state;

    event_data_t hold_data = click_count_event(2);
    event_data_t footpad_data = footpad_event(NONE_FOOTPAD);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &hold_data); // start a timer

    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    command_processor_context_handler(EVENT_FOOTPAD_CHANGED, &footpad_data);
}

static void test_context_handler_footpad_left_increases(void **state)
{
    (void)state;

    event_data_t data = footpad_event(LEFT_FOOTPAD);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_FOOTPAD_CHANGED, &data);
}

static void test_context_handler_footpad_right_decreases(void **state)
{
    (void)state;

    event_data_t data = footpad_event(RIGHT_FOOTPAD);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_FOOTPAD_CHANGED, &data);
}

/**
 * @brief Both footpads pressed at once matches none of the if/else-if
 * branches in the footpad case - a true no-op, distinct from NONE_FOOTPAD.
 */
static void test_context_handler_footpad_both_is_noop(void **state)
{
    (void)state;

    event_data_t data = footpad_event(LEFT_FOOTPAD | RIGHT_FOOTPAD);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    command_processor_context_handler(EVENT_FOOTPAD_CHANGED, &data);
}

static void test_context_handler_hold_default_nacks(void **state)
{
    (void)state;

    event_data_t data = click_count_event(9);

    expect_push(EVENT_COMMAND_NACK);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_context_handler_unhandled_event_is_noop(void **state)
{
    (void)state;

    event_data_t data = {0};
    command_processor_context_handler(EVENT_BATTERY_LEVEL_CHANGED, &data);
}

// ---------------------------------------------------------------------
// command_processor_default_handler()
// ---------------------------------------------------------------------

static void test_default_handler_click_one_toggles_lights(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);
    bool_t before_headlights = settings_get()->enable_headlights;
    bool_t before_leds = settings_get()->enable_status_leds;

    expect_push(EVENT_COMMAND_TOGGLE_LIGHTS);
    expect_push(EVENT_COMMAND_ACK);
    command_processor_default_handler(EVENT_BUTTON_CLICK, &data);

    assert_int_equal(!before_headlights, settings_get()->enable_headlights);
    assert_int_equal(!before_leds, settings_get()->enable_status_leds);
}

static void test_default_handler_click_two_toggles_beeper(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2);
    bool_t before_beep = settings_get()->enable_beep;

    expect_push(EVENT_COMMAND_TOGGLE_BEEPER);
    expect_push(EVENT_COMMAND_ACK);
    command_processor_default_handler(EVENT_BUTTON_CLICK, &data);

    assert_int_equal(!before_beep, settings_get()->enable_beep);
}

static void test_default_handler_click_default_nacks(void **state)
{
    (void)state;

    event_data_t data = click_count_event(3);

    expect_push(EVENT_COMMAND_NACK);
    command_processor_default_handler(EVENT_BUTTON_CLICK, &data);
}

static void test_default_handler_hold_one_shuts_down(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);

    expect_push(EVENT_COMMAND_SHUTDOWN);
    command_processor_default_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_default_handler_hold_two_requests_config(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2);

    expect_value(event_queue_push, event, EVENT_COMMAND_MODE_CONFIG);
    expect_any(event_queue_push, data);
    command_processor_default_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_default_handler_hold_default_nacks(void **state)
{
    (void)state;

    event_data_t data = click_count_event(9);

    expect_push(EVENT_COMMAND_NACK);
    command_processor_default_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_default_handler_unhandled_event_is_noop(void **state)
{
    (void)state;

    event_data_t data = {0};
    command_processor_default_handler(EVENT_BATTERY_LEVEL_CHANGED, &data);
}

// ---------------------------------------------------------------------
// EVENT_HANDLER(command_processor, button) - the board-mode gate and
// context/default dispatch
// ---------------------------------------------------------------------

static void test_button_handler_ignores_booting(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);

    will_return(board_mode_get, BOARD_MODE_BOOTING);
    // No push expectations - a click in this mode must do nothing at all.
    command_processor_button_event_handler(EVENT_BUTTON_CLICK, &data);
}

static void test_button_handler_ignores_fault(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);

    will_return(board_mode_get, BOARD_MODE_FAULT);
    command_processor_button_event_handler(EVENT_BUTTON_CLICK, &data);
}

static void test_button_handler_ignores_off(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);

    will_return(board_mode_get, BOARD_MODE_OFF);
    command_processor_button_event_handler(EVENT_BUTTON_CLICK, &data);
}

static void test_button_handler_ignores_disabled(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);

    will_return(board_mode_get, BOARD_MODE_DISABLED);
    command_processor_button_event_handler(EVENT_BUTTON_CLICK, &data);
}

static void test_button_handler_dispatches_to_default_handler(void **state)
{
    (void)state;

    event_data_t data = click_count_event(1);

    // cp_setup() leaves current_context == DEFAULT.
    will_return(board_mode_get, BOARD_MODE_IDLE);
    expect_push(EVENT_COMMAND_SHUTDOWN);
    command_processor_button_event_handler(EVENT_BUTTON_HOLD, &data);
}

static void test_button_handler_dispatches_to_context_handler(void **state)
{
    (void)state;

    event_data_t data = click_count_event(2);

    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    will_return(board_mode_get, BOARD_MODE_IDLE);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_button_event_handler(EVENT_BUTTON_HOLD, &data);
}

// ---------------------------------------------------------------------
// EVENT_HANDLER(command_processor, board_mode)
// ---------------------------------------------------------------------

/**
 * @brief The factory-reset gesture: holding the button through the
 * booting->idle transition resets settings and shuts down.
 */
static void test_board_mode_factory_reset_gesture(void **state)
{
    (void)state;

    will_return(button_driver_hw_is_pressed, true);
    expect_function_call(settings_reset);
    expect_push(EVENT_COMMAND_SHUTDOWN);

    event_data_t data = {0};
    data.board_mode.previous_mode = BOARD_MODE_BOOTING;
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    command_processor_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief The same booting->idle transition WITHOUT the button held must
 * fall through to the normal dispatch instead (here: current_context is
 * still DEFAULT and submode isn't IDLE_CONFIG, so it's a plain no-op).
 */
static void test_board_mode_boot_to_idle_without_button_held(void **state)
{
    (void)state;

    will_return(button_driver_hw_is_pressed, false);

    event_data_t data = {0};
    data.board_mode.previous_mode = BOARD_MODE_BOOTING;
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    command_processor_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief Leaving config mode for any reason (while in a non-default
 * context) NACKs, stops any adjustment, and returns to the default
 * context.
 */
static void test_board_mode_leaving_config_resets_context(void **state)
{
    (void)state;

    // repeat_timer_id is a module static not reset between tests by
    // cp_setup(), so establish a known-active timer here first rather
    // than assuming it's still INVALID_TIMER_ID from a previous test.
    event_data_t hold_data = click_count_event(2); // INCREASE
    expect_push(EVENT_COMMAND_CONTEXT_CHANGED);
    command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    command_processor_context_handler(EVENT_BUTTON_HOLD, &hold_data);

    expect_push(EVENT_COMMAND_NACK);
    // adjust_setting(STOP): the timer just started above is still active.
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_push(EVENT_COMMAND_CONTEXT_CHANGED); // from set_context(DEFAULT)

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_RIDING; // anything other than IDLE/CONFIG
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    command_processor_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief Entering config mode from the default context ACKs and switches
 * to the first real context.
 */
static void test_board_mode_entering_config_from_default(void **state)
{
    (void)state;

    // cp_setup() leaves current_context == DEFAULT.
    expect_push(EVENT_COMMAND_ACK);
    expect_push(EVENT_COMMAND_CONTEXT_CHANGED); // from set_context(0)

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_CONFIG;
    command_processor_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief Any other board-mode event (not matching any of the three
 * branches above) is a pure no-op.
 */
static void test_board_mode_other_transitions_are_noop(void **state)
{
    (void)state;

    // cp_setup() leaves current_context == DEFAULT, so this matches none
    // of the three active branches (not the factory-reset gesture, not
    // "leaving config" since context is already DEFAULT, not "entering
    // config" since submode isn't IDLE_CONFIG).
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_RIDING;
    data.board_mode.submode = BOARD_SUBMODE_RIDING_NORMAL;
    command_processor_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
}

static const struct CMUnitTest command_processor_tests[] = {
    cmocka_unit_test_setup(test_init_null_settings_returns_error, cp_setup),
    cmocka_unit_test_setup(test_navigation_click_one_advances_context, cp_setup),
    cmocka_unit_test_setup(test_navigation_click_two_wraps_from_zero, cp_setup),
    cmocka_unit_test_setup(test_navigation_click_two_decrements, cp_setup),
    cmocka_unit_test_setup(test_navigation_click_default_nacks, cp_setup),
    cmocka_unit_test_setup(test_navigation_hold_one_requests_config, cp_setup),
    cmocka_unit_test_setup(test_navigation_hold_other_nacks, cp_setup),
    cmocka_unit_test_setup(test_navigation_unhandled_event_is_noop, cp_setup),
    cmocka_unit_test_setup(test_stop_with_no_active_timer_is_noop, cp_setup),
    cmocka_unit_test_setup(test_stop_cancels_active_timer, cp_setup),
    cmocka_unit_test_setup(test_adjust_brightness_increase, cp_setup),
    cmocka_unit_test_setup(test_adjust_status_bar_brightness_decrease, cp_setup),
    cmocka_unit_test_setup(test_adjust_personal_color_increase, cp_setup),
    cmocka_unit_test_setup(test_adjust_personal_color_decrease, cp_setup),
    cmocka_unit_test_setup(test_adjust_boot_animation_increase, cp_setup),
    cmocka_unit_test_setup(test_adjust_idle_animation_decrease_wraps, cp_setup),
    cmocka_unit_test_setup(test_adjust_dozing_animation_dispatches, cp_setup),
    cmocka_unit_test_setup(test_adjust_shutdown_animation_dispatches, cp_setup),
    cmocka_unit_test_setup(test_adjust_riding_animation_dispatches, cp_setup),
    cmocka_unit_test_setup(test_adjust_default_context_is_noop, cp_setup),
    cmocka_unit_test_setup(test_context_handler_click_delegates_to_navigation, cp_setup),
    cmocka_unit_test_setup(test_context_handler_footpad_none_stops, cp_setup),
    cmocka_unit_test_setup(test_context_handler_footpad_left_increases, cp_setup),
    cmocka_unit_test_setup(test_context_handler_footpad_right_decreases, cp_setup),
    cmocka_unit_test_setup(test_context_handler_footpad_both_is_noop, cp_setup),
    cmocka_unit_test_setup(test_context_handler_hold_default_nacks, cp_setup),
    cmocka_unit_test_setup(test_context_handler_unhandled_event_is_noop, cp_setup),
    cmocka_unit_test_setup(test_default_handler_click_one_toggles_lights, cp_setup),
    cmocka_unit_test_setup(test_default_handler_click_two_toggles_beeper, cp_setup),
    cmocka_unit_test_setup(test_default_handler_click_default_nacks, cp_setup),
    cmocka_unit_test_setup(test_default_handler_hold_one_shuts_down, cp_setup),
    cmocka_unit_test_setup(test_default_handler_hold_two_requests_config, cp_setup),
    cmocka_unit_test_setup(test_default_handler_hold_default_nacks, cp_setup),
    cmocka_unit_test_setup(test_default_handler_unhandled_event_is_noop, cp_setup),
    cmocka_unit_test_setup(test_button_handler_ignores_booting, cp_setup),
    cmocka_unit_test_setup(test_button_handler_ignores_fault, cp_setup),
    cmocka_unit_test_setup(test_button_handler_ignores_off, cp_setup),
    cmocka_unit_test_setup(test_button_handler_ignores_disabled, cp_setup),
    cmocka_unit_test_setup(test_button_handler_dispatches_to_default_handler, cp_setup),
    cmocka_unit_test_setup(test_button_handler_dispatches_to_context_handler, cp_setup),
    cmocka_unit_test_setup(test_board_mode_factory_reset_gesture, cp_setup),
    cmocka_unit_test_setup(test_board_mode_boot_to_idle_without_button_held, cp_setup),
    cmocka_unit_test_setup(test_board_mode_leaving_config_resets_context, cp_setup),
    cmocka_unit_test_setup(test_board_mode_entering_config_from_default, cp_setup),
    cmocka_unit_test_setup(test_board_mode_other_transitions_are_noop, cp_setup),
};

#endif // TEST_COMMAND_PROCESSOR_H
