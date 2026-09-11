/*
 * Copyright (c) 2024-2026, Mitchell White <mitchell.n.white@gmail.com>
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
#include "crc16_ccitt.h"
#include "settings.h"
#include "command_processor.h"
#include "battery_lut.h"
#include "battery_lut_hw.h"
#include "test_battery_lut.h" // reuses test_battery_lut_make_valid_block()

int vesc_serial_setup(void **state)
{
    (void)state; // Unused

    // Reset event queue and timer
    event_queue_init();
    timer_init();

    // Expect init to call the vesc_serial_hw_init function and subscribe
    // to the vesc serial data event
    expect_any(vesc_serial_hw_init, baud);
    // No LUT block patched in for these tests - vesc_serial_init() should
    // fall back to the VESC's own battery_level, unchanged. Tests that
    // specifically want a valid LUT (see
    // test_vesc_serial_battery_lut_overrides_battery_level below) call
    // vesc_serial_init() directly instead of this shared setup.
    will_return(battery_lut_hw_get_block, NULL);
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

/**
 * @brief Exercises a successful COMM_GET_VALUES_SETUP_SELECTIVE decode with
 * no LUT patched in - the VESC's own battery_level must pass through
 * unchanged. This is the default/happy-path case for every rider who never
 * runs tools/battery_lut_patch.
 */
void test_vesc_serial_comm_setup_decodes_values(void **state)
{
    (void)state; // Unused
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    // current=500 (5.00A), duty_cycle=250 (25.0%), rpm=1000,
    // input_voltage=390 (39.0V), battery_level=999 (99.9%, the VESC's own
    // raw value), fault=0. current isn't used when the LUT is invalid
    // (the default here, per vesc_serial_setup) but is included with a
    // real nonzero value to prove it decodes without disturbing anything
    // else.
    uint8_t payload[] = {0x33, 0x00, 0x01, 0x01, 0xb8,
                          0x00, 0x00, 0x01, 0xf4,
                          0x00, 0xfa,
                          0x00, 0x00, 0x03, 0xe8,
                          0x01, 0x86,
                          0x03, 0xe7,
                          0x00};

    expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_DUTY_CYCLE_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_RPM_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_VOLTAGE_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_BATTERY_LEVEL_CHANGED);
    int16_t expected_battery_level = 999;
    expect_check(event_queue_push, data, validate_battery_level_event_data,
                 (uintmax_t)&expected_battery_level);

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
    for (size_t i = 0; i < sizeof(payload); i++)
    {
        ring_buffer_push(rx_buffer, payload[i]);
    }
    ring_buffer_push(rx_buffer, 0x68);
    ring_buffer_push(rx_buffer, 0xb6);
    ring_buffer_push(rx_buffer, 0x03);

    event_data_t rx_event_data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);

    assert_int_equal(999, vesc_serial_get_battery_level());
    assert_true(ring_buffer_is_empty(rx_buffer));
}

/**
 * @brief With a valid, patched LUT block, the published battery_level must
 * come from the LUT's interpolation of the IR-compensated,
 * EMA-smoothed input voltage - not the VESC's own raw battery_level on
 * the wire, and not the raw (uncompensated) voltage either.
 *
 * Uses its own inline setup (rather than vesc_serial_setup) because it
 * needs to supply a valid block to battery_lut_hw_get_block() during
 * vesc_serial_init(), instead of the shared setup's NULL default.
 */
