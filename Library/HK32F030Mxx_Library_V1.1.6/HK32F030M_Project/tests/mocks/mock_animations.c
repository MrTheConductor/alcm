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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "mock_animations.h"
void scan_animation_setup(status_leds_color_t *buffer, scan_direction_t direction,
    color_mode_t color_mode, float movement_speed, float sigma, float hue_min,
    float hue_max, float color_speed,
    scan_start_t scan_start,
    scan_end_t scan_end,
    float init_mu,
    const status_leds_color_t *rgb)
{
    check_expected(buffer);
    check_expected(direction);
    check_expected(color_mode);
    check_expected(movement_speed);
    check_expected(sigma);
    check_expected(hue_min);
    check_expected(hue_max);
    check_expected(color_speed);
    check_expected(scan_start);
    check_expected(scan_end);
    check_expected(init_mu);
    check_expected_ptr(rgb);
    function_called();
}

void fill_animation_setup(status_leds_color_t *buffer, color_mode_t color_mode,
                          brightness_mode_t brightness_mode, fill_mode_t fill_mode,
                          uint8_t first_led, uint8_t last_led, float hue_min, float hue_max,
                          float color_speed, float brightness_min, float brightness_max,
                          float brightness_speed, uint16_t brightness_sequence,
                          const status_leds_color_t *rgb) {
    check_expected(buffer);
    check_expected(color_mode);
    check_expected(brightness_mode);
    check_expected(fill_mode);
    check_expected(first_led);
    check_expected(last_led);
    check_expected(hue_min);
    check_expected(hue_max);
    check_expected(color_speed);
    check_expected(brightness_min);
    check_expected(brightness_max);
    check_expected(brightness_speed);
    check_expected(brightness_sequence);
    check_expected_ptr(rgb);
    function_called();
}

static animation_callback_t mock_callback;

void fade_animation_setup(status_leds_color_t *buffer, uint16_t period,
                          animation_callback_t callback) {
    check_expected(buffer);
    check_expected(period);
    check_expected_ptr(callback);
    function_called();
    mock_callback = callback;
}

void fade_animation_callback(void) {
    if (mock_callback != NULL) {
        mock_callback();
    }
}

void clear_fade_animation_callback(void) {
    mock_callback = NULL;
}

void fire_animation_setup(status_leds_color_t *buffer) {
    check_expected(buffer);
    function_called();
}

void stop_animation(void) {
    function_called();
}

void hsl_to_rgb(float h, float s, float l, status_leds_color_t *color) {
    check_expected(h);
    check_expected(s);
    check_expected(l);
    check_expected_ptr(color);
    function_called();
}
