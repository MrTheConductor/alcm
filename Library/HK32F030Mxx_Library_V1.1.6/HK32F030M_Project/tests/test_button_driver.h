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
#ifndef TEST_BUTTON_DRIVER_H
#define TEST_BUTTON_DRIVER_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "button_driver.h"
#include "button_driver_hw.h"
#include "event_queue.h"
#include "mock_event_queue.h"
#include "mock_timer.h"

// button_driver.c's own EVENT_HANDLER/TIMER_CALLBACK aren't declared in a
// header (button_driver.h only exposes button_driver_init) - forward
// declare them so tests can drive the handler bodies directly, matching
// the macros' expansion.
EVENT_HANDLER(button_driver, wakeup);
TIMER_CALLBACK(button_driver, debounce);

/**
 * @brief Verifies button_driver_init() wires up the hardware and
 * subscribes to EVENT_BUTTON_WAKEUP. Must run before the other tests only
 * in the sense that it's the natural first test - it doesn't touch the
 * debounce state machine's statics at all, so ordering relative to the
 * other two tests doesn't matter for correctness, only for narrative flow.
 */
static void test_button_driver_init(void **state)
{
    (void)state;

    event_queue_init();

    expect_function_call(button_driver_hw_init);
    expect_value(subscribe_event, event, EVENT_BUTTON_WAKEUP);
    expect_any(subscribe_event, callback);

    assert_int_equal(LCM_SUCCESS, button_driver_init());
}

/**
 * @brief The wakeup handler starts the 1ms debounce timer only if one
 * isn't already active - covers both branches. This runs before the
 * debounce state machine test below and leaves debounce_timer_id set to 1,
 * which that test relies on for its cancel_timer expectation.
 */
static void test_button_driver_wakeup_starts_timer_once(void **state)
{
    (void)state;

    event_data_t data = {0};

    // debounce_timer_id is still INVALID_TIMER_ID (0) at this point -
    // nothing else in this suite has started a timer yet.
    expect_value(is_timer_active, timer_id, INVALID_TIMER_ID);
    will_return(is_timer_active, false);
    expect_value(set_timer, timeout, 1);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    button_driver_wakeup_event_handler(EVENT_BUTTON_WAKEUP, &data);

    // A second wakeup while the debounce timer (id 1) is still active must
    // not start a second one.
    expect_value(is_timer_active, timer_id, 1);
    will_return(is_timer_active, true);
    button_driver_wakeup_event_handler(EVENT_BUTTON_WAKEUP, &data);
}

/**
 * @brief Drives the debounce state machine through every branch, starting
 * from the module's true process-start static state (button_state and
 * last_button_state both BUTTON_STATE_NONE, last_event EVENT_NULL,
 * last_debounce_time 0) - nothing before this test touches those statics.
 * debounce_timer_id is 1, left over from the wakeup test above.
 */
static void test_button_driver_debounce_state_machine(void **state)
{
    (void)state;

    // 1) First reading ever (RELEASED) differs from the initial
    //    last_button_state (NONE) - just resets the debounce clock, no
    //    commit, no event.
    will_return(button_driver_hw_is_pressed, false);
    button_driver_debounce_timer_callback(10);

    // 2) Same reading, only 2ms later (<= 5ms DEBOUNCE_PERIOD) - not
    //    committed yet.
    will_return(button_driver_hw_is_pressed, false);
    button_driver_debounce_timer_callback(12);

    // 3) Same reading, now 10ms after the clock was last reset (step 1) ->
    //    commits to RELEASED for the first time. Since RELEASED is the
    //    "button up" resting state and last_event starts as EVENT_NULL
    //    (never UP), this fires EVENT_BUTTON_UP and cancels the debounce
    //    timer (id 1, from the wakeup test).
    will_return(button_driver_hw_is_pressed, false);
    expect_value(event_queue_push, event, EVENT_BUTTON_UP);
    expect_any(event_queue_push, data);
    expect_value(cancel_timer, timer_id, 1);
    will_return(cancel_timer, LCM_SUCCESS);
    button_driver_debounce_timer_callback(20);

    // 4) Same reading, still committed RELEASED, well past the debounce
    //    window again - last_event is already EVENT_BUTTON_UP, so this
    //    must NOT push a duplicate event.
    will_return(button_driver_hw_is_pressed, false);
    button_driver_debounce_timer_callback(25);

    // 5) Reading flips to PRESSED - differs from last_button_state
    //    (RELEASED), resets the debounce clock again, no commit yet.
    will_return(button_driver_hw_is_pressed, true);
    button_driver_debounce_timer_callback(30);

    // 6) Boundary: exactly DEBOUNCE_PERIOD (5ms) since the clock reset in
    //    step 5. The check is strict '>', so this must NOT commit - no
    //    event expectation is set up here, so cmocka fails loudly if the
    //    code wrongly fires EVENT_BUTTON_DOWN at exactly the boundary.
    will_return(button_driver_hw_is_pressed, true);
    button_driver_debounce_timer_callback(35);

    // 7) One tick past the boundary (6ms elapsed) -> commits to PRESSED
    //    for the first time. last_event is EVENT_BUTTON_UP (from step 3),
    //    so this fires EVENT_BUTTON_DOWN. The PRESSED commit path doesn't
    //    cancel any timer (only the RELEASED path does).
    will_return(button_driver_hw_is_pressed, true);
    expect_value(event_queue_push, event, EVENT_BUTTON_DOWN);
    expect_any(event_queue_push, data);
    button_driver_debounce_timer_callback(36);

    // 8) Same reading, still committed PRESSED, well past the debounce
    //    window - last_event is already EVENT_BUTTON_DOWN, so no
    //    duplicate event (symmetric to step 4).
    will_return(button_driver_hw_is_pressed, true);
    button_driver_debounce_timer_callback(40);
}

static const struct CMUnitTest button_driver_tests[] = {
    cmocka_unit_test(test_button_driver_init),
    cmocka_unit_test(test_button_driver_wakeup_starts_timer_once),
    cmocka_unit_test(test_button_driver_debounce_state_machine),
};

#endif // TEST_BUTTON_DRIVER_H
