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

/**
 * @file mock_eeprom.h
 * @brief Test helpers for the stateful fake EEPROM in mock_eeprom.c.
 */
#ifndef MOCK_EEPROM_H
#define MOCK_EEPROM_H

#include <stdint.h>

/**
 * @brief Blanks the fake EEPROM (simulates a factory-fresh chip).
 */
void mock_eeprom_reset(void);

/**
 * @brief Stages arbitrary bytes at addr in the fake EEPROM.
 */
void mock_eeprom_set(uint16_t addr, const uint8_t *data, uint16_t len);

/**
 * @brief Number of eeprom_write() calls since the last mock_eeprom_reset().
 */
uint32_t mock_eeprom_get_write_count(void);

#endif // MOCK_EEPROM_H
