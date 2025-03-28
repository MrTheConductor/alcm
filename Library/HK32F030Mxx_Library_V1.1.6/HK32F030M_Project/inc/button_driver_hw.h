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
 * @file button_driver_hw.h
 * @brief Hardware button driver interface.
 *
 * This file contains the function declarations for initializing the hardware
 * button driver and checking the button press status.
 */
#ifndef BUTTON_DRIVER_HW_H
#define BUTTON_DRIVER_HW_H

#include <stdbool.h>

/**
 * @brief Initializes the hardware button driver.
 *
 * This function sets up the necessary hardware configurations for the button
 * driver to operate correctly.
 */
void button_driver_hw_init(void);

/**
 * @brief Checks if the hardware button is pressed.
 *
 * This function returns the current status of the hardware button.
 *
 * @return true if the button is pressed, false otherwise.
 */
bool button_driver_hw_is_pressed(void);

#endif
