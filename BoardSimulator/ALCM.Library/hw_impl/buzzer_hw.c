/*
 * BoardSimulator - Windows hardware layer for buzzer
 * Implements buzzer_hw.h interface using callbacks
 */

#include "buzzer_hw.h"
#include "hw_callbacks.h"
#include <stdbool.h>

static bool buzzer_enabled = false;

void buzzer_hw_init(void) {
    buzzer_enabled = false;
}

void buzzer_hw_enable(bool enable) {
    buzzer_enabled = enable;
    
    // If disabling, ensure buzzer is off
    if (!enable && g_buzzer_callback != NULL) {
        g_buzzer_callback(0, 0);
    }
}

void buzzer_off(void) {
    if (g_buzzer_callback != NULL) {
        g_buzzer_callback(0, 0); // Frequency 0 = off
    }
}

void buzzer_on(void) {
    if (buzzer_enabled && g_buzzer_callback != NULL) {
        // 880Hz (A5 note - one octave above A4)
        // Duration 0 = continuous (stop with buzzer_off)
        g_buzzer_callback(880, 0);
    }
}
