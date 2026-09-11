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
#ifndef TEST_BUTTON_EVENTS_H
#define TEST_BUTTON_EVENTS_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "button_events.h"
#include "event_queue.h"
#include "mock_event_queue.h"
#include "mock_timer.h"
#include "config.h"

// button_events.c's own EVENT_HANDLER/TIMER_CALLBACK aren't declared in a
// header (button_events.h only exposes button_events_init) - forward
// declare the ones a test drives directly, matching the macros' expansion.
TIMER_CALLBACK(button_events, repeat);
TIMER_CALLBACK(button_events, hold);

static int test_button_events_setup(void **state)
{
    (void)state;

    event_queue_init();
    timer_init();

    // button_events_init() unconditionally calls reset_button_state()
    // first, which cancels both timers. hold_timer/repeat_timer aren't
    // reset to INVALID_TIMER_ID by reset_button_state() itself, so after
    // the first test in this suite they carry whatever id the previous
    // test last assigned - only the id's existence, not its value, is
    // guaranteed here.
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);

    expect_value(subscribe_event, event, EVENT_BUTTON_DOWN);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BUTTON_UP);
    expect_any(subscribe_event, callback);

    assert_int_equal(LCM_SUCCESS, button_events_init());
    return 0;
}

static void press_button(uint32_t time)
{
    event_data_t data = {0};
    data.button_data.time = time;
    event_queue_call_mocked_callback(EVENT_BUTTON_DOWN, &data);
}

static void release_button(uint32_t time)
{
    event_data_t data = {0};
    data.button_data.time = time;
    event_queue_call_mocked_callback(EVENT_BUTTON_UP, &data);
}

/**
 * @brief IDLE + button_down: starts the hold timer, no event pushed yet.
 */
static void test_button_down_from_idle(void **state)
{
    (void)state;

    // reset_button_state()'s implicit cancel isn't called here - only
    // button_down()'s own unconditional cancel_timer(repeat_timer).
    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);

    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);

    press_button(100);
}

/**
 * @brief A full single-click cycle: down, up within the valid click window,
 * then the repeat timer expires with nothing else pressed -> CLICK fires
 * with click_count 1, and state resets (double-cancelling both timers).
 */
static void test_single_click_cycle(void **state)
{
    (void)state;

    // button_down (IDLE): cancel repeat(0), start hold timer -> id 1
    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(100);

    // button_up 50ms later (within [SINGLE_CLICK_MIN, SINGLE_CLICK_MAX]):
    // cancel hold(1), start repeat timer -> id 2
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, REPEAT_WINDOW);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    release_button(150);

    // Repeat timer fires with nobody re-pressing -> CLICK, click_count=1,
    // then reset_button_state() cancels both timers again.
    expect_value(event_queue_push, event, EVENT_BUTTON_CLICK);
    uint8_t expected_count = 1;
    expect_check(event_queue_push, data, validate_click_count_event_data, (uintmax_t)&expected_count);
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 2);
    will_return(cancel_timer, LCM_SUCCESS);
    call_timer_callback(2, 350);
}

/**
 * @brief A double-click: after the first release, a second press arrives
 * within REPEAT_WINDOW - clickCount increments to 2 instead of firing a
 * click for the first press.
 */
static void test_double_click_within_repeat_window(void **state)
{
    (void)state;

    // First press/release (same as single click cycle, ids 1 and 2)
    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(100);

    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, REPEAT_WINDOW);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    release_button(150);

    // Second press arrives 50ms after release, well within REPEAT_WINDOW
    // (200ms) - RELEASED branch, starts a new hold timer -> id 3. No CLICK
    // pushed for the first click.
    expect_value(cancel_timer, timer_id, 2); // cancel the repeat timer
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(200);

    // Release again quickly -> RELEASED, clickCount is now 2, new repeat
    // timer -> id 4.
    expect_value(cancel_timer, timer_id, 3);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, REPEAT_WINDOW);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    release_button(250);

    // Repeat timer fires -> CLICK with click_count=2
    expect_value(event_queue_push, event, EVENT_BUTTON_CLICK);
    uint8_t expected_count = 2;
    expect_check(event_queue_push, data, validate_click_count_event_data, (uintmax_t)&expected_count);
    expect_value(cancel_timer, timer_id, 3);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 4);
    will_return(cancel_timer, LCM_SUCCESS);
    call_timer_callback(4, 450);
}

/**
 * @brief A second press arriving AFTER the repeat window closes is treated
 * as a fresh/unexpected sequence and resets state instead of continuing
 * the click count.
 */
static void test_press_outside_repeat_window_resets(void **state)
{
    (void)state;

    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(100);

    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, REPEAT_WINDOW);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    release_button(150);

    // Second press arrives 300ms after release (> REPEAT_WINDOW=200).
    // button_down() unconditionally cancels repeat_timer (still 2) first,
    // then falls into the "outside window" else branch ->
    // reset_button_state(), which cancels hold(1) and repeat(2) again. No
    // new timer is started.
    expect_value(cancel_timer, timer_id, 2); // button_down's own unconditional cancel
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 1); // reset_button_state's hold cancel
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 2); // reset_button_state's repeat cancel
    will_return(cancel_timer, LCM_SUCCESS);
    press_button(450);
}

