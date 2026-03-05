#ifndef __ALCM_API_H__
#define __ALCM_API_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Export symbols for DLL
#ifdef _WIN32
    #ifdef BUILDING_ALCM_LIBRARY
        #define ALCM_API __declspec(dllexport)
    #else
        #define ALCM_API __declspec(dllimport)
    #endif
#else
    #define ALCM_API
#endif

// Callback function types (must match P/Invoke delegates)
typedef void (*alcm_status_led_callback)(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
typedef void (*alcm_headlight_callback)(uint8_t direction, uint16_t brightness);
typedef void (*alcm_buzzer_callback)(uint16_t frequency, uint16_t duration_ms);
typedef void (*alcm_power_callback)(bool enabled);
typedef void (*alcm_debug_callback)(const char *message);
typedef void (*alcm_vesc_request_callback)(const uint8_t *data, uint16_t len);

// Callback registration structure
typedef struct {
    alcm_status_led_callback status_led_cb;
    alcm_headlight_callback headlight_cb;
    alcm_buzzer_callback buzzer_cb;
    alcm_power_callback power_cb;
    alcm_debug_callback debug_cb;
    alcm_vesc_request_callback vesc_request_cb;
} alcm_callbacks_t;

// Initialization and control
ALCM_API int alcm_init(void);
ALCM_API void alcm_shutdown(void);
ALCM_API void alcm_register_callbacks(const alcm_callbacks_t *callbacks);

// Simulation control
ALCM_API void alcm_tick(float delta_ms);
ALCM_API int alcm_process_events(void);
ALCM_API uint32_t alcm_get_tick_count(void);
ALCM_API void alcm_reset_tick_count(void);

// Input control
ALCM_API void alcm_set_footpad_voltages(float left, float right);
ALCM_API void alcm_set_button_state(bool pressed);
ALCM_API void alcm_inject_vesc_data(const uint8_t *data, uint16_t length);

// State query
ALCM_API void alcm_get_led_state(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b);
ALCM_API void alcm_get_headlight_state(uint8_t *direction, uint16_t *brightness);

// Testing
ALCM_API void alcm_test_debug_callback(void);
ALCM_API int alcm_are_callbacks_registered(void);

#ifdef __cplusplus
}
#endif

#endif /* __ALCM_API_H__ */
