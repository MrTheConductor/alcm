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
#include <string.h>
#include <cmocka.h>
#include <assert.h>

#include "mock_event_queue.h"

lcm_status_t event_queue_push(event_type_t event, event_data_t* data)
{
    check_expected(event);
    check_expected_ptr(data);
    return (LCM_SUCCESS);
}

typedef struct
{
    event_type_t event;
    void (*callback)(event_type_t, const event_data_t*);
}
subscriber_struct_t;

#define MAX_SUBSCRIPTIONS 32

// Mock subscription list
static subscriber_struct_t subscriptions[MAX_SUBSCRIPTIONS];
static uint8_t subscription_count = 0;

lcm_status_t event_queue_init(void)
{
    subscription_count = 0;
    memset(subscriptions, 0, sizeof(subscriptions));
    return (LCM_SUCCESS);
}

lcm_status_t subscribe_event(event_type_t event, void (*callback) (event_type_t event, const event_data_t* data))
{
    check_expected(event);
    check_expected_ptr(callback);

    if (subscription_count < MAX_SUBSCRIPTIONS)
    {
        subscriptions[subscription_count].event = event;
        subscriptions[subscription_count].callback = callback;
        subscription_count++;
        return (LCM_SUCCESS);
    }
    return (LCM_ERROR);
}

void fault(emergency_fault_t fault)
{
    check_expected(fault);
}

lcm_status_t event_queue_pop_and_notify(void)
{
    function_called();
    return (LCM_SUCCESS);
}

uint8_t event_queue_get_num_events(void)
{
    return (subscription_count);
}

uint8_t event_queue_get_max_items(void)
{
    return (MAX_SUBSCRIPTIONS);
}

// Helper function to invoke the mocked callback
void event_queue_call_mocked_callback(event_type_t event, const event_data_t* data)
{
    assert(data != NULL);
    for (int i = 0; i < subscription_count; i++)
    {
        if (subscriptions[i].event == event)
        {
            subscriptions[i].callback(event, data);
        }
    }
}

void event_queue_test_bad_event(event_type_t expected, event_type_t actual, const event_data_t* data)
{
    for (int i = 0; i < subscription_count; i++)
    {
        if (subscriptions[i].event == expected)
        {
            subscriptions[i].callback(actual, data);
        }
    }
}

// Validation functions
int validate_footpads_state(const uintmax_t data, const uintmax_t check_data)
{
    const event_data_t *received_data = (const event_data_t *)(uintptr_t)data;
    const uint8_t *expected_state = (const uint8_t *)(uintptr_t)check_data;

    assert_int_equal(received_data->footpads_state, *expected_state);
    return 1;
}

int validate_board_mode_event_data(const uintmax_t data, const uintmax_t check_data)
{
    const event_data_t *received_data = (const event_data_t *)(uintptr_t)data;
    const  board_mode_event_data_t *expected_state = (const board_mode_event_data_t *)(uintptr_t)check_data;

    assert_int_equal(received_data->board_mode.mode, expected_state->mode);
    assert_int_equal(received_data->board_mode.submode, expected_state->submode);
    return 1;
}