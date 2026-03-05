/*
 * BoardSimulator - Windows hardware layer for VESC serial
 * Implements vesc_serial_hw.h interface using input buffer
 */

#include "vesc_serial_hw.h"
#include "hw_inputs.h"
#include "hw_callbacks.h"

extern vesc_request_callback_t g_vesc_request_callback;

void vesc_serial_hw_init(uint32_t baud) {
    // No actual UART initialization needed in simulation
    // The vesc_serial.c will initialize the ring buffer
}

void vesc_serial_hw_send(uint8_t *data, uint16_t len) {
    // Capture outgoing VESC request and send to simulator for processing
    if (g_vesc_request_callback != NULL) {
        g_vesc_request_callback(data, len);
    }
}
