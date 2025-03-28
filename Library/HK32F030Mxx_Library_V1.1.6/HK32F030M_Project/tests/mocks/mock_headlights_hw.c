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

#include "headlights_hw.h"

// Mock implementations

/**
 * @brief   Mock function to initialize the headlights hardware module
 */
void headlights_hw_init(void)
{
    function_called();
}

/**
 * @brief   Mock function to enable the headlights hardware module
 * @param   enable   Whether or not to enable the headlights
 */
void headlights_hw_enable(bool enable)
{
    check_expected(enable);
}

/**
 * @brief   Mock function to set the direction of the headlights
 * @param   direction   The direction to set the headlights to
 */
void headlights_hw_set_direction(headlights_direction_t direction)
{
    check_expected(direction);
}

headlights_direction_t headlights_hw_get_direction(void)
{
    mock();
}

/**
 * @brief   Mock function to set the brightness of the headlights
 * @param   brightness  The brightness to set the headlights to
 */
void headlights_hw_set_brightness(uint16_t brightness)
{
    check_expected(brightness);
}

uint16_t headlights_hw_get_brightness(void)
{
    mock();
}
