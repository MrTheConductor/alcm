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
#include "vesc_serial.h"

// Implemented in assembly (see ws2812.s)
extern void ws2812_send_buffer(uint8_t *buffer, uint32_t length);

// Global brightness scaling
static uint16_t brightness_scale = 0U;
static bool_t status_leds_enabled = false;
static const status_leds_color_t *status_leds_hw_buffer = NULL;

/**
 * @brief Initializes the status LEDs hardware module.
 *
 * This function initializes the status LEDs hardware and prepares it for use.
 */
void status_leds_hw_init(const status_leds_color_t *buffer)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // Initialize the status LEDs
    brightness_scale = 0U;
    GPIOD->BSRR = GPIO_Pin_4 << 16U;

    // Set the buffer to the provided buffer
    status_leds_hw_buffer = buffer;
}

void status_leds_hw_update(void)
{
    if (status_leds_hw_buffer != NULL)
    {
        if (status_leds_enabled)
        {
            status_leds_color_t scaled_buffer[STATUS_LEDS_COUNT];

            // Scale LEDs by global brightness
            for (uint8_t i = 0U; i < STATUS_LEDS_COUNT; i++)
            {
                scaled_buffer[i].r = (status_leds_hw_buffer[i].r * brightness_scale) >> 8U;
                scaled_buffer[i].g = (status_leds_hw_buffer[i].g * brightness_scale) >> 8U;
                scaled_buffer[i].b = (status_leds_hw_buffer[i].b * brightness_scale) >> 8U;
            }

            // Disable interrupts to prevent timing issues while bitbanging the
            // LEDs.
            interrupts_disable();
            ws2812_send_buffer((uint8_t *)scaled_buffer,
                               STATUS_LEDS_COUNT * sizeof(status_leds_color_t));
            interrupts_enable();
        }
    }
}

void status_leds_hw_refresh()
{
    if (LCM_SUCCESS == vesc_serial_check_busy_and_set_callback(status_leds_hw_update))
    {
        // All clear, update the LEDs
        status_leds_hw_update();
    }
    // Else, VESC serial is busy and we will update the LEDs when it is free
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