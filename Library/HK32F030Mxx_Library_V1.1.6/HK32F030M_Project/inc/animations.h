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
#ifndef ANIMATIONS_H
#define ANIMATIONS_H

// Include hardware-specific definitions and functions for controlling status
// LEDs
#include "status_leds_hw.h"

#define SIGMA_DEFAULT 0.7f      // default sigma value
#define SATURATION_DEFAULT 1.0f // full saturation
#define LIGHTNESS_DEFAULT 0.5f  // half lightness
#define LIGHTNESS_DEFAULT 0.5f  // half lightness

/**
 * @brief Enumeration for different scan directions.
 */
typedef enum
{
    SCAN_DIRECTION_LEFT_TO_RIGHT, // Left to right
    SCAN_DIRECTION_RIGHT_TO_LEFT, // Right to left
    // Mirror the left and right sides
    SCAN_DIRECTION_LEFT_TO_RIGHT_MIRROR,
    SCAN_DIRECTION_RIGHT_TO_LEFT_MIRROR,
    // Fill the area between mu and the edge
    SCAN_DIRECTION_LEFT_TO_RIGHT_FILL,
    SCAN_DIRECTION_RIGHT_TO_LEFT_FILL,
    SCAN_DIRECTION_SINE
} scan_direction_t;

/**
 * @brief Enumeration for different fill modes.
 */
typedef enum
{
    FILL_MODE_SOLID,              // Fill with a solid color
    FILL_MODE_HSV_GRADIENT,       // Fill with a gradient of colors (hue_min to
                                  // hue_max)
    FILL_MODE_HSV_GRADIENT_MIRROR // Fill with a mirrored gradient of colors
                                  // (hue_min to hue_max)
} fill_mode_t;

/**
 * @brief Color animation mode
 */
typedef enum
{
    COLOR_MODE_HSV_INCREASE, // Increase hue over range
    COLOR_MODE_HSV_DECREASE, // Decrease hue over range
    COLOR_MODE_HSV_SINE,     // Sine wave alternating between min/max
    COLOR_MODE_HSV_SQUARE,   // Square wave alternating between min/max
    COLOR_MODE_RGB           // Solid RGB color
} color_mode_t;

/**
 * @brief Brightness animation mode
 */
typedef enum
{
    BRIGHTNESS_MODE_STATIC,  // Static brightness
    BRIGHTNESS_MODE_SINE,    // Sine wave between min and max
    BRIGHTNESS_MODE_FLASH,   // Flash between min and max
    BRIGHTNESS_MODE_FADE,    // Fade from max to min
    BRIGHTNESS_MODE_SEQUENCE // Sequence of brightness values
} brightness_mode_t;

/**
 * @brief Callback function for animations.
 */
typedef void (*animation_callback_t)(void);

/**
 * @brief Set up a scan animation.
 *
 * @param buffer Pointer to the LED buffer.
 * @param direction Direction of the scan animation.
 * @param color_mode Color mode for the animation.
 * @param movement_speed Speed of the movement.
 * @param sigma Sigma value for the animation.
 * @param hue_min Minimum hue value.
 * @param hue_max Maximum hue value.
 * @param color_speed Speed of the color change.
 * @param rgb Pointer to the RGB color.
 */
void scan_animation_setup(status_leds_color_t *buffer, scan_direction_t direction,
                          color_mode_t color_mode, float movement_speed, float sigma, float hue_min,
                          float hue_max, float color_speed, const status_leds_color_t *rgb);

/**
 * @brief Set up a fill animation.
 *
 * @param buffer Pointer to the LED buffer.
 * @param color_mode Color mode for the animation.
 * @param brightness_mode Brightness mode for the animation.
 * @param fill_mode Fill mode for the animation.
 * @param first_led Index of the first LED.
 * @param last_led Index of the last LED.
 * @param hue_min Minimum hue value.
 * @param hue_max Maximum hue value.
 * @param color_speed Speed of the color change.
 * @param brightness_min Minimum brightness value.
 */
void fill_animation_setup(status_leds_color_t *buffer, color_mode_t color_mode,
                          brightness_mode_t brightness_mode, fill_mode_t fill_mode,
                          uint8_t first_led, uint8_t last_led, float hue_min, float hue_max,
                          float color_speed, float brightness_min, float brightness_max,
                          float brightness_speed, uint16_t brightness_sequence,
                          const status_leds_color_t *rgb);

/**
void fade_animation_setup(status_leds_color_t *buffer,
                          uint16_t period,
                          animation_callback_t callback);
 * @param buffer Pointer to the LED buffer.
 * @param period Period of the fade animation.
 * @param callback Callback function to be called at the end of the animation.
 */
void fade_animation_setup(status_leds_color_t *buffer, uint16_t period,
                          animation_callback_t callback);

/**
 * @brief Set up a fire animation.
 */
void fire_animation_setup(status_leds_color_t *buffer);

/**
 * @brief Stop the current animation.
 *
 * This function stops any currently running animation and resets the animation
 * state.
 */
void stop_animation(void);

/**
 * @brief Converts HSL color values to RGB color values.
 *
 * This function takes in hue, saturation, and lightness (HSL) values and
 * converts them to red, green, and blue (RGB) values. The hue value is
 * expected to be in the range [0, 360) degrees, while the saturation and
 * lightness values should be in the range [0, 1]. The resulting RGB values
 * are stored in a status_leds_color_t struct with each component scaled
 * to the range [0, 255].
 *
 * @param h The hue component of the color in degrees.
 * @param s The saturation component of the color.
 * @param l The lightness component of the color.
 * @param color Pointer to a status_leds_color_t struct to store the resulting
 * RGB values.
 */
void hsl_to_rgb(float h, float s, float l, status_leds_color_t *color);

#endif
