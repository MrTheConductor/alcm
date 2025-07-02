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
 * @brief Options for terminating the scan animation.
 */
typedef enum
{
    SCAN_END_NEVER,       // Never stop: run continuously
    SCAN_END_SINGLE_TICK, // Terminate after a single tick
    SCAN_END_MAX_MU       // Terminate at max mu
} scan_end_t;

/**
 * @brief Options for starting mu postion.
 */
typedef enum
{
    SCAN_START_DEFAULT, // Default: start at edge of LED array
    SCAN_START_MU       // Start at arbitrary mu value
} scan_start_t;

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
 * @brief Sets up a scan animation.
 *
 * This function initializes a scan animation with the specified parameters.
 * The animation involves scanning LEDs in a specified direction with configurable
 * color and movement properties.
 *
 * @param buffer Pointer to the LED buffer to be used for the animation.
 * @param direction The direction of the scan (e.g., left-to-right, right-to-left).
 * @param color_mode The color animation mode (e.g., HSV increase, RGB).
 * @param movement_speed The speed of the scan movement.
 * @param sigma The sigma value for the animation (affects spread or intensity).
 * @param hue_min The minimum hue value for the color range.
 * @param hue_max The maximum hue value for the color range.
 * @param color_speed The speed of the color change.
 * @param scan_start The starting position of the scan (e.g., default, arbitrary mu).
 * @param scan_end The condition for ending the scan (e.g., never, single tick).
 * @param init_mu The initial mu value for the scan.
 * @param rgb Pointer to an RGB color structure for solid color mode.
 *
 * @return The ID of the created animation.
 */
uint16_t scan_animation_setup(status_leds_color_t *buffer, scan_direction_t direction,
                              color_mode_t color_mode, float movement_speed, float sigma,
                              float hue_min, float hue_max, float color_speed,
                              scan_start_t scan_start, scan_end_t scan_end, float init_mu,
                              const status_leds_color_t *rgb);

/**
 * @brief Sets up the fill animation for status LEDs with specified parameters.
 *
 * This function configures the fill animation for a range of LEDs, allowing
 * customization of color, brightness, and animation behavior.
 *
 * @param buffer               Pointer to the buffer where the LED colors will be stored.
 * @param color_mode           The mode of color generation (e.g., static, dynamic).
 * @param brightness_mode      The mode of brightness control (e.g., constant, pulsing).
 * @param fill_mode            The mode of filling LEDs (e.g., sequential, random).
 * @param first_led            Index of the first LED in the range to be animated.
 * @param last_led             Index of the last LED in the range to be animated.
 * @param hue_min              Minimum hue value for the color range (0.0 to 1.0).
 * @param hue_max              Maximum hue value for the color range (0.0 to 1.0).
 * @param color_speed          Speed of color transitions.
 * @param brightness_min       Minimum brightness value (0.0 to 1.0).
 * @param brightness_max       Maximum brightness value (0.0 to 1.0).
 * @param brightness_speed     Speed of brightness transitions.
 * @param brightness_sequence  Sequence pattern for brightness changes.
 * @param rgb                  Pointer to a constant color structure for static RGB values.
 *
 * @return The number of LEDs configured for the animation.
 */
uint16_t fill_animation_setup(status_leds_color_t *buffer, color_mode_t color_mode,
                              brightness_mode_t brightness_mode, fill_mode_t fill_mode,
                              uint8_t first_led, uint8_t last_led, float hue_min, float hue_max,
                              float color_speed, float brightness_min, float brightness_max,
                              float brightness_speed, uint16_t brightness_sequence,
                              const status_leds_color_t *rgb);

/**
 * @brief Sets up a fade animation for status LEDs.
 *
 * This function initializes a fade animation with the specified parameters,
 * allowing the LEDs to transition smoothly between colors over a given period.
 *
 * @param buffer Pointer to a buffer of type `status_leds_color_t` that holds
 *               the color data for the animation.
 * @param period The duration of the fade animation in milliseconds.
 * @param callback A callback function of type `animation_callback_t` that will
 *                 be invoked when the animation completes or requires updates.
 * 
 * @return The number of steps or frames required for the fade animation.
 */
uint16_t fade_animation_setup(status_leds_color_t *buffer, uint16_t period,
                              animation_callback_t callback);

#ifdef ENABLE_FIRE_ANIMATION
/**
 * @brief Sets up the fire animation on the status LEDs.
 * 
 * This function initializes the fire animation by configuring the provided
 * buffer with the necessary color data for the status LEDs.
 * 
 * @param buffer Pointer to a buffer of type `status_leds_color_t` where the
 *               animation color data will be stored.
 * 
 * @return A 16-bit unsigned integer indicating the result of the setup process.
 *         The specific meaning of the return value depends on the implementation.
 */
uint16_t fire_animation_setup(status_leds_color_t *buffer);
#endif

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

/**
 * @brief Retrieves the ID of the current animation.
 * 
 * This function returns a 16-bit unsigned integer representing the unique
 * identifier of the currently active animation. The ID can be used to
 * identify or manage animations within the system.
 * 
 * @return uint16_t The ID of the current animation.
 */
uint16_t get_animation_id(void);

#endif
