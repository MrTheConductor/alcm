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
#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include "lcm_types.h"

#define INVALID_TIMER_ID 0U
#define TIMER_CALLBACK_NAME(module, e) module##_##e##_timer_callback
#define TIMER_CALLBACK(module, e) void TIMER_CALLBACK_NAME(module, e)(uint32_t system_tick)

typedef uint8_t timer_id_t;

/**
 * @brief Initialize the timer module.
 *
 * Initializes the timer module structures.
 */
lcm_status_t timer_init(void);

/**
 * @brief Generates a unique ID for a timer and starts the timer.
 *
 * @param[in] timeout  The timeout period in milliseconds.
 * @param[in] callback The callback function to be called when the timer
 * expires.
 * @param[in] repeat   Whether the timer should repeat or not.
 *
 * @return A unique ID for the timer on success. TIMER_ERROR on failure.
 */
timer_id_t set_timer(uint32_t timeout, void (*callback)(uint32_t), bool_t repeat);

/**
 * @brief Cancel a timer
 *
 * Cancels a timer and removes it from the active timer list.
 *
 * @param[in] timer_id The ID of the timer to cancel.
 *
 * @return LCM_SUCCESS on success, LCM_ERROR on failure.
 */
lcm_status_t cancel_timer(timer_id_t timer_id);

/**
 * @brief Check if a timer is active
 *
 * @param[in] timer_id The ID of the timer to check.
 *
 * @return true if the timer is active, false otherwise.
 */
bool_t is_timer_active(timer_id_t timer_id);

/**
 * @brief Returns the number of active timers.
 *
 * @return The number of active timers.
 */
uint8_t timer_active_count(void);

/**
 * @brief Check if a timer is set to repeat
 *
 * @param[in] timer_id The ID of the timer to check.
 *
 * @return true if the timer is set to repeat, false otherwise.
 */
bool_t is_timer_repeating(timer_id_t timer_id);

/**
 * @brief Returns the remaining time of a timer
 *
 * @param[in] timer_id The ID of the timer to check.
 *
 * @return The remaining time of the timer in milliseconds.
 */
uint32_t get_timer_remaining(timer_id_t timer_id);

/**
 * @brief Get the maximum number of timers that can be used
 *
 * @return The maximum number of timers that can be used.
 */
uint8_t get_max_timers(void);

#endif