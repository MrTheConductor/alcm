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
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "event_queue.h"
#include "lcm_types.h"
#include "config.h"

void test_event_queue_pop_from_empty_queue(void **state)
{
    (void)state;

    assert_int_equal(event_queue_pop_and_notify(), LCM_QUEUE_EMPTY);
    assert_int_equal(event_queue_get_num_events(), 0);
}

void test_event_queue_push(void **state)
{
    (void)state;

    event_data_t data = {0};
    data.system_tick = 999;

    // Pushing an event should disable interrupts, push the event, enable interrupts
    expect_function_call(interrupts_disable);
    expect_function_call(interrupts_enable);
    assert_int_equal(event_queue_push(EVENT_SYS_TICK, &data), LCM_SUCCESS);
    assert_int_equal(event_queue_get_num_events(), 1);

    // Pop it
    assert_int_equal(event_queue_pop_and_notify(), LCM_SUCCESS);
    assert_int_equal(event_queue_get_num_events(), 0);
}

void test_event_queue_push_null_data(void **state)
{
    (void)state;

    // Pushing an event should disable interrupts, push the event, enable interrupts
    expect_function_call(interrupts_disable);
    expect_function_call(interrupts_enable);
    assert_int_equal(event_queue_push(EVENT_SYS_TICK, NULL), LCM_SUCCESS);
    assert_int_equal(event_queue_get_num_events(), 1);

    // Pop it
    assert_int_equal(event_queue_pop_and_notify(), LCM_SUCCESS);
    assert_int_equal(event_queue_get_num_events(), 0);
}

void callback(event_type_t event, const event_data_t *data)
{
    check_expected(event);
    check_expected_ptr(data);
}

int validate_event_data(uintmax_t data, uintmax_t check_data)
{
    return ((event_data_t *)data)->system_tick == ((event_data_t *)check_data)->system_tick;
}

void test_event_queue_subscribe_and_notify(void **state)
{
    (void)state;

    assert_int_equal(subscribe_event(EVENT_SYS_TICK, callback), LCM_SUCCESS);

    event_data_t data = {0};
    data.system_tick = 999;

    // Pushing an event should disable interrupts, push the event, enable interrupts
    expect_function_call(interrupts_disable);
    expect_function_call(interrupts_enable);
    assert_int_equal(event_queue_push(EVENT_SYS_TICK, &data), LCM_SUCCESS);

    expect_value(callback, event, EVENT_SYS_TICK);
    expect_check(callback, data, validate_event_data, (uintmax_t)&data);

    assert_int_equal(event_queue_pop_and_notify(), LCM_SUCCESS);
}

void test_event_queue_full(void **state)
{
    (void)state;

    for (uint8_t i = 0; i < event_queue_get_max_items(); i++)
    {
        event_data_t data = {0};
        data.system_tick = i;

        // Pushing an event should disable interrupts, push the event, enable interrupts
        expect_function_call(interrupts_disable);
        expect_function_call(interrupts_enable);
        assert_int_equal(event_queue_push(EVENT_SYS_TICK, &data), LCM_SUCCESS);
        assert_int_equal(event_queue_get_num_events(), i + 1);
    }

    // Try to push one more should produce an error
    event_data_t data = {0};
    data.system_tick = 999;

    // Pushing an event should disable interrupts, push the event, enable interrupts
    expect_function_call(interrupts_disable);
    expect_function_call(interrupts_enable);
    assert_int_equal(event_queue_push(EVENT_SYS_TICK, &data), LCM_ERROR);

    // Pop everything off the queue - systick should be in FIFO order
    for (uint8_t i = 0; i < event_queue_get_max_items(); i++)
    {
        event_data_t data = {0};
        data.system_tick = i;
        expect_value(callback, event, EVENT_SYS_TICK);
        expect_check(callback, data, validate_event_data, (uintmax_t)&data);
        assert_int_equal(event_queue_pop_and_notify(), LCM_SUCCESS);
        assert_int_equal(event_queue_get_num_events(), event_queue_get_max_items() - i - 1);
    }
}

void test_event_queue_fault(void **state)
{
    (void)state;
    // Pushing an event should disable interrupts, push the event, enable interrupts
    expect_function_call(interrupts_disable);
    expect_function_call(interrupts_enable);
    fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
}

void test_event_queue_subscribers_full(void **state)
{
    (void)state;

    event_queue_init();

    for (uint8_t i = 0; i < MAX_SUBSCRIPTIONS + 1; i++)
    {
        assert_int_equal(subscribe_event(EVENT_SYS_TICK, callback), LCM_SUCCESS);
    }

    // Try to subscribe one more should produce an error and push a fault
    expect_function_call(interrupts_disable);
    expect_function_call(interrupts_enable);
    assert_int_equal(subscribe_event(EVENT_SYS_TICK, callback), LCM_ERROR);
}

const struct CMUnitTest event_queue_tests[] = {
    cmocka_unit_test(test_event_queue_pop_from_empty_queue),
    cmocka_unit_test(test_event_queue_push),
    cmocka_unit_test(test_event_queue_push_null_data),
    cmocka_unit_test(test_event_queue_subscribe_and_notify),
    cmocka_unit_test(test_event_queue_full),
    cmocka_unit_test(test_event_queue_fault),
    cmocka_unit_test(test_event_queue_subscribers_full),
};
