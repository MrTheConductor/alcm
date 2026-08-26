/*
 * BoardSimulator - Windows hardware layer for status LEDs
 * Implements status_leds_hw.h interface using callbacks
 */

#include "status_leds_hw.h"
#include "hw_callbacks.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// Hardware state
static const status_leds_color_t *status_leds_buffer = NULL;
static uint16_t brightness_scale = 256; // 0-256 (256 = 100%)
static bool status_leds_enabled = true;

void status_leds_hw_init(const status_leds_color_t *buffer) {
    status_leds_buffer = buffer;
    brightness_scale = 256; // Full brightness
    status_leds_enabled = true;
}

void status_leds_hw_refresh(void) {
    if (status_leds_buffer == NULL || !status_leds_enabled) {
        return;
    }
    
    // Debug: Notify that refresh was called
    if (g_debug_callback && g_status_led_callback == NULL) {
        g_debug_callback("status_leds_hw_refresh: callback is NULL!");
    }
    
    // Notify GUI of each LED's color (scaled by brightness)
    if (g_status_led_callback != NULL) {
        for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++) {
            uint8_t r = (status_leds_buffer[i].r * brightness_scale) >> 8;
            uint8_t g = (status_leds_buffer[i].g * brightness_scale) >> 8;
            uint8_t b = (status_leds_buffer[i].b * brightness_scale) >> 8;
            
            g_status_led_callback(i, r, g, b);
        }
    }
}

void status_leds_hw_set_brightness(uint8_t brightness) {
    // 0-255 = 0.0-1.0, converted to 0-256 scale (matches the real
    // firmware's scale8->scale9 trick: 255 maps to 256 = full brightness).
    brightness_scale = (uint16_t)brightness + ((uint16_t)brightness >> 7);
}

void status_leds_hw_enable(bool enable) {
    status_leds_enabled = enable;
    
    // If disabling, clear all LEDs
    if (!enable && g_status_led_callback != NULL) {
        for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++) {
            g_status_led_callback(i, 0, 0, 0);
        }
    }
}
