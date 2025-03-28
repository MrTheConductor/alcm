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
#include "button_driver.h"
#include "button_driver_hw.h"
#include "event_queue.h"
#include "timer.h"

// Definitions
#define DEBOUNCE_PERIOD 5U // ms

/**
 * @brief Enumeration for button state
 *
 * This enumeration is used to track the state of the button as it is
 * being read. It has three states: none, pressed, and released.
 * @note This enumeration is used internally by button_driver.c
 */
typedef enum
{
    BUTTON_STATE_NONE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASED
} button_state_t;

// Forward declarations
EVENT_HANDLER(button_driver, wakeup);
TIMER_CALLBACK(button_driver, debounce);

// Static variables
static button_state_t button_state = BUTTON_STATE_NONE;      // current button state
static button_state_t last_button_state = BUTTON_STATE_NONE; // previous buttons tate
static event_type_t last_event = EVENT_NULL;
static uint32_t last_debounce_time = 0U;
static timer_id_t debounce_timer_id = INVALID_TIMER_ID;

lcm_status_t button_driver_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Initialize button driver
    button_driver_hw_init();

    // Subscribe to wakeup event
    SUBSCRIBE_EVENT(button_driver, EVENT_BUTTON_WAKEUP, wakeup);

    return status;
}

EVENT_HANDLER(button_driver, wakeup)
{
    // start debounce timer if not already active
    if (!is_timer_active(debounce_timer_id))
    {
        debounce_timer_id = set_timer(1, TIMER_CALLBACK_NAME(button_driver, debounce), true);
    }
}

TIMER_CALLBACK(button_driver, debounce)
{
    button_state_t reading =
        button_driver_hw_is_pressed() ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;

    if (reading != last_button_state)
    {
        last_debounce_time = system_tick;
    }
    else if ((system_tick - last_debounce_time) > DEBOUNCE_PERIOD)
    {
        button_state = reading;
        if (button_state == BUTTON_STATE_PRESSED && last_event != EVENT_BUTTON_DOWN)
        {
            event_data_t event_data = {0};
            event_data.button_data.time = system_tick;
            event_queue_push(EVENT_BUTTON_DOWN, &event_data);

            last_event = EVENT_BUTTON_DOWN;
        }
        else if (button_state == BUTTON_STATE_RELEASED && last_event != EVENT_BUTTON_UP)
        {
            event_data_t event_data = {0};
            event_data.button_data.time = system_tick;
            event_queue_push(EVENT_BUTTON_UP, &event_data);

            // safe to cancel timer on release
            cancel_timer(debounce_timer_id);

            last_event = EVENT_BUTTON_UP;
        }
    }
    last_button_state = reading;
}