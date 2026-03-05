/*
 * BoardSimulator - Windows hardware layer for headlights
 * Implements headlights_hw.h interface using callbacks
 */

#include "headlights_hw.h"
#include "hw_callbacks.h"
#include "tim1.h"
#include <stdint.h>

// Hardware state
static headlights_direction_t current_direction = HEADLIGHTS_DIRECTION_NONE;
static uint16_t current_brightness = 0;

void headlights_hw_init(void) {
    current_direction = HEADLIGHTS_DIRECTION_NONE;
    current_brightness = 0;
    
    // Notify GUI of initial state
    if (g_headlight_callback != NULL) {
        g_headlight_callback((uint8_t)current_direction, current_brightness);
    }
}

void headlights_hw_set_direction(headlights_direction_t direction) {
    current_direction = direction;
    
    // Notify GUI
    if (g_headlight_callback != NULL) {
        g_headlight_callback((uint8_t)current_direction, current_brightness);
    }
}

headlights_direction_t headlights_hw_get_direction(void) {
    return current_direction;
}

void headlights_hw_set_brightness(uint16_t brightness) {
    // Clamp to valid range [0, TIM1_PERIOD]
    if (brightness > HEADLIGHTS_HW_MAX_BRIGHTNESS) {
        brightness = HEADLIGHTS_HW_MAX_BRIGHTNESS;
    }
    
    current_brightness = brightness;
    
    // Notify GUI
    if (g_headlight_callback != NULL) {
        g_headlight_callback((uint8_t)current_direction, current_brightness);
    }
}
