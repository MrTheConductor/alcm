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
#include "footpads.h"
#include "board_mode.h"
#include "event_queue.h"
#include "footpads_hw.h"
#include "timer.h"
#include "lcm_types.h"

// Definitions
#define FOOTPADS_SAMPLE_INTERVAL 100U // 100 ms
#define FOOTPADS_THRESHOLD 2.5f       // 2.5 V

// Forward declarations
EVENT_HANDLER(footpads, board_mode_changed);
TIMER_CALLBACK(footpads, sample);

// Local variables
static footpads_state_t footpads_state = NONE_FOOTPAD;
static timer_id_t footpads_timer_id = INVALID_TIMER_ID;

/**
 * @brief Initializes the footpads module.
 *
 * The footpads are ADC inputs that are periodically sampled. This function
 * initializes the footpads hardware and prepares them for use.
 */
lcm_status_t footpads_init()
{
    lcm_status_t status = LCM_SUCCESS;

    // Initialize the footpads hardware
    footpads_hw_init();

    // Subscribe to events
    SUBSCRIBE_EVENT(footpads, EVENT_BOARD_MODE_CHANGED, board_mode_changed);

    // Initialize states
    footpads_state = NONE_FOOTPAD;
    footpads_timer_id = INVALID_TIMER_ID;

    return status;
}

/**
 * @brief Handles board mode changes affecting the footpads
 *
 * This function handles changes in the board mode by starting or stopping
 * the footpads sampling timer accordingly. In the idle or riding modes, the
 * footpads sampling timer is started. In all other modes, the footpads
 * sampling timer is stopped.
 *
 * @param event The type of event that occurred, specified as an event_type_t
 * enumeration
 * @param value The new board mode, specified as a uint32_t which is casted to
 * board_mode_t
 */
EVENT_HANDLER(footpads, board_mode_changed)
{
    if (event == EVENT_BOARD_MODE_CHANGED)
    {
        switch (data->board_mode.mode)
        {
        // Enable footpads sampling timer when the board is in idle or
        // riding mode
        case BOARD_MODE_IDLE:
        case BOARD_MODE_RIDING:
            if (footpads_timer_id == INVALID_TIMER_ID || !is_timer_active(footpads_timer_id))
            {
                footpads_timer_id = set_timer(FOOTPADS_SAMPLE_INTERVAL,
                                              TIMER_CALLBACK_NAME(footpads, sample), true);
            }
            break;

        // Disable footpads sampling timer in all other modes
        default:
            if (footpads_timer_id != INVALID_TIMER_ID && is_timer_active(footpads_timer_id))
            {
                cancel_timer(footpads_timer_id);
            }
            break;
        }
    }
}

/**
 * @brief Handles the footpads sample timer expiration
 *
 * This function is called when the timer set in #footpads_init expires. It
 * samples the footpads and updates the #footpads_state accordingly.
 *
 * @param[in] system_tick The current system tick count
 */
TIMER_CALLBACK(footpads, sample)
{
    footpads_state_t new_state = 0;
    float left = footpads_hw_get_left();
    float right = footpads_hw_get_right();

    if (left > FOOTPADS_THRESHOLD)
    {
        new_state |= LEFT_FOOTPAD;
    }

    if (right > FOOTPADS_THRESHOLD)
    {
        new_state |= RIGHT_FOOTPAD;
    }

    if (new_state != footpads_state)
    {
        // Update state
        footpads_state = new_state;

        // Notify subscribers
        event_data_t event_data = {0};
        event_data.footpads_state = footpads_state;
        event_queue_push(EVENT_FOOTPAD_CHANGED, &event_data);
    }
}

footpads_state_t footpads_get_state(void)
{
    return footpads_state;
}