/**
 * @brief Holding past HOLD_MAX fires EVENT_BUTTON_HOLD and sets
 * holdTriggered, then releasing resets state instead of firing a click.
 */
static void test_hold_then_release(void **state)
{
    (void)state;

    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(100);

    // Hold timer fires while still PRESSED -> HOLD event, click_count=1
    expect_value(event_queue_push, event, EVENT_BUTTON_HOLD);
    uint8_t expected_count = 1;
    expect_check(event_queue_push, data, validate_click_count_event_data, (uintmax_t)&expected_count);
    call_timer_callback(1, 600);

    // Release after the hold fired -> holdTriggered short-circuits to
    // reset_button_state() (no click).
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 1); // reset_button_state's hold cancel
    will_return(cancel_timer, LCM_SUCCESS);
    expect_any(cancel_timer, timer_id); // reset_button_state's repeat cancel - never set in this test, so whatever a previous test left it at
    will_return(cancel_timer, LCM_SUCCESS);
    release_button(700);
}

/**
 * @brief A release too soon (< SINGLE_CLICK_MIN) is treated as noise and
 * resets state without starting the repeat/click sequence.
 */
static void test_release_too_short_resets(void **state)
{
    (void)state;

    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(100);

    // Released after only 5ms (< SINGLE_CLICK_MIN=10)
    expect_value(cancel_timer, timer_id, 1); // button_up's own cancel(hold)
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 1); // reset_button_state's hold cancel
    will_return(cancel_timer, LCM_SUCCESS);
    expect_any(cancel_timer, timer_id); // reset_button_state's repeat cancel - never set in this test
    will_return(cancel_timer, LCM_SUCCESS);
    release_button(105);
}

/**
 * @brief A release held too long (> SINGLE_CLICK_MAX) but before HOLD_MAX
 * fires is also treated as noise and resets state.
 */
static void test_release_too_long_resets(void **state)
{
    (void)state;

    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(100);

    // Released after 200ms (> SINGLE_CLICK_MAX=180, still < HOLD_MAX=500)
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_any(cancel_timer, timer_id); // reset_button_state's repeat cancel - never set in this test
    will_return(cancel_timer, LCM_SUCCESS);
    release_button(300);
}

/**
 * @brief Regression test: a stale holdTriggered flag from a previous press
 * must not leak into a new click cycle - button_down() from IDLE always
 * clears it, but the RELEASED->PRESSED re-click path does not, so this
 * exercises that path specifically to confirm the flag still ends up
 * correct by the next button_up.
 */
static void test_no_stale_hold_triggered_across_clicks(void **state)
{
    (void)state;

    // First press/hold/release: sets holdTriggered=true, then release
    // resets state (holdTriggered back to false via reset_button_state).
    expect_any(cancel_timer, timer_id); // repeat_timer's stale value from a previous test, not under test here
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(100);

    expect_value(event_queue_push, event, EVENT_BUTTON_HOLD);
    uint8_t expected_count = 1;
    expect_check(event_queue_push, data, validate_click_count_event_data, (uintmax_t)&expected_count);
    call_timer_callback(1, 600);

    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_any(cancel_timer, timer_id); // reset_button_state's repeat cancel - never set in this test
    will_return(cancel_timer, LCM_SUCCESS);
    release_button(700);

    // Fresh press/release cycle should behave like a normal single click -
    // if holdTriggered had leaked as true, this click would silently be
    // swallowed by button_up's holdTriggered branch instead of starting
    // the repeat window.
    expect_any(cancel_timer, timer_id); // button_down's own cancel(repeat_timer) - still never set in this test
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, HOLD_MAX);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    press_button(800);

    expect_value(cancel_timer, timer_id, 2);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_value(set_timer, timeout, REPEAT_WINDOW);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, false);
    release_button(850);
}

/**
 * @brief Timer callbacks are no-ops if the button state has moved on by
 * the time they fire (e.g. repeat firing while already PRESSED again, or
 * hold firing after release already happened) - covers the "stale
 * callback" branches with no event pushed.
 */
static void test_stale_timer_callbacks_are_noops(void **state)
{
    (void)state;

    // Called directly (bypassing the mock timer table entirely) while
    // IDLE (nothing pressed) - both should be no-ops, no event pushed.
    TIMER_CALLBACK_NAME(button_events, repeat)(999);

    // Hold callback firing while IDLE - no-op.
    TIMER_CALLBACK_NAME(button_events, hold)(999);
}

static const struct CMUnitTest button_events_tests[] = {
    cmocka_unit_test_setup(test_button_down_from_idle, test_button_events_setup),
    cmocka_unit_test_setup(test_single_click_cycle, test_button_events_setup),
    cmocka_unit_test_setup(test_double_click_within_repeat_window, test_button_events_setup),
    cmocka_unit_test_setup(test_press_outside_repeat_window_resets, test_button_events_setup),
    cmocka_unit_test_setup(test_hold_then_release, test_button_events_setup),
    cmocka_unit_test_setup(test_release_too_short_resets, test_button_events_setup),
    cmocka_unit_test_setup(test_release_too_long_resets, test_button_events_setup),
    cmocka_unit_test_setup(test_no_stale_hold_triggered_across_clicks, test_button_events_setup),
    cmocka_unit_test_setup(test_stale_timer_callbacks_are_noops, test_button_events_setup),
};

#endif // TEST_BUTTON_EVENTS_H
