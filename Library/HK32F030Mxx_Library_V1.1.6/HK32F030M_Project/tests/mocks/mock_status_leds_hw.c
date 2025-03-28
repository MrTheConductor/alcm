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

#include "status_leds_hw.h"

/**
 * @brief   Mock function to initialize the status LEDs hardware module
 */
void status_leds_hw_init(void)
{
    function_called();
}

lcm_status_t status_leds_hw_refresh(const status_leds_color_t* buffer)
{
    check_expected_ptr(buffer);
    return LCM_SUCCESS;
}

void status_leds_hw_set_brightness(float brightness)
{
    check_expected(brightness);
}

void status_leds_hw_enable(bool enable)
{
    check_expected(enable);
}