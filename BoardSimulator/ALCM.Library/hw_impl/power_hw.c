/*
 * BoardSimulator - Windows hardware layer for power management
 * Implements power_hw.h interface using callbacks
 */

#include "power_hw.h"
#include "hw_callbacks.h"
#include <stdbool.h>

void power_hw_init(void) {
    // Initialize with power on
    if (g_power_callback != NULL) {
        g_power_callback(true);
    }
}

void power_hw_set_power(power_hw_t power_hw) {
    if (g_power_callback != NULL) {
        g_power_callback(power_hw == POWER_HW_ON);
    }
}

void power_hw_set_charge(power_hw_t power_hw) {
    // Charge control not visualized in GUI currently
    // Could add later if needed
}
