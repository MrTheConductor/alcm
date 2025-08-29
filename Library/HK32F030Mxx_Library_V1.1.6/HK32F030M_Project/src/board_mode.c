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
#include <stdlib.h>

#include "board_mode.h"
#include "event_queue.h"
#include "footpads.h"
#include "vesc_serial.h"
#include "timer.h"
#include "config.h"
#include "hysteresis.h"

// Forward declarations
EVENT_HANDLER(board_mode, command);
EVENT_HANDLER(board_mode, rpm_changed);
EVENT_HANDLER(board_mode, emergency_fault);
EVENT_HANDLER(board_mode, footpad_changed);
EVENT_HANDLER(board_mode, vesc_alive);
EVENT_HANDLER(board_mode, duty_cycle_changed);

// Timer handlers
void board_mode_idle_timer_handler(uint32_t system_tick);

static board_mode_t board_mode = BOARD_MODE_UNKNOWN;
static board_submode_t board_submode = BOARD_SUBMODE_UNDEFINED;

// The idle timer is used to shutdown the board after a period of inactivity
static timer_id_t board_mode_idle_timer_id = INVALID_TIMER_ID;

// Hysteresis values for RPM and duty cycle
static hysteresis_t stopped_rpm_hysteresis;
static hysteresis_t slow_rpm_hysteresis;
static hysteresis_t danger_hysteresis;
static hysteresis_t warning_hysteresis;

/**
 * @brief Initialization function for board mode management
 *
 * Initializes the board mode by setting it to BOARD_MODE_OFF and subscribing
 * to the relevant events. This function should be called once during the
 * system initialization process.
 */
lcm_status_t board_mode_init(void)
{
    lcm_status_t status = LCM_SUCCESS;
    board_mode = BOARD_MODE_OFF;
    board_submode = BOARD_SUBMODE_UNDEFINED;

    // Subscribe to events
    SUBSCRIBE_EVENT(board_mode, EVENT_BUTTON_UP, command);
    SUBSCRIBE_EVENT(board_mode, EVENT_COMMAND_SHUTDOWN, command);
    SUBSCRIBE_EVENT(board_mode, EVENT_COMMAND_BOOT, command);
    SUBSCRIBE_EVENT(board_mode, EVENT_COMMAND_MODE_CONFIG, command);
    SUBSCRIBE_EVENT(board_mode, EVENT_RPM_CHANGED, rpm_changed);
    SUBSCRIBE_EVENT(board_mode, EVENT_EMERGENCY_FAULT, emergency_fault);
    SUBSCRIBE_EVENT(board_mode, EVENT_FOOTPAD_CHANGED, footpad_changed);
    SUBSCRIBE_EVENT(board_mode, EVENT_VESC_ALIVE, vesc_alive);
    SUBSCRIBE_EVENT(board_mode, EVENT_DUTY_CYCLE_CHANGED, duty_cycle_changed);
#if defined(ENABLE_ROLL_EVENTS)
    SUBSCRIBE_EVENT(board_mode, EVENT_IMU_ROLL_CHANGED, command);
#endif

    // Initialize hysteresis values
    if (LCM_SUCCESS != hysteresis_init(&stopped_rpm_hysteresis, STOPPED_RPM_THRESHOLD,
                                       STOPPED_RPM_THRESHOLD - (STOPPED_RPM_THRESHOLD * 0.1f)))
    {
        status = LCM_ERROR;
    }

    if (LCM_SUCCESS != hysteresis_init(&slow_rpm_hysteresis, SLOW_RPM_THRESHOLD,
                                       SLOW_RPM_THRESHOLD - (SLOW_RPM_THRESHOLD * 0.1f)))
    {
        status = LCM_ERROR;
    }

    if (LCM_SUCCESS != hysteresis_init(&danger_hysteresis, DUTY_CYCLE_DANGER_THRESHOLD,
                                       DUTY_CYCLE_DANGER_THRESHOLD - 5.0f))
    {
        status = LCM_ERROR;
    }

    if (LCM_SUCCESS != hysteresis_init(&warning_hysteresis, DUTY_CYCLE_WARNING_THRESHOLD,
                                       DUTY_CYCLE_WARNING_THRESHOLD - 5.0f))
    {
        status = LCM_ERROR;
    }

    return status;
}

/**
 * @brief Gets the current board mode
 *
 * @return The current board mode as a board_mode_t enumeration
 */
board_mode_t board_mode_get(void)
{
    return board_mode;
}

/**
 * @brief Gets the current board submode
 *
 * @return The current board submode as a board_submode_t enumeration
 */
