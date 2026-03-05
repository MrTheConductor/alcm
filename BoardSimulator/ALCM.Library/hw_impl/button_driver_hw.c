/*
 * BoardSimulator - Windows hardware layer for button driver
 * Implements button_driver_hw.h interface using input state
 */

#include "button_driver_hw.h"
#include "hw_inputs.h"
#include "hw_callbacks.h"
#include "systick_sim.h"
#include <stdio.h>

void button_driver_hw_init(void) {
    // Initialize button as not pressed
    g_button_pressed = false;
}

bool button_driver_hw_is_pressed(void) {
    // Debug log
    if (g_debug_callback) {
        char buf[120];
        sprintf(buf, "[button_driver_hw] button_driver_hw_is_pressed() called, returning %s (systick=%u)",
                g_button_pressed ? "TRUE" : "FALSE", systick_get_tick_count());
        g_debug_callback(buf);
    }
    
    // Return button state set by GUI
    return g_button_pressed;
}
