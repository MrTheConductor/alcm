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
 * @file hysteresis.c
 * @brief Implementation of hysteresis logic
 *
 * This file contains the implementation of hysteresis logic used in the
 * Advanced LCM (ALCM) project. It provides functions to initialize and apply
 * hysteresis to a given value.
 */
#include <stddef.h>
#include "hysteresis.h"

/**
 * @brief Initializes the hysteresis structure with specified thresholds.
 */
lcm_status_t hysteresis_init(hysteresis_t *hysteresis, float32_t set_threshold,
                             float32_t reset_threshold)
{
    lcm_status_t status = LCM_SUCCESS;

    if (hysteresis == NULL)
    {
        status = LCM_ERROR;
    }
    else if (set_threshold < reset_threshold)
    {
        status = LCM_ERROR;
        hysteresis->state = STATE_ERROR;
    }
    else
    {
        hysteresis->state = STATE_RESET;
        hysteresis->set_threshold = set_threshold;
        hysteresis->reset_threshold = reset_threshold;
    }

    return status;
}

/**
 * @brief Applies hysteresis logic to a given value.
 */
hys_state_t apply_hysteresis(hysteresis_t *hysteresis, float32_t value)
{
    hys_state_t state = STATE_ERROR;

    if (hysteresis != NULL)
    {
        if ((hysteresis->state == STATE_RESET) && (value >= hysteresis->set_threshold))
        {
            hysteresis->state = STATE_SET;
            state = STATE_SET;
        }
        else if ((hysteresis->state == STATE_SET) && (value < hysteresis->reset_threshold))
        {
            hysteresis->state = STATE_RESET;
            state = STATE_RESET;
        }
        else
        {
            state = hysteresis->state;
        }
    }

    return state;
}

// Newline at end of file