board_submode_t board_submode_get(void)
{
    return board_submode;
}

/**
 * @brief Sets the current board mode and submode
 *
 * Sets the current board mode and submode. If the board mode or submode is
 * different from the current values, an EVENT_BOARD_MODE_CHANGED event is
 * raised and timers are managed accordingly.
 *
 * @param mode The new board mode, specified as a board_mode_t enumeration
 * @param submode The new board submode, specified as a board_submode_t
 * enumeration
 */
void set_board_mode(board_mode_t mode, board_submode_t submode)
{
    if (board_mode != mode || board_submode != submode)
    {
        event_data_t event_data = {0};

        // Raise the EVENT_BOARD_MODE_CHANGED event
        event_data.board_mode.mode = mode;
        event_data.board_mode.submode = submode;
        event_data.board_mode.previous_mode = board_mode;
        event_data.board_mode.previous_submode = board_submode;
        event_queue_push(EVENT_BOARD_MODE_CHANGED, &event_data);

        // Update the board mode
        board_mode = mode;
        board_submode = submode;

        // Manage timers
        switch (board_mode)
        {
        case BOARD_MODE_IDLE:
            switch (board_submode)
            {
            case BOARD_SUBMODE_IDLE_ACTIVE:
                board_mode_idle_timer_id =
                    set_timer(IDLE_ACTIVE_TIMEOUT, board_mode_idle_timer_handler, false);
                break;
            case BOARD_SUBMODE_IDLE_DEFAULT:
                board_mode_idle_timer_id =
                    set_timer(IDLE_DEFAULT_TIMEOUT, board_mode_idle_timer_handler, false);
                break;
            case BOARD_SUBMODE_IDLE_DOZING:
                board_mode_idle_timer_id =
                    set_timer(IDLE_DOZING_TIMEOUT, board_mode_idle_timer_handler, false);
                break;
            case BOARD_SUBMODE_IDLE_SHUTTING_DOWN:
                board_mode_idle_timer_id =
                    set_timer(IDLE_SHUTTING_DOWN_TIMEOUT, board_mode_idle_timer_handler, false);
                break;
            case BOARD_SUBMODE_IDLE_CONFIG:
                // User is configuring the board, so don't need to set a timer
                if (board_mode_idle_timer_id != INVALID_TIMER_ID &&
                    is_timer_active(board_mode_idle_timer_id))
                {
                    cancel_timer(board_mode_idle_timer_id);
                }
                break;
            default:
                fault(EMERGENCY_FAULT_INVALID_STATE);
                break;
            }
            break;
        case BOARD_MODE_RIDING:
            if (board_mode_idle_timer_id != INVALID_TIMER_ID &&
                is_timer_active(board_mode_idle_timer_id))
            {
                cancel_timer(board_mode_idle_timer_id);
            }
            break;
        case BOARD_MODE_FAULT:
            if (board_mode_idle_timer_id != INVALID_TIMER_ID &&
                is_timer_active(board_mode_idle_timer_id))
            {
                cancel_timer(board_mode_idle_timer_id);
            }
            break;
        default:
            // Do nothing
            break;
        }
    }
    // Else: mode already defined
}

/**
 * @brief Handles the VESC alive event affecting the board mode
 *
 * This function is triggered when the VESC is detected as alive. It sets
 * the board mode to IDLE with the ACTIVE submode.
 *
 * @param event The type of event that occurred, specified as an event_type_t
 * enumeration
 */
EVENT_HANDLER(board_mode, vesc_alive)
{
    // If the event is EVENT_VESC_ALIVE and we are booting, set the board mode
    // to IDLE with the ACTIVE submode to indicate that the board is ready
    if (event == EVENT_VESC_ALIVE && board_mode == BOARD_MODE_BOOTING)
    {
        set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_ACTIVE);
    }
}

EVENT_HANDLER(board_mode, command)
{
    switch (event)
    {
    case EVENT_COMMAND_BOOT:
        if (board_mode == BOARD_MODE_OFF)
        {
            set_board_mode(BOARD_MODE_BOOTING, BOARD_SUBMODE_UNDEFINED);
        }
        // Else: Nothing to do in this mode
        break;
    case EVENT_COMMAND_SHUTDOWN:
        /* This is tricky. If the user is actively riding, we
         * shouldn't shut down. However, there's a small corner case
         * where the footpad sensor gets stuck and the board is ghosting.
         * In that case, this is the only way to shut down.
         */
        set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_SHUTTING_DOWN);
        break;
    case EVENT_COMMAND_MODE_CONFIG:
        if (data->enable)
        {
            if (board_mode == BOARD_MODE_IDLE)
            {
                set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_CONFIG);
            }
            else
            {
                // Configuration mode can only be entered from idle mode so
                // raise a NACK event
                event_queue_push(EVENT_COMMAND_NACK, NULL);
            }
        }
        else
        {
            // Return to IDLE_ACTIVE
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_ACTIVE);
        }
        break;
    case EVENT_BUTTON_UP:
        // Abort shutdown if the user releases the button
        if (board_mode == BOARD_MODE_IDLE && board_submode == BOARD_SUBMODE_IDLE_SHUTTING_DOWN)
        {
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_ACTIVE);
        }
        break;