void test_vesc_serial_battery_lut_overrides_battery_level(void **state)
{
    (void)state; // Unused

    event_queue_init();
    timer_init();

    static battery_lut_block_t block; // static: outlives this function
    block = test_battery_lut_make_valid_block(); // r_int_milliohms = 100 (0.1 ohm)

    expect_any(vesc_serial_hw_init, baud);
    will_return(battery_lut_hw_get_block, &block); // consumed by battery_lut_init()
    expect_value(subscribe_event, event, EVENT_SERIAL_DATA_RX);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    vesc_serial_init();

    // battery_lut_get_voltage_compensation() and battery_lut_get_percent()
    // each re-read the block on every call (no RAM caching - see
    // battery_lut.c), so the packet-processing below needs one queued
    // value per call, in addition to the one already consumed by init.
    will_return(battery_lut_hw_get_block, &block); // compensation
    will_return(battery_lut_hw_get_block, &block); // LUT lookup

    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    // raw battery_level=999 (must NOT be what publishes), current=1000
    // (10.00A discharge), raw input_voltage=380 (38.0V). At 0.1 ohm, 10A
    // compensates +1.0V -> 39.0V, which the LUT (see
    // test_battery_lut_make_valid_block) interpolates to 70.0% (700
    // tenths) on a 10S pack. The raw, uncompensated 38.0V would instead
    // interpolate to 60.0% - asserting 700 here only passes if
    // compensation actually ran.
    uint8_t payload[] = {0x33, 0x00, 0x01, 0x01, 0xb8,
                          0x00, 0x00, 0x03, 0xe8,
                          0x00, 0xfa,
                          0x00, 0x00, 0x03, 0xe8,
                          0x01, 0x7c,
                          0x03, 0xe7,
                          0x00};

    expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_DUTY_CYCLE_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_RPM_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_VOLTAGE_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_BATTERY_LEVEL_CHANGED);
    int16_t expected_battery_level = 700;
    expect_check(event_queue_push, data, validate_battery_level_event_data,
                 (uintmax_t)&expected_battery_level);

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
    for (size_t i = 0; i < sizeof(payload); i++)
    {
        ring_buffer_push(rx_buffer, payload[i]);
    }
    ring_buffer_push(rx_buffer, 0x34);
    ring_buffer_push(rx_buffer, 0xba);
    ring_buffer_push(rx_buffer, 0x03);

    event_data_t rx_event_data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);

    assert_int_equal(700, vesc_serial_get_battery_level());
    assert_true(ring_buffer_is_empty(rx_buffer));
}

/**
 * @brief Exercises refloat's COMMAND_LCM_POLL app-integration relay.
 *
 * Covers: baseline establishment on the first poll reply (no apply),
 * a repeated/unchanged reply (no apply, "sticks"), a changed headlight
 * brightness (applied + event, status untouched), a changed status
 * brightness (applied + event, headlight untouched), a too-short reply,
 * and a reply with the wrong refloat package id (both ignored, no crash,
 * no events, previously-applied values untouched).
 */
