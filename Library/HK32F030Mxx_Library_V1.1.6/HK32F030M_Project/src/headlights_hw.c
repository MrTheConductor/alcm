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
#include "headlights_hw.h"
#include "hk32f030m.h"

/**
 * @brief Initializes the headlights hardware module.
 *
 * This function configures the necessary GPIO and TIM peripherals to control
 * the headlights. It sets up PWM on pin PB4 using TIM1 channel 2 to manage
 * brightness levels. Additionally, it initializes PC7 and PC6 as output pins
 * for controlling the direction of headlights based on the movement (forward
 * and backward). This setup enables control over the headlights' brightness
 * and direction.
 */

void headlights_hw_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

    // PWM (PB4) using TIM1 channel 2 controls brightness
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_3);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);

    // PC7 is used moving forward (white front, red rear)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // PC6 is used moving backward (red front, white rear)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
 * @brief Sets the direction of the headlights.
 *
 * This function configures the GPIO pins to set the direction of the
 * headlights. If the direction is set to HEADLIGHTS_DIRECTION_FORWARD, the
 * forward direction pin (GPIO_Pin_6) is activated and the reverse direction pin
 * (GPIO_Pin_7) is deactivated. For HEADLIGHTS_DIRECTION_REVERSE, the reverse
 * direction pin is activated and the forward direction pin is deactivated. If
 * the direction is HEADLIGHTS_DIRECTION_NONE, both pins are deactivated.
 *
 * @param direction The direction to set the headlights to, represented by
 *                  the headlights_direction_t enum.
 */

void headlights_hw_set_direction(headlights_direction_t direction)
{
    switch (direction)
    {
    case HEADLIGHTS_DIRECTION_REVERSE:
        GPIOC->BSRR = GPIO_Pin_6;
        GPIOC->BRR = GPIO_Pin_7;
        break;

    case HEADLIGHTS_DIRECTION_FORWARD:
        GPIOC->BSRR = GPIO_Pin_7;
        GPIOC->BRR = GPIO_Pin_6;
        break;

    case HEADLIGHTS_DIRECTION_NONE:
        GPIOC->BRR = GPIO_Pin_6;
        GPIOC->BRR = GPIO_Pin_7;
        break;

    default:
        // FIXME
        break;
    }
}

/**
 * @brief Retrieves the current direction of the headlights.
 *
 * This function reads the output data register of GPIO port C to determine
 * the current direction of the headlights. If the pin corresponding to
 * forward direction (GPIO_Pin_6) is set, it returns
 * HEADLIGHTS_DIRECTION_FORWARD. If the pin corresponding to reverse direction
 * (GPIO_Pin_7) is set, it returns HEADLIGHTS_DIRECTION_REVERSE. Otherwise, it
 * returns HEADLIGHTS_DIRECTION_NONE.
 *
 * @return The current direction of the headlights as a value of
 *         headlights_direction_t.
 */
headlights_direction_t headlights_hw_get_direction(void)
{
    if (GPIOC->ODR & GPIO_Pin_6)
    {
        return HEADLIGHTS_DIRECTION_REVERSE;
    }
    else if (GPIOC->ODR & GPIO_Pin_7)
    {
        return HEADLIGHTS_DIRECTION_FORWARD;
    }
    else
    {
        return HEADLIGHTS_DIRECTION_NONE;
    }
}

/**
 * @brief Sets the brightness of the headlights.
 *
 * This function adjusts the pulse width modulation (PWM) signal to set
 * the desired brightness level for the headlights. The brightness value
 * is clamped to the maximum allowable value defined by TIM1_PERIOD, and
 * the PWM compare value is updated only if the headlights are enabled.
 *
 * @param brightness The desired brightness level, ranging from 0 to
 * TIM1_PERIOD.
 */

void headlights_hw_set_brightness(uint16_t brightness)
{
    uint32_t compareValue = 0U;

    // Ensure brightness is within the expected range
    if (brightness > TIM1_PERIOD)
    {
        brightness = TIM1_PERIOD;
    }

    compareValue = (brightness * TIM1_PERIOD) / TIM1_PERIOD;
    TIM_SetCompare2(TIM1, compareValue);
}