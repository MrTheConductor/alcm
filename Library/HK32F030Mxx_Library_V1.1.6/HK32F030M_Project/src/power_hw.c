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
#include "power_hw.h"
#include "hk32f030m.h"

void power_hw_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    GPIO_StructInit(&GPIO_InitStructure);

    // PWR_EN (PA2)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinLockConfig(GPIOA, GPIO_Pin_2);

    // Charge (PC5)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinLockConfig(GPIOC, GPIO_Pin_5);
}

void power_hw_set_power(power_hw_t power_hw)
{
    switch (power_hw)
    {
    case POWER_HW_ON:
        GPIOA->BSRR = GPIO_Pin_2;
        break;

    case POWER_HW_OFF:
        GPIOA->BRR = GPIO_Pin_2;
        break;

    default:
        break;
    }
}

void power_hw_set_charge(power_hw_t power_hw)
{
    switch (power_hw)
    {
    case POWER_HW_ON:
        GPIOC->BSRR = GPIO_Pin_5;
        break;

    case POWER_HW_OFF:
        GPIOC->BRR = GPIO_Pin_5;
        break;

    default:
        break;
    }
}