#if defined(ENABLE_ROLL_EVENTS)
    case EVENT_IMU_ROLL_CHANGED:
        // If the board is on its side, transition to dozing idle mode
        if (board_mode == BOARD_MODE_IDLE && 
            (board_submode == BOARD_SUBMODE_IDLE_ACTIVE || board_submode == BOARD_SUBMODE_IDLE_DEFAULT) &&
            (data->imu_roll > 45.0f || data->imu_roll < -45.0f))
        {
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_DOZING);
        }
        // Otherwise, if the board is dozing and turned upright, transition to active idle mode
        else if (board_mode == BOARD_MODE_IDLE && 
                 board_submode == BOARD_SUBMODE_IDLE_DOZING &&
                 (data->imu_roll <= 45.0f && data->imu_roll >= -45.0f))
        {
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_ACTIVE);
        } 
        break;
#endif // ENABLE_ROLL_EVENTS
    default:
        // Unexpected event
        break;
    }
}

/**
 * @brief Timer handler for idle mode.
 *
 * This function is called when the idle timer expires. It checks if the board
 * is in the idle mode and transitions the board submode accordingly. If the
 * board is in the default idle submode, it transitions to the dozing submode.
 * If the board is in the dozing submode, it transitions to the shutting down
 * submode. No action is taken for other submodes.
 *
 * @param system_tick The current system tick count.
 */
void board_mode_idle_timer_handler(uint32_t system_tick)
{
    if (board_mode == BOARD_MODE_IDLE)
    {
        switch (board_submode)
        {
        case BOARD_SUBMODE_IDLE_ACTIVE:
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_DEFAULT);
            break;
        case BOARD_SUBMODE_IDLE_DEFAULT:
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_DOZING);
            break;
        case BOARD_SUBMODE_IDLE_DOZING:
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_SHUTTING_DOWN);
            break;
        case BOARD_SUBMODE_IDLE_SHUTTING_DOWN:
            set_board_mode(BOARD_MODE_OFF, BOARD_SUBMODE_UNDEFINED);
            break;
        default:
            // Nothing to do in this mode
            break;
        }
    }
    // Else: Nothing to do in this mode
}

/**
 * @brief Updates the riding submode based on the duty cycle and RPM
 *
 * This function is called when the duty cycle or RPM values change. It sets
 * the riding submode based on the current duty cycle and RPM values.
 *
 * @see BOARD_SUBMODE_RIDING_DANGER
 * @see BOARD_SUBMODE_RIDING_WARNING
 * @see BOARD_SUBMODE_RIDING_NORMAL
 * @see BOARD_SUBMODE_RIDING_SLOW
 * @see DUTY_CYCLE_DANGER_THRESHOLD
 * @see DUTY_CYCLE_WARNING_THRESHOLD
 * @see SLOW_RPM_THRESHOLD
 */
