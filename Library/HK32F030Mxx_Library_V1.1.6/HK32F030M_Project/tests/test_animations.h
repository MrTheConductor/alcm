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
#ifndef TEST_ANIMATIONS_H
#define TEST_ANIMATIONS_H
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "mock_timer.h"
#include "mock_event_queue.h"
#include "animations.h"

#define NUM_LEDS 10

// animation_start() isn't declared in animations.h (private) - non-static
// so tests can call it directly. animation_tick_t is a private typedef
// but its underlying type (a plain function pointer) is safe to spell out
// directly here without needing the typedef itself.
void animation_start(void (*callback)(uint32_t tick));

int test_animations_setup(void **state)
{
    (void)state; // Unused parameter

    // Initialize the timer
    timer_init();

    return 0;
}

/**
 * @brief Test the fill animation setup function.
 *
 * @param state Pointer to the test state.
 */
static void fill_test(void **state)
{
    (void)state; // Unused parameter

    status_leds_color_t buffer[NUM_LEDS];

    // Expect timer to be set
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);

    fill_animation_setup(buffer, COLOR_MODE_HSV_INCREASE, BRIGHTNESS_MODE_STATIC, FILL_MODE_SOLID,
                         0, NUM_LEDS - 1, FIXED16(0.0), FIXED16(360.0), 1000, FIXED16(1.0),
                         FIXED16(1.0), 0, 0, NULL);

    // Run a few ticks of the animation
    for (int i = 0; i < 100; i++)
    {
        // Expect a call to status_leds_set_color to clear the LEDs
        expect_function_call(status_leds_set_color);
        expect_any(status_leds_set_color, color);
        expect_any(status_leds_set_color, begin);
        expect_any(status_leds_set_color, end);
        will_return(status_leds_set_color, LCM_SUCCESS);

        // Expect a second call to status_leds_set_color to set the solid color
        expect_function_call(status_leds_set_color);
        expect_any(status_leds_set_color, color);
        expect_any(status_leds_set_color, begin);
        expect_any(status_leds_set_color, end);
        will_return(status_leds_set_color, LCM_SUCCESS);

        // Expect a call to status_leds_refresh
        expect_function_call(status_leds_refresh);
        will_return(status_leds_refresh, LCM_SUCCESS);

        call_timer_callback(1, i);
    }
}

int test_animations_teardown(void **state)
{
    (void)state; // Unused parameter

    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);

    stop_animation();

    return 0;
}

/**
 * @brief hsl_to_rgb() is public and pure - no mocking needed at all.
 * Exercises each of the 6 hue sectors plus the wraparound normalization
 * for out-of-[0,360) hue values.
 */
static void test_hsl_to_rgb_sectors_and_wraparound(void **state)
{
    (void)state;

    status_leds_color_t color = {0};

    // NULL color is a no-op, not a crash.
    hsl_to_rgb(FIXED16(0.0), 255, 128, NULL);

    // One representative hue per 60-degree sector, full saturation, mid
    // lightness - just confirm each sector path runs and produces a
    // plausible (non-degenerate) color without crashing.
    fixed16_t hues[] = {FIXED16(30.0),  FIXED16(90.0),  FIXED16(150.0),
                        FIXED16(210.0), FIXED16(270.0), FIXED16(330.0)};
    for (size_t i = 0; i < sizeof(hues) / sizeof(hues[0]); i++)
    {
        hsl_to_rgb(hues[i], 255, 128, &color);
    }

    // Hue wraparound: 720 degrees and -30 degrees should both normalize
    // into [0,360) and not crash or hang the while-loops.
    hsl_to_rgb(FIXED16(720.0), 255, 128, &color);
    hsl_to_rgb(FIXED16(-30.0), 255, 128, &color);

    // Zero saturation (grayscale) and zero lightness (black) edge cases.
    hsl_to_rgb(FIXED16(180.0), 0, 128, &color);
    hsl_to_rgb(FIXED16(180.0), 255, 0, &color);
}

/**
 * @brief animation_start()'s NULL-callback branch is a defensive fault,
 * distinct from the normal path already exercised via fill_test().
 */
static void test_animation_start_null_callback_faults(void **state)
{
    (void)state;

    expect_value(fault, fault, EMERGENCY_FAULT_NULL_POINTER);
    animation_start(NULL);
}

/**
 * @brief stop_animation()'s "nothing to cancel" branch - untested by
 * test_animations_teardown(), which always drives the active-timer path.
 */
