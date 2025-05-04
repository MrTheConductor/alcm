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
#include "buzzer_hw.h"
#include "hk32f030m.h"

// Static variables
static bool buzzer_enabled = false; // Buzzer enabled flag

/**
 * @brief Initializes the hardware for the buzzer.
 */
void buzzer_hw_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief Turns off the buzzer.
 */
void buzzer_off(void)
{
    GPIOA->BRR = GPIO_Pin_3;
}

/**
 * @brief Turns on the buzzer.
 */
void buzzer_on(void)
{
    // Only turn on the buzzer if it is enabled
    if (buzzer_enabled)
    {
        GPIOA->BSRR = GPIO_Pin_3;
    }
}

/**
 * @brief Enables or disables the buzzer.
 */
void buzzer_hw_enable(bool enable)
{
    buzzer_enabled = enable;

    // Turn off the buzzer if it is disabled
    if (!enable)
    {
        buzzer_off();
    }
}