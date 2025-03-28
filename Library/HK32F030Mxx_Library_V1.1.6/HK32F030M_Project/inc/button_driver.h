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
 *
 */

/**
 * @file button_driver.h
 * @brief Header file for the button driver module.
 */
#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H
#include "lcm_types.h"

/**
 * @brief Initializes the button driver.
 *
 * This function sets up the necessary configurations and initializes
 * the hardware and software components required for the button driver
 * to function properly.
 *
 * @param None
 * @return lcm_status_t Returns LCM_SUCCESS if initialization is successful
 */
lcm_status_t button_driver_init(void);

#endif