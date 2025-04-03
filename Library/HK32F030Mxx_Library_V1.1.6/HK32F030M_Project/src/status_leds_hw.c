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
#include "lcm_types.h"
#include "status_leds_hw.h"
#include "interrupts.h"
#include "hk32f030m.h"
#include "tiny_math.h"

// Implemented in assembly (see ws2812.s)
extern void ws2812_send_buffer(uint8_t *buffer, uint32_t length);

// Global brightness scaling
static uint16_t brightness_scale = 0U;
static bool_t status_leds_enabled = false;

/**
 * @brief Initializes the status LEDs hardware module.
 *
 * This function initializes the status LEDs hardware and prepares it for use.
 */
void status_leds_hw_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0U};
    GPIO_StructInit(&GPIO_InitStructure);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // Initialize the status LEDs
    brightness_scale = 0U;
    GPIOD->BSRR = GPIO_Pin_4 << 16U;
}

lcm_status_t status_leds_hw_refresh(const status_leds_color_t *buffer)
{
    lcm_status_t result = LCM_SUCCESS;

    if (buffer == NULL)
    {
        result = LCM_ERROR_NULL_POINTER;
    }
    else
    {
        if (status_leds_enabled)
        {
            status_leds_color_t scaled_buffer[STATUS_LEDS_COUNT];

            // Scale LEDs by global brightness
            for (uint8_t i = 0U; i < STATUS_LEDS_COUNT; i++)
            {
                scaled_buffer[i].r = (buffer[i].r * brightness_scale) >> 8U;
                scaled_buffer[i].g = (buffer[i].g * brightness_scale) >> 8U;
                scaled_buffer[i].b = (buffer[i].b * brightness_scale) >> 8U;
            }

            // Disable interrupts to prevent timing issues while bitbanging the
            // LEDs.
            interrupts_disable(INTERRUPT_YIELD_NORMAL);
            ws2812_send_buffer((uint8_t *)scaled_buffer,
                               STATUS_LEDS_COUNT * sizeof(status_leds_color_t));
            interrupts_enable();
        }
    }

    return result;
}

/**
 * @brief Sets the global brightness of the status LEDs.
 *
 * @param brightness Float value in the range [0.0f, 1.0f] to set the
 * global brightness to. The actual brightness of the LEDs will be
 * scaled by this value. The value is clamped to the range before
 * being applied.
 */
void status_leds_hw_set_brightness(float32_t brightness)
{
    float32_t new_brightness = CLAMP(brightness, 0.0f, 1.0f);
    float32_t temp_float_scale = new_brightness * 256.0f;
    brightness_scale = (uint16_t)(temp_float_scale);
}

void status_leds_hw_enable(bool_t enable)
{
    status_leds_enabled = enable;
}