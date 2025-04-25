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
 * @file footpads.h
 * @brief Footpads module header file.
 */
#ifndef FOOTPADS_H
#define FOOTPADS_H
#include <stdint.h>
#include "lcm_types.h"

#define NONE_FOOTPAD 0x00U
#define LEFT_FOOTPAD 0x01U
#define RIGHT_FOOTPAD 0x02U

typedef uint8_t footpads_state_t;

/**
 * @brief Initializes the footpads module.
 *
 * This function initializes the footpads hardware and prepares it for use.
 */
lcm_status_t footpads_init(void);

/**
 * @brief Gets the current state of the footpads.
 * @return A bitwise OR of #LEFT_FOOTPAD and #RIGHT_FOOTPAD indicating the state
 * of each footpad.
 */
footpads_state_t footpads_get_state(void);

#endif
