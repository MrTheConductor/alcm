#include "hw_callbacks.h"
#include <stddef.h>

// Global callback pointers (initialized to NULL)
status_led_callback_t g_status_led_callback = NULL;
headlight_callback_t g_headlight_callback = NULL;
buzzer_callback_t g_buzzer_callback = NULL;
power_callback_t g_power_callback = NULL;
debug_callback_t g_debug_callback = NULL;
vesc_request_callback_t g_vesc_request_callback = NULL;

void hw_register_status_led_callback(status_led_callback_t callback) {
    g_status_led_callback = callback;
    if (g_debug_callback) {
        g_debug_callback(callback != NULL ? "Status LED callback registered" : "Status LED callback set to NULL");
    }
}

void hw_register_headlight_callback(headlight_callback_t callback) {
    g_headlight_callback = callback;
    if (g_debug_callback) {
        g_debug_callback(callback != NULL ? "Headlight callback registered" : "Headlight callback set to NULL");
    }
}

void hw_register_buzzer_callback(buzzer_callback_t callback) {
    g_buzzer_callback = callback;
}

void hw_register_power_callback(power_callback_t callback) {
    g_power_callback = callback;
}

void hw_register_debug_callback(debug_callback_t callback) {
    g_debug_callback = callback;
}

void hw_register_vesc_request_callback(vesc_request_callback_t callback) {
    g_vesc_request_callback = callback;
}
