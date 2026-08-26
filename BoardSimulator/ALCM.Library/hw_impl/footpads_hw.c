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

uint16_t footpads_hw_get_left(void) {
    // Convert the GUI-set voltage back into a raw ADC count, matching the
    // real firmware's footpads_hw_get_left() contract (uint16_t counts, not
    // volts) now that the volts->threshold scaling lives in footpads.c.
    return (uint16_t)(g_footpad_left_voltage / ADC_VOLTAGE_SCALE);
}

uint16_t footpads_hw_get_right(void) {
    return (uint16_t)(g_footpad_right_voltage / ADC_VOLTAGE_SCALE);
}

void footpads_hw_calibrate(void) {
    // No-op in simulation - calibration is handled by GUI
}
