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
#include <stdbool.h>

#include "button_events.h"
#include "event_queue.h"
#include "timer.h"
#include "config.h"

// #define DEBUG_BUTTON

// State variables for tracking button events
typedef enum
{
    BUTTON_STATE_IDLE,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASED
} ButtonState;

static ButtonState buttonState = BUTTON_STATE_IDLE;
static uint32_t clickCount = 0U;        // Tracks the number of clicks
static uint32_t pressedStartTime = 0U;  // Tracks press start time for holds
static uint32_t releasedStartTime = 0U; // Tracks release time for multiple clicks
static bool holdTriggered = false;      // Flag for hold events
static timer_id_t hold_timer = INVALID_TIMER_ID;
static timer_id_t repeat_timer = INVALID_TIMER_ID;

// Forward declarations
EVENT_HANDLER(button_events, button_down);
EVENT_HANDLER(button_events, button_up);
TIMER_CALLBACK(button_events, repeat);
TIMER_CALLBACK(button_events, hold);
void reset_button_state(void);

/**
 * @brief Reset the button state to idle
 *
 * This function resets the button state variables to
 * their initial values, including the click count, button
 * state, pressed and released start times, and hold triggered
 * flag. It also cancels the hold and repeat timers.
 */
void reset_button_state()
{
    clickCount = 0U;
    buttonState = BUTTON_STATE_IDLE;
    pressedStartTime = 0U;
    releasedStartTime = 0U;
    holdTriggered = false;
    cancel_timer(hold_timer);
    cancel_timer(repeat_timer);
}

/**
 * @brief Initializes the button event handling module
 *
 * This function resets the button state and subscribes
 * to the relevant events. It should be called once
 * during the system initialization process.
 */
lcm_status_t button_events_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    reset_button_state();
    SUBSCRIBE_EVENT(button_events, EVENT_BUTTON_DOWN, button_down);
    SUBSCRIBE_EVENT(button_events, EVENT_BUTTON_UP, button_up);

    return status;
}

/**
 * @brief Handles the repeat timer expiration
 *
 * This function is triggered when the repeat timer expires after a button
 * release event. It pushes a button click event to the event queue if the
 * button is still in the released state and resets the button state.
 */
TIMER_CALLBACK(button_events, repeat)
{
    event_data_t event_data = {0};
    event_data.click_count = clickCount;

    if (buttonState == BUTTON_STATE_RELEASED)
    {
        event_queue_push(EVENT_BUTTON_CLICK, &event_data);
        reset_button_state();
    }
}

/**
 * @brief Handles the hold timer expiration
 *
 * This function is triggered when the hold timer expires after a button
 * press event. It pushes a button hold event to the event queue if the
 * button is still in the pressed state and sets the hold triggered flag.
 */
TIMER_CALLBACK(button_events, hold)
{
    event_data_t event_data = {0};
    event_data.click_count = clickCount;

    if (buttonState == BUTTON_STATE_PRESSED)
    {
        holdTriggered = true;
        event_queue_push(EVENT_BUTTON_HOLD, &event_data);
    }
}

/**
 * @brief Handles button down events
 *
 * This function is triggered when a button down event occurs.
 * It cancels any active repeat timers and updates the button state
 * based on the current state. If the button is in the idle state,
 * it starts the button press detection by setting the pressed start
 * time, incrementing the click count, and transitioning the button state
 * to pressed. It also sets a hold timer to detect hold events. If the
 * button is in the released state and within the repeat window, it starts
 * a new click by updating the pressed start time and click count. If the
 * button is in any other state, it handles unexpected states by resetting
 * the button state or pushing an emergency fault event.
 *
 * @param event The type of event that occurred, specified as an event_type_t
 * enumeration
 * @param system_tick The current system tick count, specified as a uint32_t
 */
EVENT_HANDLER(button_events, button_down)
{
    // Cancel the repeat
    cancel_timer(repeat_timer);

    switch (buttonState)
    {
    // Start button press detection
    case BUTTON_STATE_IDLE:
        pressedStartTime = data->button_data.time;
        clickCount++;
        buttonState = BUTTON_STATE_PRESSED;
        holdTriggered = false;
        hold_timer = set_timer(HOLD_MAX, TIMER_CALLBACK_NAME(button_events, hold), false);
        break;

    // If within the release window, start a new click
    case BUTTON_STATE_RELEASED:
        if (data->button_data.time - releasedStartTime <= REPEAT_WINDOW)
        {
            pressedStartTime = data->button_data.time;
            clickCount++;
            buttonState = BUTTON_STATE_PRESSED;
            hold_timer = set_timer(HOLD_MAX, TIMER_CALLBACK_NAME(button_events, hold), false);
        }
        else
        {
            // Something is off...
            reset_button_state();
        }
        break;

    case BUTTON_STATE_PRESSED:
        // Shouldn't normally happen
        break;

    default:
        // Shouldn't get here
        fault(EMERGENCY_FAULT_INVALID_STATE);
        break;
    }
}

/**
 * @brief Handles button up events
 *
 * This function is triggered when a button up event occurs.
 * It cancels any active hold timers and updates the button state
 * based on the current state. If the button is in the pressed state,
 * it checks if the hold timer was triggered and resets the state if
 * so. If not, it calculates the time since the button was pressed and
 * handles the event accordingly. If the time is too short or too long,
 * it resets the button state and logs the event. If the time is within
 * the valid range, it updates the released start time and transitions
 * the button state to released. If the button is in any other state, it
 * handles unexpected states by resetting the button state or pushing
 * an emergency fault event.
 *
 * @param event The type of event that occurred, specified as an event_type_t
 * enumeration
 * @param system_tick The current system tick count, specified as a uint32_t
 */
EVENT_HANDLER(button_events, button_up)
{
    // Cancel the hold timer
    cancel_timer(hold_timer);

    uint32_t click_time = data->button_data.time - pressedStartTime;

    switch (buttonState)
    {
    case BUTTON_STATE_PRESSED:
        if (holdTriggered)
        {
            reset_button_state();
        }
        else if (click_time < SINGLE_CLICK_MIN)
        {
            reset_button_state();
        }
        else if (click_time > SINGLE_CLICK_MAX)
        {
            reset_button_state();
        }
        else
        {
            releasedStartTime = data->button_data.time;
            buttonState = BUTTON_STATE_RELEASED;
            repeat_timer =
                set_timer(REPEAT_WINDOW, TIMER_CALLBACK_NAME(button_events, repeat), false);
        }
        break;

    case BUTTON_STATE_IDLE:
    case BUTTON_STATE_RELEASED:
        // Shouldn't normally happen
        break;

    default:
        // Shouldn't get here
        fault(EMERGENCY_FAULT_INVALID_STATE);
        break;
    }
}