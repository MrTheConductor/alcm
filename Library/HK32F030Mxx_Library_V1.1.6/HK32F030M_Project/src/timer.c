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
#include <stddef.h>
#include <string.h>

#include "timer.h"
#include "event_queue.h"
#include "config.h"
#include "lcm_types.h"

#define FIRST_TIMER_ID (INVALID_TIMER_ID + 1U)

/**
 * @struct timer
 * @brief Timer structure
 *
 * This structure contains all the necessary information to manage a timer.
 * It consists of a timeout period, a counter, a callback function, a flag to
 * indicate whether the timer should repeat, and a unique ID.
 */
typedef struct
{
    uint32_t timeout;           // Timeout period in milliseconds
    uint32_t counter;           // Counter to keep track of time
    void (*callback)(uint32_t); // Callback function to call when timer expires
    bool_t repeat;              // Flag to indicate whether the timer should repeat
    timer_id_t id;              // Unique ID for the timer
} timer_t;

// Static variables
static timer_id_t next_timer_id = FIRST_TIMER_ID; // used to generate unique IDs
static timer_t timers[MAX_TIMERS] = {0};

// Forward declarations
EVENT_HANDLER(timer, system_tick);

/**
 * @brief Initialize the timer module
 */
lcm_status_t timer_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Initialize the timer module
    next_timer_id = FIRST_TIMER_ID;
    memset(timers, 0, sizeof(timers));

    // Subscribe to the system tick event
    SUBSCRIBE_EVENT(timer, EVENT_SYS_TICK, system_tick);

    return status;
}

timer_id_t find_timer_id_by_callback(void (*callback)(uint32_t))
{
    timer_id_t timer_id = INVALID_TIMER_ID;
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timers[i].callback == callback)
        {
            timer_id = timers[i].id;
            break;
        }
    }
    return timer_id;
}

/**
 * @brief Find a timer by its ID
 *
 * @param timer_id The ID of the timer to find
 * @return A pointer to the timer structure if found, NULL otherwise
 */
timer_t *find_timer_by_id(timer_id_t timer_id)
{
    timer_t *found_timer = NULL;
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timers[i].id == timer_id)
        {
            found_timer = &timers[i];
            break;
        }
    }
    return found_timer;
}

/**
 * @brief Find the next available timer
 *
 * @return A pointer to the next available timer structure
 */
timer_t *find_available_timer(void)
{
    timer_t *available_timer = NULL;
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timers[i].callback == NULL)
        {
            available_timer = &timers[i];
            break;
        }
    }
    return available_timer;
}

/**
 * @brief Find the next available timer ID
 *
 * @return The next available timer ID
 */
timer_id_t find_next_timer_id(void)
{
    const timer_t *timer = find_timer_by_id(next_timer_id);
    while (timer != NULL)
    {
        next_timer_id++;
        timer = find_timer_by_id(next_timer_id);
    }
    return next_timer_id;
}

/**
 * @brief Generates a unique ID for a timer and starts the timer.
 */
timer_id_t set_timer(uint32_t timeout, void (*callback)(uint32_t), bool_t repeat)
{
    timer_id_t timer_id = find_timer_id_by_callback(callback);

    if (timer_id == INVALID_TIMER_ID)
    {
        // Create a new timer
        timer_t *timer = find_available_timer();
        if (timer != NULL)
        {
            timer->timeout = timeout;
            timer->counter = timeout;
            timer->callback = callback;
            timer->repeat = repeat;
            timer->id = find_next_timer_id();
            timer_id = timer->id;
        }
        else
        {
            timer_id = INVALID_TIMER_ID;
            fault(EMERGENCY_FAULT_OVERFLOW);
        }
    }
    else
    {
        // This timer already exists, so update it
        timer_t *timer = find_timer_by_id(timer_id);
        timer->timeout = timeout;
        timer->counter = timeout;
        timer->repeat = repeat;
    }

    // Return either the new timer ID or the existing timer ID
    return timer_id;
}

/**
 * @brief Cancel a timer
 */
lcm_status_t cancel_timer(timer_id_t timer_id)
{
    lcm_status_t status = LCM_ERROR;
    if (timer_id != INVALID_TIMER_ID)
    {
        timer_t *timer = find_timer_by_id(timer_id);
        if (timer != NULL)
        {
            timer->callback = NULL;
            status = LCM_SUCCESS;
        }
    }
    return status;
}

/**
 * @brief Check if a timer is active
 */
bool_t is_timer_active(timer_id_t timer_id)
{
    bool_t is_active = false;
    if (timer_id != INVALID_TIMER_ID)
    {
        const timer_t *timer = find_timer_by_id(timer_id);
        if ((timer != NULL) && (timer->callback != NULL))
        {
            is_active = true;
        }
    }
    return is_active;
}

/**
 * @brief Decrements all active timers and calls their callbacks when they
 * expire.
 */
EVENT_HANDLER(timer, system_tick)
{
    (void)event; // Ignore unused parameter

    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timers[i].callback != NULL)
        {
            timers[i].counter--;
            if (timers[i].counter == 0U)
            {
                timers[i].callback(data->system_tick);

                // There's a possibility that the callback has
                // called set_timer() which is why we need to check
                // the counter again
                if (timers[i].counter == 0U)
                {
                    // Doesn't seem they have called set_timer()
                    if (timers[i].repeat)
                    {
                        timers[i].counter = timers[i].timeout;
                    }
                    else
                    {
                        // Cancel the callback
                        timers[i].callback = NULL;
                    }
                }
                // Else, the callback has called set_timer()
            }
            // No else needed as counter is already decremented
        }
        // No else needed as callback is NULL
    }
}

/**
 * @brief Returns the number of active timers.
 */
uint8_t timer_active_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timers[i].callback != NULL)
        {
            count++;
        }
    }
    return count;
}

/**
 * @brief Checks if a timer is set to repeat.
 */
bool_t is_timer_repeating(timer_id_t timer_id)
{
    bool_t is_repeating = false;
    if (timer_id != INVALID_TIMER_ID)
    {
        const timer_t *timer = find_timer_by_id(timer_id);
        if (timer != NULL)
        {
            is_repeating = timer->repeat;
        }
    }
    return is_repeating;
}

/**
 * @brief Returns the remaining time of a timer
 */
uint32_t get_timer_remaining(timer_id_t timer_id)
{
    uint32_t remaining_time = 0;
    if (timer_id != INVALID_TIMER_ID)
    {
        const timer_t *timer = find_timer_by_id(timer_id);
        if (timer != NULL)
        {
            remaining_time = timer->counter;
        }
    }
    return remaining_time;
}

/**
 * @brief Get the maximum number of timers that can be used
 */
uint8_t get_max_timers(void)
{
    return MAX_TIMERS;
}
