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
 * @file eeprom.h
 * @brief EEPROM interface for HK32F030Mxx microcontrollers.
 */
#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

/**
 * @brief Writes data to the EEPROM.
 *
 * This function writes a specified length of data to the EEPROM starting at the given address.
 *
 * @param addr The starting address in the EEPROM where the data will be written.
 * @param data A pointer to the data to be written to the EEPROM.
 * @param len The length of the data to be written, in bytes.
 */
void eeprom_write(uint16_t addr, uint8_t *data, uint16_t len);

/**
 * @brief Reads data from EEPROM.
 *
 * This function reads a specified number of bytes from the EEPROM starting
 * from the given address and stores them in the provided data buffer.
 *
 * @param addr The starting address in the EEPROM to read from.
 * @param data Pointer to the buffer where the read data will be stored.
 * @param len The number of bytes to read from the EEPROM.
 */
void eeprom_read(uint16_t addr, uint8_t *data, uint16_t len);

#endif // EEPROM_H