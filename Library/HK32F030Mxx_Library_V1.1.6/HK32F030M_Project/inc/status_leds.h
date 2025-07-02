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
#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include <stdint.h>

#include "config.h"
#include "lcm_types.h"
#include "status_leds_hw.h"

typedef enum
{
    ANIMATION_OPTION_NONE,
    // Unique animations
    ANIMATION_OPTION_RAINBOW_SCAN,
    ANIMATION_OPTION_RAINBOW_MIRROR,
#ifdef ENABLE_KNIGHT_RIDER_ANIMATION
    ANIMATION_OPTION_KNIGHT_RIDER,
#endif
    ANIMATION_OPTION_RAINBOW_BAR,
#ifdef ENABLE_THE_FUZZ_ANIMATION
    ANIMATION_OPTION_THE_FUZZ,
#endif
#ifdef ENABLE_FIRE_ANIMATION
    ANIMATION_OPTION_FIRE,
#endif
    // Personalized animations
#ifdef ENABLE_EXPANDING_PULSE_ANIMATION
    ANIMATION_OPTION_EXPANDING_PULSE,
#endif
#ifdef ENABLE_IMPLODING_PULSE_ANIMATION
    ANIMATION_OPTION_IMPLODING_PULSE,
#endif
    ANIMATION_OPTION_120_SCROLL,
    ANIMATION_OPTION_COMPLEMENTARY_WAVE,
    ANIMATION_OPTION_FLOATWHEEL_CLASSIC,
    ANIMATION_OPTION_PERSONAL_SCAN,
    ANIMATION_OPTION_COUNT
} animation_option_t;

/**
 * @brief Initializes the status LEDs
 *
 * This function initializes the status LEDs and prepares them for use.
 *
 * @return lcm_status_t Status of the initialization operation.
 */
lcm_status_t status_leds_init(void);

/**
 * @brief Sets status leds [begin, end] to the specified color.
 *
 * @param color The color to set the LEDs to.
 * @param begin The starting index of the LEDs to set.
 * @param end The ending index of the LEDs to set.
 * @return lcm_status_t Status of the operation.
 */
lcm_status_t status_leds_set_color(const status_leds_color_t *color, uint8_t begin, uint8_t end);

/**
 * @brief Refreshes the status LEDs display.
 */
lcm_status_t status_leds_refresh(void);

#endif