void test_vesc_serial_app_integration(void **state)
{
    (void)state; // Unused

    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();
    settings_t *settings = settings_get();
    event_data_t rx_event_data = {0};

    // Sentinel value, unreachable by any brightness percentage used below
    // (50/70/60% convert to 127/178/153), so we can tell "untouched" apart
    // from "applied".
    settings->headlight_brightness = 255U;
    settings->status_brightness = 255U;

    // Packet 1: baseline poll response (headlight=50%, status=30%). This is
    // also the first valid packet, so it raises EVENT_VESC_ALIVE. Since this
    // only establishes the remote baseline, no settings should be applied.
    {
        uint8_t payload[] = {0x24, 0x65, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 20, 30};

        expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
        expect_any(event_queue_push, data);

        ring_buffer_push(rx_buffer, 0x02);
        ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
        for (size_t i = 0; i < sizeof(payload); i++)
        {
            ring_buffer_push(rx_buffer, payload[i]);
        }
        ring_buffer_push(rx_buffer, 0xbc);
        ring_buffer_push(rx_buffer, 0x71);
        ring_buffer_push(rx_buffer, 0x03);

        event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);
    }

    assert_int_equal(255U, settings->headlight_brightness);
    assert_int_equal(255U, settings->status_brightness);

    // Packet 2: identical values repeated - matches the baseline, so nothing
    // should be applied and no events should fire.
    {
        uint8_t payload[] = {0x24, 0x65, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 20, 30};

        ring_buffer_push(rx_buffer, 0x02);
        ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
        for (size_t i = 0; i < sizeof(payload); i++)
        {
            ring_buffer_push(rx_buffer, payload[i]);
        }
        ring_buffer_push(rx_buffer, 0xbc);
        ring_buffer_push(rx_buffer, 0x71);
        ring_buffer_push(rx_buffer, 0x03);

        event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);
    }

    assert_int_equal(255U, settings->headlight_brightness);
    assert_int_equal(255U, settings->status_brightness);

    // Packet 3: headlight brightness changes to 70% (status unchanged) -
    // should apply and fire EVENT_COMMAND_SETTINGS_CHANGED for the
    // headlight brightness context only.
    {
        uint8_t payload[] = {0x24, 0x65, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70, 20, 30};
        command_processor_context_t expected_context = COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS;

        expect_value(event_queue_push, event, EVENT_COMMAND_SETTINGS_CHANGED);
        expect_check(event_queue_push, data, validate_context_event_data,
                     (uintmax_t)&expected_context);

        ring_buffer_push(rx_buffer, 0x02);
        ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
        for (size_t i = 0; i < sizeof(payload); i++)
        {
            ring_buffer_push(rx_buffer, payload[i]);
        }
        ring_buffer_push(rx_buffer, 0xb8);
        ring_buffer_push(rx_buffer, 0xb9);
        ring_buffer_push(rx_buffer, 0x03);

        event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);
    }

    assert_int_equal(178, settings->headlight_brightness); // 70% -> (70*255)/100
    assert_int_equal(255U, settings->status_brightness);

    // Packet 4: status bar brightness changes to 60% (headlight unchanged
    // from its new 70% baseline) - should apply and fire
    // EVENT_COMMAND_SETTINGS_CHANGED for the status bar context only.
    {
        uint8_t payload[] = {0x24, 0x65, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70, 20, 60};
        command_processor_context_t expected_context = COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS;

        expect_value(event_queue_push, event, EVENT_COMMAND_SETTINGS_CHANGED);
        expect_check(event_queue_push, data, validate_context_event_data,
                     (uintmax_t)&expected_context);

        ring_buffer_push(rx_buffer, 0x02);
        ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
        for (size_t i = 0; i < sizeof(payload); i++)
        {
            ring_buffer_push(rx_buffer, payload[i]);
        }
        ring_buffer_push(rx_buffer, 0xbc);
        ring_buffer_push(rx_buffer, 0x99);
        ring_buffer_push(rx_buffer, 0x03);

        event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);
    }

    assert_int_equal(178, settings->headlight_brightness); // 70% -> (70*255)/100
    assert_int_equal(153, settings->status_brightness);    // 60% -> (60*255)/100

    // Packet 5: too short to contain the status brightness byte - should be
    // ignored entirely (no crash, no events, no change to either setting).
    {
        uint8_t payload[] = {0x24, 0x65, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70, 20};

        ring_buffer_push(rx_buffer, 0x02);
        ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
        for (size_t i = 0; i < sizeof(payload); i++)
        {
            ring_buffer_push(rx_buffer, payload[i]);
        }
        ring_buffer_push(rx_buffer, 0x8e);
        ring_buffer_push(rx_buffer, 0x3b);
        ring_buffer_push(rx_buffer, 0x03);

        event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);
    }

    assert_int_equal(178, settings->headlight_brightness); // 70% -> (70*255)/100
    assert_int_equal(153, settings->status_brightness);    // 60% -> (60*255)/100

    // Packet 6: wrong refloat package id (not 101) - should be ignored
    // entirely (no crash, no events, no change to either setting).
    {
        uint8_t payload[] = {0x24, 0x63, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 70, 20, 60};

        ring_buffer_push(rx_buffer, 0x02);
        ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
        for (size_t i = 0; i < sizeof(payload); i++)
        {
            ring_buffer_push(rx_buffer, payload[i]);
        }
        ring_buffer_push(rx_buffer, 0xb7);
        ring_buffer_push(rx_buffer, 0xfe);
        ring_buffer_push(rx_buffer, 0x03);

        event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);
    }

    assert_int_equal(178, settings->headlight_brightness); // 70% -> (70*255)/100
    assert_int_equal(153, settings->status_brightness);    // 60% -> (60*255)/100

    assert_true(ring_buffer_is_empty(rx_buffer));
}

// Not declared in vesc_serial.h - non-static so tests can call it
// directly, matching this codebase's convention.
void clear_outstanding_packets(void);

/**
 * @brief board_mode_change() dispatches every polling-mode value the same
 * way as BOOTING/IDLE (already covered above) - a couple more for branch
 * coverage of the switch itself, plus the non-polling default branch's
 * "already inactive" sub-case.
 */
/**
 * @brief vesc_serial_tx_timerid is a module static that vesc_serial_init()
 * never resets, so it carries over from whatever earlier tests in this
 * group left it at (non-invalid, since test_vesc_serial_timer_callback
 * started one and nothing since has torn it down) - is_timer_active() is
 * therefore always consulted here, not short-circuited.
 */
