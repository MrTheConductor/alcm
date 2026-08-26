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
 * @file footpads_hw.h
 * @brief Header file for footpads hardware initialization and reading.
 */
#ifndef FOOTPADS_HW_H
#define FOOTPADS_HW_H

#include <stdint.h>

/**
 * @brief Initializes the hardware for the footpads.
 *
 * This function sets up the necessary hardware configurations for the footpads
 * to function correctly. It should be called during the system initialization
 * phase before any footpad operations are performed.
 */
void footpads_hw_init(void);

/**
 * @brief Retrieves the raw ADC count from the left footpad hardware sensor.
 *
 * @return The current raw ADC reading from the left footpad hardware sensor.
 */
uint16_t footpads_hw_get_left(void);

/**
 * @brief Retrieves the raw ADC count from the right footpad sensor.
 *
 * @return The current raw ADC reading from the right footpad sensor.
 */
uint16_t footpads_hw_get_right(void);

/**
 * @brief Calibrates the footpads hardware.
 */
void footpads_hw_calibrate(void);

#endif // FOOTPADS_HW_H
