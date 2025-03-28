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
#ifndef _HEADLIGHTS_H_
#define _HEADLIGHTS_H_

#include <stdint.h>
#include "lcm_types.h"

/**
 * @brief Initializes the headlights module.
 *
 * This function performs the necessary initialization steps for the
 * headlights module, including:
 *  - Retrieving and validating the headlights settings.
 *  - Initializing the headlights hardware.
 *  - Enabling/disabling the headlights based on user settings.
 *  - Setting the initial headlight direction to none.
 *  - Setting up the RPM hysteresis.
 *  - Subscribing to events that trigger headlight state changes.
 *
 * After this function is called, the headlights module is ready to
 * respond to events and user settings.
 *
 * @note This function must be called once at startup before the
 *       headlights module can be used.
 * @note The function uses the `settings_get()` function to retrieve user
 *       preferences, and will trigger an emergency fault if that function
 *       returns NULL.
 * @note The initial direction is set to `HEADLIGHTS_DIRECTION_NONE`.
 * @note The `headlights_rpm_hys` hysteresis struct is initialized with a
 *       positive and negative threshold defined by `RPM_HYSTERISIS`.
 * @see settings_get()
 * @see headlights_hw_init()
 * @see headlights_hw_enable()
 * @see headlights_hw_set_direction()
 * @see hysteresis_init()
 * @see SUBSCRIBE_EVENT()
 */
lcm_status_t headlights_init(void);

#endif