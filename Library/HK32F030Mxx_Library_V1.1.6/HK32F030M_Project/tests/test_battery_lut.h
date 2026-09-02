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
#ifndef TEST_BATTERY_LUT_H
#define TEST_BATTERY_LUT_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "battery_lut.h"
#include "battery_lut_hw.h"
#include "crc16_ccitt.h"

// A representative 10S pack: 42.0V/100% -> 37.0V/50% -> 30.0V/0%, tenths of
// a volt/percent, with a correctly-computed CRC. r_int_milliohms=100
// (0.1 ohm) is a plausible whole-pack value - harmless for tests that
// only exercise interpolation/validation, and gives the IR-compensation
// tests (and vesc_serial.c's integration test) a nonzero value for free.
static battery_lut_block_t test_battery_lut_make_valid_block(void)
{
    battery_lut_block_t block = {0};

    block.payload.magic = BATTERY_LUT_MAGIC;
    block.payload.schema_version = BATTERY_LUT_SCHEMA_VERSION;
    block.payload.cell_count = 10U;
    block.payload.breakpoint_count = 3U;
    block.payload.breakpoints[0].voltage_tenths = 420U;
    block.payload.breakpoints[0].percent_tenths = 1000U;
    block.payload.breakpoints[1].voltage_tenths = 370U;
    block.payload.breakpoints[1].percent_tenths = 500U;
    block.payload.breakpoints[2].voltage_tenths = 300U;
    block.payload.breakpoints[2].percent_tenths = 0U;
    block.payload.r_int_milliohms = 100U;

    block.crc16 = crc16_ccitt((const uint8_t *)&block.payload, sizeof(block.payload));

    return block;
}

