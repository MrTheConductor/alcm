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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "timer.h"
#include "mock_timer.h"

// Mock implementations
static uint32_t timer_count = 0;
#define MAX_TIMERS 8

typedef struct
{
    uint32_t timer_id;
    void (*callback)(uint32_t);
} timer_t;

timer_t timers[MAX_TIMERS] = {0};

lcm_status_t timer_init(void)
{
    timer_count = 0;
    memset(timers, 0, sizeof(timers));
    return LCM_SUCCESS;
}

/**
 * @brief Mock implementation of set_timer
 *
 * @param[in] timeout  The timeout period in milliseconds.
 * @param[in] callback The callback function to be called when the timer expires.
 * @param[in] repeat   Whether the timer should repeat or not.
 *
 * @return A unique ID for the timer on success. TIMER_ERROR on failure.
 */
timer_id_t set_timer(uint32_t timeout, void (*callback)(uint32_t), bool repeat)
{
    check_expected(timeout);
    check_expected_ptr(callback);
    check_expected(repeat);
    timers[timer_count].timer_id = timer_count+1;
    timers[timer_count].callback = callback;
    timer_count++;
    return timer_count;
}

void call_timer_callback(timer_id_t timer_id, uint32_t system_tick)
{
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        if (timers[i].timer_id == timer_id)
        {
            timers[i].callback(system_tick);
        }
    }
}

/**
 * @brief Mock implementation of cancel_timer
 *
 * @param[in] timer_id The ID of the timer to cancel.
 *
 * @return void
 */
lcm_status_t cancel_timer(timer_id_t timer_id)
{
    check_expected(timer_id);
    return (lcm_status_t)mock();
}

/**
 * @brief Mock implementation of is_timer_active
 *
 * @param[in] timer_id The ID of the timer to check.
 *
 * @return true if the timer is active, false otherwise.
 */
bool is_timer_active(timer_id_t timer_id)
{
    check_expected(timer_id);
    return mock_type(bool);
}
