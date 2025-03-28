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
#ifndef TEST_RING_BUFFER_H
#define TEST_RING_BUFFER_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "ring_buffer.h"

// Helper function to initialize a test ring buffer
static void initialize_ring_buffer(ring_buffer_t *buf, uint8_t *buffer, uint16_t size)
{
    buf->buffer = buffer;
    buf->size = size;
    buf->read_idx = 0;
    buf->write_idx = 0;
}

// Test: Verify that a new buffer is empty
static void test_ring_buffer_is_empty_on_init(void **state)
{
    uint8_t buffer[8];
    ring_buffer_t ring_buf;
    initialize_ring_buffer(&ring_buf, buffer, sizeof(buffer));
    assert_true(ring_buffer_is_empty(&ring_buf));
}

// Test: Verify that pushing to an empty buffer works
static void test_ring_buffer_push_to_empty(void **state)
{
    uint8_t buffer[8];
    ring_buffer_t ring_buf;
    initialize_ring_buffer(&ring_buf, buffer, sizeof(buffer));

    assert_true(ring_buffer_push(&ring_buf, 42));
    assert_false(ring_buffer_is_empty(&ring_buf));
}

// Test: Verify that popping from a buffer works
static void test_ring_buffer_pop_from_non_empty(void **state)
{
    uint8_t buffer[8];
    ring_buffer_t ring_buf;
    initialize_ring_buffer(&ring_buf, buffer, sizeof(buffer));

    ring_buffer_push(&ring_buf, 42);
    uint8_t value;
    assert_true(ring_buffer_pop(&ring_buf, &value));
    assert_int_equal(value, 42);
    assert_true(ring_buffer_is_empty(&ring_buf));
}

// Test: Verify full condition
static void test_ring_buffer_full_condition(void **state)
{
    uint8_t buffer[8];
    ring_buffer_t ring_buf;
    initialize_ring_buffer(&ring_buf, buffer, sizeof(buffer));

    for (int i = 0; i < 8 - 1; i++)
    { // One less than size to prevent overwrite
        assert_true(ring_buffer_push(&ring_buf, i));
    }
    assert_true(ring_buffer_is_full(&ring_buf));
    assert_false(ring_buffer_push(&ring_buf, 99)); // Should fail
}

// Test: Verify circular behavior
static void test_ring_buffer_wraparound(void **state)
{
    uint8_t buffer[4];
    ring_buffer_t ring_buf;
    initialize_ring_buffer(&ring_buf, buffer, sizeof(buffer));

    for (int i = 0; i < 4 - 1; i++)
    { // Fill the buffer
        assert_true(ring_buffer_push(&ring_buf, i));
    }

    uint8_t value;
    assert_true(ring_buffer_pop(&ring_buf, &value));
    assert_int_equal(value, 0);

    assert_true(ring_buffer_push(&ring_buf, 99)); // Should wrap around
    assert_true(ring_buffer_pop(&ring_buf, &value));
    assert_int_equal(value, 1);
}

// Test: Verify empty condition after wraparound
static void test_ring_buffer_empty_after_wraparound(void **state)
{
    uint8_t buffer[4];
    ring_buffer_t ring_buf;
    initialize_ring_buffer(&ring_buf, buffer, sizeof(buffer));

    for (int i = 0; i < 4 - 1; i++)
    { // Fill the buffer
        assert_true(ring_buffer_push(&ring_buf, i));
    }

    uint8_t value;
    for (int i = 0; i < 4 - 1; i++)
    {
        assert_true(ring_buffer_pop(&ring_buf, &value));
        assert_int_equal(value, i);
    }

    assert_true(ring_buffer_is_empty(&ring_buf));
}

const struct CMUnitTest ring_buffer_tests[] = {
    cmocka_unit_test(test_ring_buffer_is_empty_on_init),
    cmocka_unit_test(test_ring_buffer_push_to_empty),
    cmocka_unit_test(test_ring_buffer_pop_from_non_empty),
    cmocka_unit_test(test_ring_buffer_full_condition),
    cmocka_unit_test(test_ring_buffer_wraparound),
    cmocka_unit_test(test_ring_buffer_empty_after_wraparound),
};

#endif