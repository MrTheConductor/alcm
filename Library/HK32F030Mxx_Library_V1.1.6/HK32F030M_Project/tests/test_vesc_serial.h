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
#ifndef TEST_VESC_SERIAL_H
#define TEST_VESC_SERIAL_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "vesc_serial.h"

int vesc_serial_setup(void **state)
{
    (void)state; // Unused

    // Reset event queue and timer
    event_queue_init();
    timer_init();

    // Expect init to call the vesc_serial_hw_init function and subscribe
    // to the vesc serial data event
    expect_any(vesc_serial_hw_init, baud);
    expect_value(subscribe_event, event, EVENT_SERIAL_DATA_RX);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    vesc_serial_init();

    // Expect ring buffer to be initialized and empty
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();
    assert_non_null(rx_buffer);
    assert_true(ring_buffer_is_empty(rx_buffer));
    return 0;
}

void test_vesc_serial_timer(void **state)
{
    (void)state; // Unused

    // When the board mode changes to booting, the repeating vesc serial timer
    // should be set
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_BOOTING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // When the board mode changes to idle, the repeating vesc serial timer
    // should keep running
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Unless the board turns off, then the timer should be cancelled
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    data.board_mode.mode = BOARD_MODE_OFF;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

void test_vesc_serial_timer_callback(void **state)
{
    (void)state; // Unused

    // When the board mode changes to booting, the repeating vesc serial timer
    // should be set
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_BOOTING;
    data.board_mode.submode = BOARD_SUBMODE_UNDEFINED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Timer callback should trigger polling
    expect_any(vesc_serial_hw_send, data);
    expect_any(vesc_serial_hw_send, len);
    call_timer_callback(1, 100);
}

void test_vesc_serial_missing_start_byte(void **state)
{
    (void)state; // Unused

    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    // fill the buffer with garbage
    for (int i = 0; i < 10; i++)
    {
        ring_buffer_push(rx_buffer, 0x00);
    }

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_missing_length(void **state)
{
    (void)state; // Unused

    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02);

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_length_too_big(void **state)
{
    (void)state; // Unused

    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0xff);

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_payload_too_short(void **state)
{
    (void)state; // Unused
    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x03);
    ring_buffer_push(rx_buffer, 0x00);

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_crc_missing(void **state)
{
    (void)state; // Unused
    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02); // Start
    ring_buffer_push(rx_buffer, 0x01); // Length
    ring_buffer_push(rx_buffer, 0x00); // command ID
    // next two bytes should be the CRC

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));

    ring_buffer_push(rx_buffer, 0x02); // Start
    ring_buffer_push(rx_buffer, 0x01); // Length
    ring_buffer_push(rx_buffer, 0x00); // command ID
    ring_buffer_push(rx_buffer, 0x00); // crc-low

    // call the RX_DATA event
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_missing_end_byte(void **state)
{
    (void)state; // Unused
    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_crc_invalid(void **state)
{
    (void)state; // Unused
    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x03);

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_unknown_command(void **state)
{
    (void)state; // Unused
    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x03);

    // Since this is the first valid packet, (even though it is an unknown
    // command), it should still set the VESC to alive
    expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
    expect_any(event_queue_push, data);

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));

    // No VESC alive event after the first one
    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x03);

    // call the RX_DATA event
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);

    // should consume all the data in the buffer
    assert_true(ring_buffer_is_empty(rx_buffer));
}

void test_vesc_serial_comm_setup_wrong_size(void **state)
{
    (void)state; // Unused
    // get the ring buffer
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x33);
    ring_buffer_push(rx_buffer, 0x06);
    ring_buffer_push(rx_buffer, 0x30);
    ring_buffer_push(rx_buffer, 0x03);

    // Since this is the first valid packet, (even though it is the wrong
    // size), it should still set the VESC to alive
    expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
    expect_any(event_queue_push, data);

    // Should get a fault because this is the wrong length
    expect_value(fault, fault, EMERGENCY_FAULT_INVALID_LENGTH);

    // call the RX_DATA event
    event_data_t data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &data);
}

const struct CMUnitTest vesc_serial_tests[] = {
    cmocka_unit_test_setup(test_vesc_serial_timer, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_timer_callback, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_missing_start_byte, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_missing_length, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_length_too_big, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_payload_too_short, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_crc_missing, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_missing_end_byte, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_crc_invalid, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_unknown_command, vesc_serial_setup),
    cmocka_unit_test_setup(test_vesc_serial_comm_setup_wrong_size, vesc_serial_setup),
};

#endif