static void test_battery_lut_validate_valid_block(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    assert_true(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_null(void **state)
{
    (void)state; // Unused
    assert_false(battery_lut_validate_block(NULL));
}

static void test_battery_lut_validate_bad_magic(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.payload.magic = 0xdeadbeefU;
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_bad_schema(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.payload.schema_version = (uint8_t)(BATTERY_LUT_SCHEMA_VERSION + 1U);
    // Recompute the CRC so this failure is isolated to the schema check,
    // not a side effect of the CRC no longer matching.
    block.crc16 = crc16_ccitt((const uint8_t *)&block.payload, sizeof(block.payload));
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_zero_cell_count(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.payload.cell_count = 0U;
    block.crc16 = crc16_ccitt((const uint8_t *)&block.payload, sizeof(block.payload));
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_breakpoint_count_too_low(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.payload.breakpoint_count = 1U;
    block.crc16 = crc16_ccitt((const uint8_t *)&block.payload, sizeof(block.payload));
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_breakpoint_count_too_high(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.payload.breakpoint_count = (uint8_t)(BATTERY_LUT_MAX_BREAKPOINTS + 1U);
    block.crc16 = crc16_ccitt((const uint8_t *)&block.payload, sizeof(block.payload));
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_bad_crc(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.crc16 ^= 0xffffU;
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_voltage_not_descending(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    // Breakpoint 1's voltage should be strictly less than breakpoint 0's -
    // make it equal instead.
    block.payload.breakpoints[1].voltage_tenths = block.payload.breakpoints[0].voltage_tenths;
    block.crc16 = crc16_ccitt((const uint8_t *)&block.payload, sizeof(block.payload));
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_validate_percent_increasing(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    // Breakpoint 1's percent should be <= breakpoint 0's - make it greater
    // instead. This is the case that would otherwise underflow-wrap
    // battery_lut_interpolate()'s unsigned math.
    block.payload.breakpoints[1].percent_tenths =
        (uint16_t)(block.payload.breakpoints[0].percent_tenths + 1U);
    block.crc16 = crc16_ccitt((const uint8_t *)&block.payload, sizeof(block.payload));
    assert_false(battery_lut_validate_block(&block));
}

static void test_battery_lut_interpolate_at_top_breakpoint(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    assert_int_equal(1000, battery_lut_interpolate(&block, 420U));
}

static void test_battery_lut_interpolate_above_top_clamps(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    assert_int_equal(1000, battery_lut_interpolate(&block, 500U));
}

static void test_battery_lut_interpolate_at_bottom_breakpoint(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    assert_int_equal(0, battery_lut_interpolate(&block, 300U));
}

static void test_battery_lut_interpolate_below_bottom_clamps(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    assert_int_equal(0, battery_lut_interpolate(&block, 100U));
}

static void test_battery_lut_interpolate_midpoint(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    // 39.0V is 40% of the way from 37.0V (50%) up to 42.0V (100%):
    // 500 + (390-370)*(1000-500)/(420-370) = 500 + 20*500/50 = 500 + 200 = 700
    assert_int_equal(700, battery_lut_interpolate(&block, 390U));
}

static void test_battery_lut_init_invalid_falls_back(void **state)
{
    (void)state; // Unused
    will_return(battery_lut_hw_get_block, NULL);
    battery_lut_init();
    assert_false(battery_lut_is_valid());
}

static void test_battery_lut_init_valid_block(void **state)
{
    (void)state; // Unused
    static battery_lut_block_t block; // static: outlives this function, hw getter returns its address
    block = test_battery_lut_make_valid_block();

    will_return(battery_lut_hw_get_block, &block); // consumed by battery_lut_init()
    battery_lut_init();
    assert_true(battery_lut_is_valid());

    // battery_lut_get_percent() re-reads the block on every call (no RAM
    // caching), so this needs its own queued value too.
    will_return(battery_lut_hw_get_block, &block);
    assert_int_equal(700, battery_lut_get_percent(390U));

    // Same for battery_lut_get_voltage_compensation()'s own flash read.
    will_return(battery_lut_hw_get_block, &block);
    assert_int_equal(10, battery_lut_get_voltage_compensation(1000)); // 10A @ 0.1 ohm -> +1.0V
}

static void test_battery_lut_compensate_voltage_discharge(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block(); // r_int_milliohms = 100
    // 10A discharge, 0.1 ohm: I*R = 10 * 0.1 = 1.0V -> +10 tenths.
    assert_int_equal(10, battery_lut_compensate_voltage(&block, 1000));
}

static void test_battery_lut_compensate_voltage_regen(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    // 10A regen (negative current): terminal voltage is above OCV, so the
    // correction must subtract, not add.
    assert_int_equal(-10, battery_lut_compensate_voltage(&block, -1000));
}

static void test_battery_lut_compensate_voltage_zero_current(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    assert_int_equal(0, battery_lut_compensate_voltage(&block, 0));
}

static void test_battery_lut_compensate_voltage_zero_r_int_disables(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.payload.r_int_milliohms = 0U; // unpatched/unset - compensation must be a no-op
    assert_int_equal(0, battery_lut_compensate_voltage(&block, 1000));
}

static void test_battery_lut_compensate_voltage_truncates_like_firmware_integer_math(void **state)
{
    (void)state; // Unused
    battery_lut_block_t block = test_battery_lut_make_valid_block();
    block.payload.r_int_milliohms = 150U;
    // (333 * 150) / 10000 = 4.995 -> integer division truncates to 4.
    assert_int_equal(4, battery_lut_compensate_voltage(&block, 333));
}

const struct CMUnitTest battery_lut_tests[] = {
    cmocka_unit_test(test_battery_lut_validate_valid_block),
    cmocka_unit_test(test_battery_lut_validate_null),
    cmocka_unit_test(test_battery_lut_validate_bad_magic),
    cmocka_unit_test(test_battery_lut_validate_bad_schema),
    cmocka_unit_test(test_battery_lut_validate_zero_cell_count),
    cmocka_unit_test(test_battery_lut_validate_breakpoint_count_too_low),
    cmocka_unit_test(test_battery_lut_validate_breakpoint_count_too_high),
    cmocka_unit_test(test_battery_lut_validate_bad_crc),
    cmocka_unit_test(test_battery_lut_validate_voltage_not_descending),
    cmocka_unit_test(test_battery_lut_validate_percent_increasing),
    cmocka_unit_test(test_battery_lut_interpolate_at_top_breakpoint),
    cmocka_unit_test(test_battery_lut_interpolate_above_top_clamps),
    cmocka_unit_test(test_battery_lut_interpolate_at_bottom_breakpoint),
    cmocka_unit_test(test_battery_lut_interpolate_below_bottom_clamps),
    cmocka_unit_test(test_battery_lut_interpolate_midpoint),
    cmocka_unit_test(test_battery_lut_init_invalid_falls_back),
    cmocka_unit_test(test_battery_lut_init_valid_block),
    cmocka_unit_test(test_battery_lut_compensate_voltage_discharge),
    cmocka_unit_test(test_battery_lut_compensate_voltage_regen),
    cmocka_unit_test(test_battery_lut_compensate_voltage_zero_current),
    cmocka_unit_test(test_battery_lut_compensate_voltage_zero_r_int_disables),
    cmocka_unit_test(test_battery_lut_compensate_voltage_truncates_like_firmware_integer_math),
};

#endif
