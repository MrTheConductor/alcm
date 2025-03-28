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
#include "power.h"
#include "board_mode.h"
#include "event_queue.h"
#include "power_hw.h"
#include "lcm_types.h"

// Forward declaration
EVENT_HANDLER(power, board_mode_changed);

lcm_status_t power_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Initialize hardware
    power_hw_init();

    // Set initial state
    power_hw_set_power(POWER_HW_OFF);
    power_hw_set_charge(POWER_HW_OFF);

    // Subscribe to events
    SUBSCRIBE_EVENT(power, EVENT_BOARD_MODE_CHANGED, board_mode_changed);

    return status;
}

/**
 * @brief Handles board mode changes affecting the power state
 *
 * This function handles changes in the board mode by setting the power
 * state accordingly. Different board modes will set the power state to
 * different values.
 *
 * @param value The new board mode, specified as a uint32_t which is casted to
 * board_mode_t
 */
EVENT_HANDLER(power, board_mode_changed)
{
    switch (event)
    {
    case EVENT_BOARD_MODE_CHANGED:
        switch (data->board_mode.mode)
        {
        case BOARD_MODE_BOOTING:
            power_hw_set_power(POWER_HW_ON);
            break;
        case BOARD_MODE_OFF:
            power_hw_set_power(POWER_HW_OFF);
            break;
        // TBD: add charging modes
        default:
            break;
        }
        break;

    default:
        // Unexpected event: raise an error
        fault(EMERGENCY_FAULT_INVALID_EVENT);
        break;
    }
}