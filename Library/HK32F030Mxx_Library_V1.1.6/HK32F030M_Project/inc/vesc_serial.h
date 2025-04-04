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
#ifndef VESC_SERIAL_H
#define VESC_SERIAL_H

#include "ring_buffer.h"
#include "lcm_types.h"

// There's a couple ways to get the battery level:
// 1. Read the input voltage and calculate a value based on
//    the number and type of cells installed.
// 2. Read the battery level directly from the VESC based
//    on their calculations.
//
// The first method is more accurate, but it requires
// programming the number of cells and battery curve into
// the firmware. The second method is less accurate, but
// requires no modification from board to board.
#undef ENABLE_INPUT_VOLTAGE

/**
 * @brief VESC serial callback function type
 */
typedef void (*vesc_serial_callback_t)(void);

lcm_status_t vesc_serial_init(void);
ring_buffer_t *vesc_serial_get_rx_buffer(void);

// Getters for the VESC serial data
float32_t vesc_serial_get_duty_cycle(void);
int32_t vesc_serial_get_rpm(void);
#if defined(ENABLE_INPUT_VOLTAGE)
float32_t vesc_serial_get_input_voltage(void);
#endif
float32_t vesc_serial_get_battery_level(void);
uint8_t vesc_serial_get_fault(void);
lcm_status_t vesc_serial_check_busy_and_set_callback(vesc_serial_callback_t callback);

#endif
