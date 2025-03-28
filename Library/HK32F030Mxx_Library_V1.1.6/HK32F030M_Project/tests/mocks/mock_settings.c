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

 /**
  * @file mock_settings.c
  * @brief Mock source file for settings.c
  * @details This file is used for unit testing and mocking purposes.
  */
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "settings.h"
#include "status_leds.h"

static settings_t mock_settings;

lcm_status_t settings_init(void) {
    mock_settings.headlight_brightness = 1.0f;
    mock_settings.status_brightness = 1.0f;
    mock_settings.enable_beep = true;
    mock_settings.enable_status_leds = true;
    mock_settings.enable_headlights = true;
    mock_settings.boot_animation = ANIMATION_OPTION_NONE;
    mock_settings.idle_animation = ANIMATION_OPTION_NONE;
    mock_settings.dozing_animation = ANIMATION_OPTION_NONE;
    mock_settings.shutdown_animation = ANIMATION_OPTION_NONE;
    mock_settings.ride_animation = ANIMATION_OPTION_NONE;
    mock_settings.personal_color = 0.0f;

    return LCM_SUCCESS;
}

void settings_save(void) {
    // Mock save function does nothing
}

settings_t *settings_get(void) {
    return &mock_settings;
}