static void test_board_mode_change_riding_and_disabled_poll(void **state)
{
    (void)state;

    // Not active - starts a fresh timer.
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_RIDING;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);

    // Already active - no new timer.
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    data.board_mode.mode = BOARD_MODE_DISABLED;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief The non-polling ("default") branch, with the timer non-invalid
 * (see the comment above) and reporting inactive - is_timer_active() is
 * consulted but cancel_timer() must NOT be called.
 */
static void test_board_mode_change_off_with_inactive_timer(void **state)
{
    (void)state;

    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_OFF;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief The tx timer callback's outstanding-packet accounting: it only
 * increments while vesc_alive is true, and faults once the count reaches
 * MAX_OUTSTANDING_PACKETS (5), which also resets vesc_alive and clears the
 * count back to 0.
 */
static void test_tx_timer_faults_after_max_outstanding_packets(void **state)
{
    (void)state;

    // Arm the tx timer via a board-mode dispatch first, so
    // call_timer_callback() below has a real, known timer id to invoke -
    // timer_init() (in vesc_serial_setup()) resets the mock's id counter
    // fresh each test, so this is the first set_timer() call here and is
    // assigned id 1, regardless of vesc_serial_tx_timerid's carried-over
    // value from earlier tests (see test_board_mode_change_riding_and_
    // disabled_poll's comment).
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    event_data_t boot_data = {0};
    boot_data.board_mode.mode = BOARD_MODE_BOOTING;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &boot_data);

    // Establish vesc_alive = true via one valid (if unknown-command)
    // packet first - this also calls clear_outstanding_packets() at the
    // top of the rx handler, so outstanding starts at 0 here.
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();
    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x03);
    expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
    expect_any(event_queue_push, data);
    event_data_t rx_data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_data);

    // 5 ticks (0..4) increment outstanding to 5, each just sending; the
    // 6th tick sees outstanding (post-increment check is `>= 5`, checked
    // against the PRE-increment value 5 on the 6th call) trips the fault.
    for (int i = 0; i < 5; i++)
    {
        expect_any(vesc_serial_hw_send, data);
        expect_any(vesc_serial_hw_send, len);
        call_timer_callback(1, 0);
    }

    expect_value(fault, fault, EMERGENCY_FAULT_VESC_COMM_TIMEOUT);
    expect_any(vesc_serial_hw_send, data);
    expect_any(vesc_serial_hw_send, len);
    call_timer_callback(1, 0);

    // vesc_alive is now false again - the NEXT tick must not increment
    // (and therefore must not fault again either).
    expect_any(vesc_serial_hw_send, data);
    expect_any(vesc_serial_hw_send, len);
    call_timer_callback(1, 0);
}

static bool_t busy_callback_invoked = false;
static void test_busy_callback(void)
{
    busy_callback_invoked = true;
}

/**
 * @brief vesc_serial_check_busy_and_set_callback()/clear_outstanding_packets():
 * not busy while idle (never alive, or no outstanding packets), busy once
 * alive with an outstanding packet, and the queued callback fires exactly
 * once the next time anything clears the outstanding count.
 */
static void test_check_busy_and_set_callback(void **state)
{
    (void)state;

    busy_callback_invoked = false;

    // Never alive yet - not busy regardless of outstanding count.
    assert_int_equal(LCM_SUCCESS, vesc_serial_check_busy_and_set_callback(test_busy_callback));

    // Arm the tx timer with a real, known id (see the comment in
    // test_tx_timer_faults_after_max_outstanding_packets for why this is
    // needed before call_timer_callback() below can do anything).
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_any(set_timer, timeout);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
    event_data_t boot_data = {0};
    boot_data.board_mode.mode = BOARD_MODE_BOOTING;
    event_queue_call_mocked_callback(EVENT_BOARD_MODE_CHANGED, &boot_data);

    // Become alive via one valid packet (also clears outstanding to 0).
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();
    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x03);
    expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
    expect_any(event_queue_push, data);
    event_data_t rx_data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_data);

    // Alive but outstanding is still 0 - not busy.
    assert_int_equal(LCM_SUCCESS, vesc_serial_check_busy_and_set_callback(test_busy_callback));

    // One tx tick makes outstanding 1 - now busy, callback stored.
    expect_any(vesc_serial_hw_send, data);
    expect_any(vesc_serial_hw_send, len);
    call_timer_callback(1, 0);
    assert_int_equal(LCM_BUSY, vesc_serial_check_busy_and_set_callback(test_busy_callback));

    // Anything that clears outstanding (another valid rx) must invoke the
    // stored callback exactly once.
    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, 0x01);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x00);
    ring_buffer_push(rx_buffer, 0x03);
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_data);
    assert_true(busy_callback_invoked);
}

