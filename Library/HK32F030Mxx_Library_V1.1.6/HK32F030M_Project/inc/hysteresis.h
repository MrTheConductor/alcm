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
 * @file hysteresis.h
 * @brief Header file for hysteresis logic
 */
#ifndef HYSTERESIS_H
#define HYSTERESIS_H
#include "lcm_types.h"

/**
 * @brief Enumeration for hysteresis states
 */
typedef enum
{
    STATE_RESET, // State is reset
    STATE_SET,   // State is set
    STATE_ERROR
} hys_state_t;

/**
 * @brief Structure for hysteresis logic
 */
typedef struct
{
    hys_state_t state;         // Current state of the hysteresis
    float32_t set_threshold;   // Threshold to set the state
    float32_t reset_threshold; // Threshold to reset the state
} hysteresis_t;

/**
 * @brief Initializes the hysteresis structure with specified thresholds.
 *
 * @param hysteresis Pointer to the hysteresis structure to initialize.
 * @param set_threshold The threshold value at which the hysteresis will set.
 * @param reset_threshold The threshold value at which the hysteresis will reset.
 */
lcm_status_t hysteresis_init(hysteresis_t *hysteresis, float32_t set_threshold,
                             float32_t reset_threshold);

/**
 * @brief Applies hysteresis logic to a given value.
 */
hys_state_t apply_hysteresis(hysteresis_t *hysteresis, float32_t value);

#endif // HYSTERESIS_H