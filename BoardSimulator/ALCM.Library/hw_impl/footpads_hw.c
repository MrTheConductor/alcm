/*
 * BoardSimulator - Windows hardware layer for footpads
 * Implements footpads_hw.h interface using input state
 */

#include "footpads_hw.h"
#include "hw_inputs.h"

// ADC conversion factor (simulating 12-bit ADC with 3.3V reference)
#define ADC_VOLTAGE_SCALE (3.3f / 4095.0f)

void footpads_hw_init(void) {
    // Initialize default voltages to 0
    g_footpad_left_voltage = 0.0f;
    g_footpad_right_voltage = 0.0f;
}

float footpads_hw_get_left(void) {
    // Return voltage set by GUI
    return g_footpad_left_voltage;
}

float footpads_hw_get_right(void) {
    // Return voltage set by GUI
    return g_footpad_right_voltage;
}

void footpads_hw_calibrate(void) {
    // No-op in simulation - calibration is handled by GUI
}