/**
 * @brief The EVENT_VESC_FAULT_CHANGED path is never exercised by any
 * existing test (they all use fault=0, matching the cached initial 0) -
 * a nonzero fault byte must fire it, and the accessor must reflect it.
 */
static void test_comm_setup_fault_changed(void **state)
{
    (void)state;

    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();

    // Same payload as test_vesc_serial_comm_setup_decodes_values but with
    // fault=5 instead of 0 - only the fault byte and its CRC differ.
    uint8_t payload[] = {0x33, 0x00, 0x01, 0x01, 0xb8,
                          0x00, 0x00, 0x01, 0xf4,
                          0x00, 0xfa,
                          0x00, 0x00, 0x03, 0xe8,
                          0x01, 0x86,
                          0x03, 0xe7,
                          0x05};

    // Compute the CRC dynamically rather than hand-deriving it - avoids
    // an error-prone manual recompute for a payload that only exists to
    // change one byte from an already-verified test.
    uint16_t crc = crc16_ccitt(payload, sizeof(payload));

    expect_value(event_queue_push, event, EVENT_VESC_ALIVE);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_DUTY_CYCLE_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_RPM_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_VOLTAGE_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_BATTERY_LEVEL_CHANGED);
    expect_any(event_queue_push, data);
    expect_value(event_queue_push, event, EVENT_VESC_FAULT_CHANGED);
    expect_any(event_queue_push, data);

    ring_buffer_push(rx_buffer, 0x02);
    ring_buffer_push(rx_buffer, (uint8_t)sizeof(payload));
    for (size_t i = 0; i < sizeof(payload); i++)
    {
        ring_buffer_push(rx_buffer, payload[i]);
    }
    ring_buffer_push(rx_buffer, (uint8_t)(crc >> 8));
    ring_buffer_push(rx_buffer, (uint8_t)(crc & 0xFF));
    ring_buffer_push(rx_buffer, 0x03);

    event_data_t rx_event_data = {0};
    event_queue_call_mocked_callback(EVENT_SERIAL_DATA_RX, &rx_event_data);

    assert_int_equal(5, vesc_serial_get_fault());
}

/**
 * @brief Cheap accessor coverage - each getter just needs to reflect
 * whatever was last decoded, asserted alongside the decode test's own
 * battery_level assertion elsewhere; this covers the remaining getters
 * directly against the module's true initial (zeroed) state.
 */
static void test_getters_reflect_initial_state(void **state)
{
    (void)state;

    assert_int_equal(0, vesc_serial_get_duty_cycle());
    assert_int_equal(0, vesc_serial_get_rpm());
    assert_int_equal(0, vesc_serial_get_input_voltage());
    assert_int_equal(0, vesc_serial_get_battery_level());
    assert_int_equal(0, vesc_serial_get_fault());
#ifdef ENABLE_IMU_EVENTS
    assert_int_equal(0, vesc_serial_get_imu_pitch());
    assert_int_equal(0, vesc_serial_get_imu_roll());
#endif
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
    cmocka_unit_test_setup(test_vesc_serial_comm_setup_decodes_values, vesc_serial_setup),
    cmocka_unit_test(test_vesc_serial_battery_lut_overrides_battery_level),
    cmocka_unit_test_setup(test_vesc_serial_app_integration, vesc_serial_setup),
    cmocka_unit_test_setup(test_board_mode_change_riding_and_disabled_poll, vesc_serial_setup),
    cmocka_unit_test_setup(test_board_mode_change_off_with_inactive_timer, vesc_serial_setup),
    cmocka_unit_test_setup(test_tx_timer_faults_after_max_outstanding_packets, vesc_serial_setup),
    cmocka_unit_test_setup(test_check_busy_and_set_callback, vesc_serial_setup),
    cmocka_unit_test_setup(test_comm_setup_fault_changed, vesc_serial_setup),
    cmocka_unit_test_setup(test_getters_reflect_initial_state, vesc_serial_setup),
};

#endif