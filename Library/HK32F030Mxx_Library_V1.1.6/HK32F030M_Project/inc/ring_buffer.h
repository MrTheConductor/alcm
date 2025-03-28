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
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include "lcm_types.h"

/**
 *  @brief Ring buffer structure
 *
 *  Ring buffer structure to store data in a circular buffer.
 */
typedef struct
{
    volatile uint8_t *buffer; // Pointer to the buffer
    uint16_t read_idx;        // Read index
    uint16_t write_idx;       // Write index
    uint16_t size;            // Size of the buffer
} ring_buffer_t;

/**
 * @brief Pushes a byte of data into the ring buffer.
 *
 * @param buf Pointer to the ring buffer structure.
 * @param data The byte of data to push into the buffer.
 * @return true if the data was successfully pushed, false if the buffer is full.
 */
bool_t ring_buffer_push(ring_buffer_t *buf, uint8_t data);

/**
 * @brief Pops a byte of data from the ring buffer.
 *
 * @param buf Pointer to the ring buffer structure.
 * @param data Pointer to store the popped byte of data.
 * @return true if the data was successfully popped, false if the buffer is empty.
 */
bool_t ring_buffer_pop(ring_buffer_t *buf, uint8_t *data);

/**
 * @brief Checks if the ring buffer is full.
 *
 * @param buf Pointer to the ring buffer structure.
 * @return true if the buffer is full, false otherwise.
 */
bool_t ring_buffer_is_full(const ring_buffer_t *buf);

/**
 * @brief Checks if the ring buffer is empty.
 *
 * @param buf Pointer to the ring buffer structure.
 * @return true if the buffer is empty, false otherwise.
 */
bool_t ring_buffer_is_empty(const ring_buffer_t *buf);

#endif