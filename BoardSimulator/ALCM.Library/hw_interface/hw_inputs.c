#include "hw_inputs.h"
#include "ring_buffer.h"
#include "vesc_serial.h"
#include "event_queue.h"
#include "systick_sim.h"
#include "hw_callbacks.h"
#include <string.h>
#include <stdio.h>

// Global hardware input state
float g_footpad_left_voltage = 0.0f;
float g_footpad_right_voltage = 0.0f;
bool g_button_pressed = false;

// Track button state to detect changes
static bool g_button_pressed_last = false;

// VESC RX buffer - we don't maintain our own, we use vesc_serial's buffer
// External interrupt handler
extern void USART1_IRQHandler(void);

void hw_set_footpad_voltages(float left, float right) {
    // Clamp to valid range
    if (left < 0.0f) left = 0.0f;
    if (left > 3.3f) left = 3.3f;
    if (right < 0.0f) right = 0.0f;
    if (right > 3.3f) right = 3.3f;
    
    g_footpad_left_voltage = left;
    g_footpad_right_voltage = right;
}

void hw_set_button_state(bool pressed) {
    g_button_pressed = pressed;
    
    // Simulate GPIO interrupt - when button state changes, trigger wakeup event
    // This starts the button debounce timer in button_driver.c
    if (pressed != g_button_pressed_last) {
        event_data_t event_data = {0};
        event_data.button_data.time = systick_get_tick_count();
        
        // Debug log
        if (g_debug_callback) {
            char buf[120];
            sprintf(buf, "[hw_inputs] EVENT_BUTTON_WAKEUP pushed (button=%s, systick=%u)",
                    pressed ? "PRESSED" : "RELEASED", event_data.button_data.time);
            g_debug_callback(buf);
        }
        
        event_queue_push(EVENT_BUTTON_WAKEUP, &event_data);
        g_button_pressed_last = pressed;
    }
}

void hw_inject_vesc_data(const uint8_t *data, uint16_t length) {
    // Get the VESC RX ring buffer (same one that USART handler uses)
    ring_buffer_t *rx_buffer = vesc_serial_get_rx_buffer();
    
    if (rx_buffer == NULL || data == NULL) {
        return;
    }
    
    // Push data into the ring buffer
    for (uint16_t i = 0; i < length; i++) {
        ring_buffer_push(rx_buffer, data[i]);
    }
    
    // Simulate USART IDLE interrupt (notifies vesc_serial that data is ready)
    USART1_IRQHandler();
}
