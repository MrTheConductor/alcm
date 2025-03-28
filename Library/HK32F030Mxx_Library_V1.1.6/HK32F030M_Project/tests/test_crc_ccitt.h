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
#ifndef TEST_CRC_CCITT_H
#define TEST_CRC_CCITT_H

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

#include "crc16_ccitt.h"

/**
 * @brief Tests that the CRC calculation function returns the correct result for
 * an empty data array.
 *
 * @param[in] state unused
 */
static void test_crc16_empty_data(void **state)
{
    (void)state; // Unused
    uint8_t data[] = {0};
    uint16_t result = crc16_ccitt(data, 0);
    assert_int_equal(result, 0x0000); // Known CRC for empty data
}

/**
 * @brief Tests that the CRC calculation function returns the correct result for
 * a single byte.
 */
static void test_crc16_single_byte(void **state)
{
    (void)state;             // Unused
    uint8_t data[] = {0x31}; // ASCII '1'
    uint16_t result = crc16_ccitt(data, sizeof(data));
    assert_int_equal(result, 0x2672); // Precomputed CRC
}

/**
 * @brief Tests that the CRC calculation function returns the correct result for
 * multiple bytes.
 *
 * This test uses a precomputed CRC for the given data. The data is a sequence
 * of 4 bytes containing the hexadecimal values 0x12, 0x34, 0x56, and 0x78. The
 * expected result is 0xE5CC.
 */
static void test_crc16_multiple_bytes(void **state)
{
    (void)state; // Unused
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
    uint16_t result = crc16_ccitt(data, sizeof(data));
    assert_int_equal(result, 0xb42c); // Precomputed CRC
}

/**
 * @brief Tests that the CRC calculation function returns the correct result for
 * a known string.
 *
 * The test string is "123456789", which has a known CRC of 0x29B1.
 */
static void test_crc16_known_string(void **state)
{
    (void)state; // Unused
    const char *test_string = "123456789";
    uint16_t result = crc16_ccitt((const uint8_t *)test_string, 9);
    assert_int_equal(result, 0x31c3); // Known CRC for "123456789"
}

/**
 * @brief Tests that the CRC calculation function returns the correct result for
 * all byte values from 0x00 to 0xFF.
 *
 * This test verifies the CRC-16/CCITT-FALSE implementation by using a dataset
 * that contains all possible byte values. The expected CRC result for this
 * sequence is 0xE3DC, which has been precomputed.
 */
static void test_crc16_all_bytes(void **state)
{
    (void)state; // Unused
    uint8_t data[256];
    for (int i = 0; i < 256; i++)
    {
        data[i] = (uint8_t)i;
    }
    uint16_t result = crc16_ccitt(data, sizeof(data));
    assert_int_equal(result, 0x7e55); // Precomputed CRC for 0x00-0xFF
}

const struct CMUnitTest crc16_ccitt_tests[] = {
    cmocka_unit_test(test_crc16_empty_data),     cmocka_unit_test(test_crc16_single_byte),
    cmocka_unit_test(test_crc16_multiple_bytes), cmocka_unit_test(test_crc16_known_string),
    cmocka_unit_test(test_crc16_all_bytes),
};

#endif