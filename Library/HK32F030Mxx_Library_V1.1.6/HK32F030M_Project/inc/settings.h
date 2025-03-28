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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "lcm_types.h"
#include "status_leds.h"

/**
 * @brief Structure to hold the settings for the device.
 */
typedef struct
{
    float32_t headlight_brightness;        // Brightness level for the headlights
    float32_t status_brightness;           // Brightness level for the status LEDs.
    float32_t personal_color;              // Personal color.
    bool_t enable_beep;                    // Flag to enable or disable beep sound.
    bool_t enable_status_leds;             // Flag to enable or disable status LEDs.
    bool_t enable_headlights;              // Flag to enable or disable headlights.
    animation_option_t boot_animation;     // Animation option for boot sequence.
    animation_option_t idle_animation;     // Animation option for idle state.
    animation_option_t dozing_animation;   // Animation option for dozing state.
    animation_option_t shutdown_animation; // Animation option for shutdown sequence.
    animation_option_t ride_animation;     // Animation option for riding state.
} settings_t;

/**
 * @brief Initialize the settings with default values.
 */
lcm_status_t settings_init(void);

/**
 * @brief Save the current settings to persistent storage.
 */
void settings_save(void);

/**
 * @brief Get a pointer to the current settings.
 * @return Pointer to the current settings.
 */
settings_t *settings_get(void);

#endif // SETTINGS_H