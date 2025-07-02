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
#ifndef _HEADLIGHTS_HW_H_
#define _HEADLIGHTS_HW_H_

#include "tim1.h"
#include <stdint.h>
#include <stdbool.h>

#define HEADLIGHTS_HW_MAX_BRIGHTNESS TIM1_PERIOD

/**
 * @brief Headlights direction
 */
typedef enum
{
    HEADLIGHTS_DIRECTION_FORWARD, // Forward direction (default)
    HEADLIGHTS_DIRECTION_REVERSE, // Reverse direction
    HEADLIGHTS_DIRECTION_NONE     // Unknown/off
} headlights_direction_t;

/**
 * @brief Initializes the hardware components for the headlights.
 *
 * This function sets up the necessary hardware configurations and
 * initializes the peripherals required for the headlights to function
 * properly. It should be called during the system initialization phase.
 */
void headlights_hw_init(void);

/**
 * @brief Sets the direction of the headlights.
 *
 * This function configures the direction of the headlights based on the
 * specified direction parameter.
 *
 * @param direction The desired direction for the headlights. This parameter
 *                  should be of type `headlights_direction_t`.
 */
void headlights_hw_set_direction(headlights_direction_t direction);

/**
 * @brief Retrieves the current direction of the headlights.
 *
 * This function returns the current direction of the headlights.
 *
 * @return headlights_direction_t The current direction of the headlights.
 */
headlights_direction_t headlights_hw_get_direction(void);

/**
 * @brief Sets the brightness level of the headlights.
 *
 * This function adjusts the brightness of the headlights to the specified level.
 *
 * @param brightness The desired brightness level, ranging from 0 (off) to the maximum value
 * supported by the hardware.
 *
 * @note This is a hardware brightness level that corresponds directly to the PWM signal.
 *
 */
void headlights_hw_set_brightness(uint16_t brightness);

#endif