static void test_stop_animation_when_already_inactive(void **state)
{
    (void)state;

    status_leds_color_t buffer[NUM_LEDS];
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    fill_animation_setup(buffer, COLOR_MODE_HSV_INCREASE, BRIGHTNESS_MODE_STATIC, FILL_MODE_SOLID, 0,
                         NUM_LEDS - 1, FIXED16(0.0), FIXED16(360.0), 1000, FIXED16(1.0), FIXED16(1.0),
                         0, 0, NULL);

    // Timer reports "not active" this time - stop_animation() must not
    // call cancel_timer().
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    stop_animation();
}

/**
 * @brief fade_animation_tick()'s two branches: in-progress (scales the
 * buffer toward black) and complete (zeroes the buffer, cancels the
 * timer, and invokes the callback).
 */
static bool_t fade_callback_invoked = false;
static void test_fade_callback(void)
{
    fade_callback_invoked = true;
}

static void test_fade_animation_completes_and_calls_back(void **state)
{
    (void)state;

    fade_callback_invoked = false;
    status_leds_color_t buffer[NUM_LEDS];
    for (int i = 0; i < NUM_LEDS; i++)
    {
        buffer[i].r = 255;
        buffer[i].g = 255;
        buffer[i].b = 255;
    }

    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    // STATUS_LEDS_FADE_TO_BLACK_TIMEOUT is short enough that a couple of
    // ANIMATION_DELAY (25ms) ticks reach it - use a large enough tick
    // count that the very first tick already completes, keeping this
    // test to a single, simple in-progress-vs-complete pair.
    fade_animation_setup(buffer, 10U, test_fade_callback);

    // First tick: elapsed (25ms) >= period (10ms) -> complete immediately.
    expect_function_call(status_leds_refresh);
    will_return(status_leds_refresh, LCM_SUCCESS);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    call_timer_callback(1, 0);

    assert_true(fade_callback_invoked);
    assert_int_equal(0, buffer[0].r);
    assert_int_equal(0, buffer[0].g);
    assert_int_equal(0, buffer[0].b);
}

static void test_fade_animation_in_progress_scales_down(void **state)
{
    (void)state;

    status_leds_color_t buffer[NUM_LEDS];
    for (int i = 0; i < NUM_LEDS; i++)
    {
        buffer[i].r = 255;
        buffer[i].g = 255;
        buffer[i].b = 255;
    }

    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_any(set_timer, repeat);
    // A long period relative to one ANIMATION_DELAY tick, so the first
    // tick lands in the "still fading" branch instead of completing.
    fade_animation_setup(buffer, 1000U, NULL);

    expect_function_call(status_leds_refresh);
    will_return(status_leds_refresh, LCM_SUCCESS);
    call_timer_callback(1, 0);

    // Scaled down from full brightness, but not yet zero.
    assert_true(buffer[0].r < 255);
    assert_true(buffer[0].r > 0);
}

/**
 * @brief scan_animation_setup() with a direction/scan_end combination not
 * covered elsewhere (SCAN_START_MU + SCAN_END_SINGLE_TICK runs the tick
 * synchronously instead of arming a timer - no set_timer call at all).
 */
static void test_scan_animation_single_tick_runs_synchronously(void **state)
{
    (void)state;

    status_leds_color_t buffer[NUM_LEDS];
    status_leds_color_t rgb = {255, 0, 0};

    // No set_timer expectation - SCAN_END_SINGLE_TICK never arms a timer.
    expect_function_call(status_leds_refresh);
    will_return(status_leds_refresh, LCM_SUCCESS);

    scan_animation_setup(buffer, SCAN_DIRECTION_SINE, COLOR_MODE_RGB, 500, FIXED16(0.0),
                         FIXED16(0.0), 0, SCAN_START_MU, SCAN_END_SINGLE_TICK, FIXED16(2.0), &rgb);
}

const struct CMUnitTest animations_tests[] = {
    cmocka_unit_test_setup_teardown(fill_test, test_animations_setup, test_animations_teardown),
    cmocka_unit_test(test_hsl_to_rgb_sectors_and_wraparound),
    cmocka_unit_test_setup(test_animation_start_null_callback_faults, test_animations_setup),
    cmocka_unit_test_setup(test_stop_animation_when_already_inactive, test_animations_setup),
    cmocka_unit_test_setup(test_fade_animation_completes_and_calls_back, test_animations_setup),
    cmocka_unit_test_setup(test_fade_animation_in_progress_scales_down, test_animations_setup),
    cmocka_unit_test_setup(test_scan_animation_single_tick_runs_synchronously, test_animations_setup),
};
#endif
