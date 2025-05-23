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

#include "buzzer_hw.h"

/**
 * @brief   Mock function to initialize the buzzer hardware module
 */
void buzzer_hw_init(void)
{
    function_called();
}

/**
 * @brief   Mock function to enable the buzzer hardware module
 * @param   enable Whether or not to enable the buzzer
 */
void buzzer_hw_enable(bool enable)
{
    check_expected(enable);
}

/**
 * @brief   Mock function to turn the buzzer off
 */
void buzzer_off(void)
{
    function_called();
}

/**
 * @brief   Mock function to turn the buzzer on with a specified duty cycle
 * @param   duty_cycle The PWM duty cycle to use
 */
void buzzer_on()
{
    function_called();
}