void update_riding_submode()
{
    float duty_cycle = vesc_serial_get_duty_cycle();
    int32_t rpm = vesc_serial_get_rpm();

#if defined(ENABLE_ROLL_EVENTS)
    float imu_roll = vesc_serial_get_imu_roll();
    if (imu_roll > 45.0f || imu_roll < -45.0f)
    {
        // Don't change the riding submode if the board is on its side, 
        // it's not possible to ride it in this state
        return;
    }
#endif

    // Set the submode based on the duty cycle and RPM
    if (apply_hysteresis(&danger_hysteresis, duty_cycle) == STATE_SET)
    {
        if (board_submode != BOARD_SUBMODE_RIDING_DANGER)
        {
            set_board_mode(BOARD_MODE_RIDING, BOARD_SUBMODE_RIDING_DANGER);
        }
        // No else required - already in danger submode
    }
    else if (apply_hysteresis(&warning_hysteresis, duty_cycle) == STATE_SET)
    {
        if (board_submode != BOARD_SUBMODE_RIDING_WARNING)
        {
            set_board_mode(BOARD_MODE_RIDING, BOARD_SUBMODE_RIDING_WARNING);
        }
        // No else required - already in warning submode
    }
    else if (apply_hysteresis(&slow_rpm_hysteresis, (float)abs(rpm)) == STATE_SET)
    {
        if (board_submode != BOARD_SUBMODE_RIDING_NORMAL)
        {
            set_board_mode(BOARD_MODE_RIDING, BOARD_SUBMODE_RIDING_NORMAL);
        }
        // No else required - already in normal submode
    }
    else if (apply_hysteresis(&stopped_rpm_hysteresis, (float)abs(rpm)) == STATE_SET)
    {
        if (board_submode != BOARD_SUBMODE_RIDING_SLOW)
        {
            set_board_mode(BOARD_MODE_RIDING, BOARD_SUBMODE_RIDING_SLOW);
        }
        // No else required - already in stopped submode
    }
    else
    {
        if (board_submode != BOARD_SUBMODE_RIDING_STOPPED)
        {
            set_board_mode(BOARD_MODE_RIDING, BOARD_SUBMODE_RIDING_STOPPED);
        }
    }
}

/**
 * @brief Handles RPM changed events
 *
 * This function processes events related to the RPM changed event. When the
 * RPM changes in the idle mode, the board mode transitions to the riding
 * mode if the RPM is non-zero. Conversely, when the RPM changes in the
 * riding mode, the board mode transitions to the idle mode if the RPM is
 * zero.
 *
 * @param event The type of event that occurred, specified as an event_type_t
 *              enumeration
 * @param rpm The new RPM value, specified as a uint32_t
 */
EVENT_HANDLER(board_mode, rpm_changed)
{
    switch (board_mode)
    {
    case BOARD_MODE_IDLE:
        if (abs(data->rpm) > 0)
        {
            update_riding_submode();
        }
        break;
    case BOARD_MODE_RIDING:
        if (abs(data->rpm) == 0 && footpads_get_state() == NONE_FOOTPAD)
        {
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_ACTIVE);
        }
        else
        {
            update_riding_submode();
        }
        break;
    default:
        // Nothing to do in this mode
        break;
    }
}

/**
 * @brief Handles duty cycle changed events
 *
 * This function processes events related to the duty cycle changed event. When
 * the duty cycle changes in the riding mode, the board mode transitions to
 * one of the submodes based on the duty cycle value.
 *
 * @param event The type of event that occurred, specified as an event_type_t
 *              enumeration
 * @param duty_cycle The new duty cycle value, specified as a float
 */
EVENT_HANDLER(board_mode, duty_cycle_changed)
{
    if (board_mode == BOARD_MODE_RIDING)
    {
        update_riding_submode();
    }
    // Else: Nothing to do in this mode
}

/**
 * @brief Handles emergency fault events
 *
 * This function processes events related to the emergency fault event.
 * When an emergency fault occurs, the board mode transitions to the
 * fault mode.
 *
 * @param event The type of event that occurred, specified as an event_type_t
 *              enumeration
 * @param value The value associated with the event, specified as a uint32_t
 */
EVENT_HANDLER(board_mode, emergency_fault)
{
    set_board_mode(BOARD_MODE_FAULT, BOARD_SUBMODE_UNDEFINED);
}

/**
 * @brief Handles footpad state changes
 *
 * This function processes events related to changes in the footpad state.
 * When a footpad state change occurs, the board mode transitions to the
 * riding mode if the footpads are occupied and the board is in the idle
 * mode. Conversely, when a footpad state change occurs and the footpads
 * are unoccupied, the board mode transitions to the idle mode if the
 * board is in the riding mode.
 *
 * @param value The new footpads state, specified as a uint32_t which is
 *              casted to footpads_state_t
 */
EVENT_HANDLER(board_mode, footpad_changed)
{
    switch (board_mode)
    {
    case BOARD_MODE_IDLE:
        if (board_submode != BOARD_SUBMODE_IDLE_CONFIG)
        {
            // Rider has stepped on board
            if (data->footpads_state != NONE_FOOTPAD)
            {
                update_riding_submode();
            }
        }
        break;

    case BOARD_MODE_RIDING:
        // Rider has stepped off board and RPM is zero
        if (data->footpads_state == NONE_FOOTPAD && vesc_serial_get_rpm() == 0)
        {
            set_board_mode(BOARD_MODE_IDLE, BOARD_SUBMODE_IDLE_ACTIVE);
        }
        break;

    default:
        // Nothing to do in this mode
        break;
    }
}