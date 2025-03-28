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
#include "ring_buffer.h"

/**
 * @brief Get the next index in the ring buffer.
 */
uint16_t ring_buffer_next(const ring_buffer_t *buf, uint16_t idx)
{
    return (idx + 1U) % buf->size;
}

/**
 * @brief Check if the ring buffer is empty.
 */
bool_t ring_buffer_is_empty(const ring_buffer_t *buf)
{
    return buf->read_idx == buf->write_idx;
}

/**
 * @brief Check if the ring buffer is full.
 */
bool_t ring_buffer_is_full(const ring_buffer_t *buf)
{
    return ring_buffer_next(buf, buf->write_idx) == buf->read_idx;
}

/**
 * @brief Push data into the ring buffer.
 */
bool_t ring_buffer_push(ring_buffer_t *buf, uint8_t data)
{
    bool_t result = true;
    if (!ring_buffer_is_full(buf))
    {
        buf->buffer[buf->write_idx] = data;
        buf->write_idx = ring_buffer_next(buf, buf->write_idx);
    }
    else
    {
        result = false; // Buffer full
    }
    return result;
}

/**
 * @brief Pop data from the ring buffer.
 */
bool_t ring_buffer_pop(ring_buffer_t *buf, uint8_t *data)
{
    bool_t result = true;

    if (!ring_buffer_is_empty(buf))
    {
        *data = buf->buffer[buf->read_idx];
        buf->read_idx = ring_buffer_next(buf, buf->read_idx);
    }
    else
    {
        result = false;
    }
    return result;
}

// Newline at end of file
