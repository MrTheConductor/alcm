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
#include "animations.h"

#define NUM_LEDS 10

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
                         0, NUM_LEDS - 1, 0.0f, 360.0f, 1000, 1.0f, 1.0f, 0, 0, NULL);

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

const struct CMUnitTest animations_tests[] = {
    cmocka_unit_test_setup_teardown(fill_test, test_animations_setup, test_animations_teardown),
};
#endif