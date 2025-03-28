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
#include <setjmp.h>
#include <cmocka.h>
#include "vesc_serial.h"

lcm_status_t vesc_serial_init(void) {
    function_called();
    return LCM_SUCCESS;
}

ring_buffer_t *vesc_serial_get_rx_buffer(void) {
    return (ring_buffer_t *)mock();
}

float vesc_serial_get_duty_cycle(void) {
    return (float)mock();
}

int32_t vesc_serial_get_rpm(void) {
    return (int32_t)mock();
}

float vesc_serial_get_battery_level(void) {
    return (float)mock();
}

uint8_t vesc_serial_get_fault(void) {
    return (uint8_t)mock();
}