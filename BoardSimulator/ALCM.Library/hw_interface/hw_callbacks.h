#ifndef __HW_CALLBACKS_H__
#define __HW_CALLBACKS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Callback function types for hardware events

// Status LED callback - called when LED colors are updated
// index: LED index (0-9 for 10 LEDs)
// r, g, b: Color components (0-255)
typedef void (*status_led_callback_t)(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

// Headlight callback - called when headlight state changes
// direction: 0=center, 1=left, 2=right
// brightness: PWM value (0-1024)
typedef void (*headlight_callback_t)(uint8_t direction, uint16_t brightness);

// Buzzer callback - called when buzzer should sound
// frequency: Frequency in Hz (0 = off)
// duration_ms: Duration in milliseconds
typedef void (*buzzer_callback_t)(uint16_t frequency, uint16_t duration_ms);

// Power callback - called when power latch state changes
// enabled: true to keep power on, false to power off
typedef void (*power_callback_t)(bool enabled);

// Debug callback - called for debug/log messages
// message: Null-terminated debug string
typedef void (*debug_callback_t)(const char *message);

// VESC request callback - called when ALCM sends request to VESC
// data: Request packet data
// len: Request packet length
typedef void (*vesc_request_callback_t)(const uint8_t* data, uint16_t len);

// Global callback pointers
extern status_led_callback_t g_status_led_callback;
extern headlight_callback_t g_headlight_callback;
extern buzzer_callback_t g_buzzer_callback;
extern power_callback_t g_power_callback;
extern debug_callback_t g_debug_callback;
extern vesc_request_callback_t g_vesc_request_callback;

// Callback registration functions
void hw_register_status_led_callback(status_led_callback_t callback);
void hw_register_headlight_callback(headlight_callback_t callback);
void hw_register_buzzer_callback(buzzer_callback_t callback);
void hw_register_power_callback(power_callback_t callback);
void hw_register_debug_callback(debug_callback_t callback);
void hw_register_vesc_request_callback(vesc_request_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __HW_CALLBACKS_H__ */
