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
 * @file crc16_ccitt.h
 * @brief CRC16-CCITT header file
 */
#ifndef CRC16_CCITT_H
#define CRC16_CCITT_H

#include <stdint.h>

/**
 * @brief Computes the CRC-16-CCITT checksum for the given data.
 *
 * This function calculates the CRC-16-CCITT checksum for a block of data.
 * The CRC-16-CCITT algorithm is commonly used in telecommunications and
 * data storage applications to detect errors in data transmission or storage.
 *
 * @param data Pointer to the data buffer for which the checksum is to be calculated.
 * @param len Length of the data buffer in bytes.
 * @return The computed CRC-16-CCITT checksum.
 */
uint16_t crc16_ccitt(const uint8_t *data, uint16_t length);

#endif