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
#ifndef _STATUS_LEDS_HW_H
#define _STATUS_LEDS_HW_H

#include <stdint.h>
#include "lcm_types.h"

#define STATUS_LEDS_COUNT 10U

/**
 * @brief Status LED color struct.
 *
 * This struct is used to represent the color of a single status LED.
 *
 * @struct status_leds_color_t
 * @var r     The red component of the color (0-255)
 * @var g     The green component of the color (0-255)
 * @var b     The blue component of the color (0-255)
 */
typedef struct
{
    uint8_t g;
    uint8_t r;
    uint8_t b;
} status_leds_color_t;

void status_leds_hw_init(const status_leds_color_t *buffer);
void status_leds_hw_refresh(void);
void status_leds_hw_set_brightness(float32_t brightness);
void status_leds_hw_enable(bool_t enable);

#endif