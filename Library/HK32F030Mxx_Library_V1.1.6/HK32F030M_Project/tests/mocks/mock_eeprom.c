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
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

#include "eeprom.h"

/**
 * @brief Stateful fake EEPROM backing store.
 *
 * Large enough for any real settings_eeprom_t plus headroom; real
 * eeprom_read()/eeprom_write() just memcpy to/from this, so callers (e.g.
 * settings.c) see genuine read-your-writes persistence across calls within
 * a test, rather than needing a strict expect_value() per call.
 */
#define MOCK_EEPROM_SIZE 256U
static uint8_t mock_eeprom_data[MOCK_EEPROM_SIZE];

/**
 * @brief Plain counter (not a cmocka expectation) so tests that care about
 * whether a write actually happened - e.g. settings_save()'s dirty-vs-clean
 * skip-the-write optimization - can check it without every other test
 * needing a matching expect_function_call() for writes it doesn't care
 * about.
 */
static uint32_t mock_eeprom_write_count = 0U;

void eeprom_write(uint16_t addr, uint8_t *data, uint16_t len)
{
    mock_eeprom_write_count++;
    memcpy(&mock_eeprom_data[addr], data, len);
}

void eeprom_read(uint16_t addr, uint8_t *data, uint16_t len)
{
    memcpy(data, &mock_eeprom_data[addr], len);
}

/**
 * @brief Test helper: blanks the fake EEPROM (simulates a factory-fresh chip)
 * and clears the write counter.
 */
void mock_eeprom_reset(void)
{
    memset(mock_eeprom_data, 0xFF, sizeof(mock_eeprom_data));
    mock_eeprom_write_count = 0U;
}

/**
 * @brief Test helper: number of eeprom_write() calls since the last
 * mock_eeprom_reset().
 */
uint32_t mock_eeprom_get_write_count(void)
{
    return mock_eeprom_write_count;
}

/**
 * @brief Test helper: stages arbitrary bytes at addr, e.g. to pre-load a
 * previously-saved settings blob or a deliberately corrupted one.
 */
void mock_eeprom_set(uint16_t addr, const uint8_t *data, uint16_t len)
{
    memcpy(&mock_eeprom_data[addr], data, len);
}