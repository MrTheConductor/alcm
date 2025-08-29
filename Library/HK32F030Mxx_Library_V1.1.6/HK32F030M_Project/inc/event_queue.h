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
#ifndef _EVENT_QUEUE_H
#define _EVENT_QUEUE_H

#include <stdint.h>

#include "board_mode.h"
#include "command_processor.h"
#include "config.h"
#include "footpads.h"
#include "lcm_types.h"

#define EVENT_HANDLER_NAME(module, e) module##_##e##_event_handler
#define EVENT_HANDLER(module, e)                                                                   \
    void EVENT_HANDLER_NAME(module, e)(event_type_t event, const event_data_t *data)
#define SUBSCRIBE_EVENT(module, enum, e)                                                           \
    if (LCM_SUCCESS != subscribe_event(enum, EVENT_HANDLER_NAME(module, e)))                       \
    status = LCM_ERROR

typedef enum
{
    EMERGENCY_FAULT_UNDEFINED = 0,
    EMERGENCY_FAULT_NULL_POINTER,
    EMERGENCY_FAULT_OUT_OF_BOUNDS,
    EMERGENCY_FAULT_DIVIDE_BY_ZERO,
    EMERGENCY_FAULT_OVERFLOW,
    EMERGENCY_FAULT_UNDERFLOW,
    EMERGENCY_FAULT_INVALID_ARGUMENT,
    EMERGENCY_FAULT_INVALID_STATE,
    EMERGENCY_FAULT_INVALID_EVENT,
    EMERGENCY_FAULT_INVALID_LENGTH,
    EMERGENCY_FAULT_VESC,
    EMERGENCY_FAULT_VESC_COMM_TIMEOUT,
    EMERGENCY_FAULT_INIT_FAIL,
    EMERGENCY_FAULT_UNEXPECTED_ERROR
} emergency_fault_t;

typedef enum
{
    EVENT_NULL = 0,

    // System tick
    EVENT_SYS_TICK,

    // Button events
    EVENT_BUTTON_WAKEUP,
    EVENT_BUTTON_DOWN,
    EVENT_BUTTON_UP,
    EVENT_BUTTON_CLICK,
    EVENT_BUTTON_HOLD,

    // Footpad events
    EVENT_FOOTPAD_CHANGED,

    // State change events
    EVENT_BOARD_MODE_CHANGED,

    // Serial data
    EVENT_SERIAL_DATA_RX,

    // VESC events
    EVENT_DUTY_CYCLE_CHANGED,
    EVENT_RPM_CHANGED,
    EVENT_BATTERY_LEVEL_CHANGED,
    EVENT_VESC_ALIVE,
    
    // IMU events
#if defined(ENABLE_PITCH_EVENTS)
    EVENT_IMU_PITCH_CHANGED,
#endif
#if defined(ENABLE_ROLL_EVENTS)
    EVENT_IMU_ROLL_CHANGED,
#endif

    // Command events
    EVENT_COMMAND_CONTEXT_CHANGED,
    EVENT_COMMAND_TOGGLE_LIGHTS,
    EVENT_COMMAND_TOGGLE_BEEPER,
    EVENT_COMMAND_BOOT,
    EVENT_COMMAND_SHUTDOWN,
    EVENT_COMMAND_ACK,
    EVENT_COMMAND_ACK_2,
    EVENT_COMMAND_NACK,
    EVENT_COMMAND_SETTINGS_CHANGED,
    EVENT_COMMAND_MODE_CONFIG,
    EVENT_EMERGENCY_FAULT,

    // Must be last
    NUMBER_OF_EVENTS
} event_type_t;

/**
 * @brief Data structure for board mode change event
 *
 * This structure holds the current board mode and submode, which is used
 * to track the state of the board.
 */
typedef struct
{
    board_mode_t mode;
    board_mode_t previous_mode;
    board_submode_t submode;
    board_submode_t previous_submode;
} board_mode_event_data_t;

typedef struct
{
    uint32_t time; // Timestamp of the event
} button_event_data_t;

/**
 * @union event_data_t
 * @brief A union to represent different types of event data.
 *
 * This union encapsulates various types of event data that can be used in the system.
 * It allows different types of data to be stored in the same memory location,
 * depending on the event type.
 */
typedef union {
    uint32_t system_tick;
    board_mode_event_data_t board_mode;
    footpads_state_t footpads_state;
    emergency_fault_t emergency_fault;
    button_event_data_t button_data;
    float32_t duty_cycle;
    int32_t rpm;
    float32_t voltage;
    float32_t battery_level;
    uint8_t vesc_fault;
    uint8_t click_count;
    command_processor_context_t context;
    bool_t enable;
    float32_t imu_pitch;
    float32_t imu_roll;
} event_data_t;

/**
 * @brief Event queue data structure
 *
 * A queue of events that can be posted to any module.
 */
typedef struct
{
    event_type_t event;
    event_data_t data;
} event_struct_t;

/**
 * @brief Initializes the event queue
 *
 * This function initializes the event queue and prepares it for use.
 */
lcm_status_t event_queue_init(void);

/**
 * @brief Pushes an event onto the event queue.
 *
 * This function adds an event to the event queue with the specified event type and associated data.
 *
 * @param event The type of event to be pushed onto the queue.
 * @param data Pointer to the data associated with the event.
 * @return lcm_status_t Status of the operation.
 */
lcm_status_t event_queue_push(event_type_t event, const event_data_t *data);

/**
 * @brief Subscribes a callback function to a specific event type.
 *
 * This function allows a user to register a callback function that will be
 * called whenever the specified event occurs. The callback function will
 * receive the event type and associated event data as parameters.
 *
 * @param event The type of event to subscribe to.
 * @param callback The function to be called when the event occurs. The callback
 *                 function should take two parameters: the event type and a
 *                 pointer to the event data.
 *
 * @return lcm_status_t Status of the subscription operation.
 */
lcm_status_t subscribe_event(event_type_t event,
                             void (*callback)(event_type_t event, const event_data_t *data));

/**
 * @brief Handles an emergency fault.
 *
 * This function is called to handle an emergency fault condition.
 *
 * @param fault The type of emergency fault to handle.
 */
void fault(emergency_fault_t fault);

/**
 * @brief Pops an event from the event queue and notifies the relevant handler.
 *
 * This function removes the next event from the event queue and triggers the
 * appropriate notification or handler for that event. It ensures that events
 * are processed in the order they were added to the queue.
 *
 * @note This function should only be called by the main event loop or a similar
 *      mechanism that is responsible for processing events.
 *
 * @return lcm_status_t Returns the status of the operation. Possible values
 *                      include success, failure, or specific error codes
 *                      defined in lcm_status_t.
 */
lcm_status_t event_queue_pop_and_notify(void);

/**
 * @brief Gets the number of events currently in the event queue
 *
 * @return The number of events in the queue
 */
uint8_t event_queue_get_num_events(void);

/**
 * @brief Gets the maximum number of items that can be stored in the event queue
 *
 * @return The maximum number of items that can be stored in the queue
 */
uint8_t event_queue_get_max_items(void);

#endif