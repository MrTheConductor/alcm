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
#include <string.h>

#include "event_queue.h"
#include "config.h"
#include "interrupts.h"

/**
 * @brief Event queue data structure
 */
typedef struct
{
    event_type_t event; // The type of event
    void (*callback)(
        event_type_t,
        const event_data_t *); // The callback function to be called when the event occurs
    uint8_t next;              // The next subscriber in the list
} subscriber_struct_t;

// Note: These are volatile because ISRs can push events to the queue
static volatile uint8_t event_queue_head = 0U;
static volatile uint8_t event_queue_tail = 0U;
static volatile event_struct_t event_queue[EVENT_QUEUE_SIZE] = {0};

/**
 * @breif The subscriber list
 *
 * @note This is a static array of subscribers. The first NUMBER_OF_EVENTS
 *       subscribers are the first subscribers for each event. The rest are
 *       free for additional subscribers.
 */
static subscriber_struct_t subscribers[(uint8_t)NUMBER_OF_EVENTS + MAX_SUBSCRIPTIONS] = {0};
static uint8_t next_subscriber_index = (uint8_t)NUMBER_OF_EVENTS;

/**
 * @brief Initializes the event queue and subscriber list.
 */
lcm_status_t event_queue_init(void)
{
    event_queue_head = 0U;
    event_queue_tail = 0U;
    next_subscriber_index = (uint8_t)NUMBER_OF_EVENTS;
    memset((void *)event_queue, 0, sizeof(event_struct_t) * EVENT_QUEUE_SIZE);
    memset((void *)subscribers, 0,
           sizeof(subscriber_struct_t) * ((uint8_t)NUMBER_OF_EVENTS + MAX_SUBSCRIPTIONS));

    return (LCM_SUCCESS);
}

/**
 * @brief Checks if the event queue is full.
 */
bool_t is_queue_full(void)
{
    uint8_t next_tail = (event_queue_tail + 1U) % EVENT_QUEUE_SIZE; // Calculate next tail position
    return (next_tail == event_queue_head);                         // Compare with the head
}

/**
 * @brief Checks if the event queue is empty.
 */
bool_t is_queue_empty(void)
{
    uint8_t head = event_queue_head; // Store the current value of event_queue_head
    uint8_t tail = event_queue_tail; // Store the current value of event_queue_tail
    return (head == tail);           // Compare the stable values
}

/**
 * @brief   Pushes an event to the event queue.
 */
lcm_status_t event_queue_push(event_type_t event, const event_data_t *data)
{
    lcm_status_t status = LCM_ERROR;

    interrupts_disable();
    // if the queue is not full and the event is valid
    if ((!is_queue_full()) && (event < NUMBER_OF_EVENTS) && (event != EVENT_NULL))
    {
        // copy event to event queue
        event_queue[event_queue_tail].event = event;
        event_data_t *event_data = (event_data_t *)&(event_queue[event_queue_tail].data);

        if (data != NULL)
        {
            // copy data to event queue
            memcpy((void *)event_data, (const void *)data, sizeof(event_data_t));
        }
        else
        {
            // clear data
            memset((void *)event_data, 0, sizeof(event_data_t));
        }
        event_queue_tail = (event_queue_tail + 1U) % EVENT_QUEUE_SIZE;
        status = LCM_SUCCESS;
    }

    interrupts_enable();
    return status;
}

/**
 * @brief Pushes an emergency fault event to the event queue.
 */
void fault(emergency_fault_t fault)
{
    event_data_t event_data;
    event_data.emergency_fault = fault;

    // push the emergency fault event to the queue, ignore
    // the return value as we're already in an emergency fault state
    (void)event_queue_push(EVENT_EMERGENCY_FAULT, &event_data);
}

/**
 * @brief Notifies all subscribers of a given event.
 */
lcm_status_t notify_subscribers(volatile const event_struct_t *event)
{
    lcm_status_t status = LCM_SUCCESS;
    uint8_t index = 0U;

    if (event != NULL)
    {
        index = (uint8_t)event->event;
        while ((index != 0U) && (index < ((uint8_t)NUMBER_OF_EVENTS + MAX_SUBSCRIPTIONS)))
        {
            if (subscribers[index].callback != NULL)
            {
                subscribers[index].callback(event->event, (const event_data_t *)&event->data);
            }
            index = subscribers[index].next;
        }
    }
    else
    {
        status = LCM_ERROR;
    }

    return status;
}

/**
 * @brief Pops the next event from the event queue and notifies all subscribers.
 */
lcm_status_t event_queue_pop_and_notify(void)
{
    lcm_status_t status = LCM_ERROR;

    if (!is_queue_empty())
    {
        if (LCM_SUCCESS == notify_subscribers(&event_queue[event_queue_head]))
        {
            event_queue_head = (event_queue_head + 1U) % EVENT_QUEUE_SIZE;
            status = LCM_SUCCESS;
        }
        // No else needed, status is already LCM_ERROR
    }
    else
    {
        status = LCM_QUEUE_EMPTY;
    }

    return status;
}

/**
 * @brief   Subscribes to an event, allowing a module to be notified when the
 *          event occurs.
 */
lcm_status_t subscribe_event(event_type_t event,
                             void (*callback)(event_type_t event, const event_data_t *data))
{
    lcm_status_t status = LCM_SUCCESS;

    // Check if the event is valid and the callback is not NULL
    if ((event < NUMBER_OF_EVENTS) && (callback != NULL) && (event != EVENT_NULL))
    {
        if (subscribers[event].callback == NULL)
        {
            // Slot is empty, populate it
            subscribers[event].event = event;
            subscribers[event].callback = callback;
            subscribers[event].next = 0U;
        }
        else
        {
            // Find the next available subscriber index
            if (next_subscriber_index < ((uint8_t)NUMBER_OF_EVENTS + MAX_SUBSCRIPTIONS))
            {
                uint8_t current_index = (uint8_t)event;
                while (subscribers[current_index].next != 0U)
                {
                    current_index = subscribers[current_index].next;
                }

                // Populate the new subscriber slot
                subscribers[next_subscriber_index].event = event;
                subscribers[next_subscriber_index].callback = callback;
                subscribers[next_subscriber_index].next = 0U;

                // Link the last subscriber to the new one
                subscribers[current_index].next = next_subscriber_index;

                // Increment the next available subscriber index
                next_subscriber_index++;
            }
            else
            {
                // Trigger system-wide fault
                fault(EMERGENCY_FAULT_OVERFLOW);
                status = LCM_ERROR;
            }
        }
    }
    else
    {
        status = LCM_ERROR;
    }

    return status;
}

/**
 * @brief Returns the number of events in the event queue.
 */
uint8_t event_queue_get_num_events(void)
{
    uint8_t num_events = 0U;
    uint8_t head = event_queue_head;
    uint8_t tail = event_queue_tail;

    if (head <= tail)
    {
        num_events = (uint8_t)(tail - head);
    }
    else
    {
        num_events = (EVENT_QUEUE_SIZE - head) + tail;
    }

    return num_events;
}

/**
 * @brief Returns the maximum number of items in the event queue.
 */
uint8_t event_queue_get_max_items(void)
{
    // return the size of the queue minus 1 as the queue is circular
    return EVENT_QUEUE_SIZE - 1U;
}

// New line at EOF
