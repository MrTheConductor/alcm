/*
 * BoardSimulator - C API for ALCM Library
 * Provides P/Invoke interface for C# applications
 */

#define BUILDING_ALCM_LIBRARY
#include "alcm_api.h"
#include <stdio.h>

// ALCM headers
#include "lcm_types.h"
#include "event_queue.h"
#include "tim1.h"
#include "command_processor.h"
#include "timer.h"
#include "button_driver.h"
#include "button_events.h"
#include "board_mode.h"
#include "power.h"
#include "buzzer.h"
#include "headlights.h"
#include "footpads.h"
#include "status_leds.h"
#include "vesc_serial.h"
#include "systick_sim.h"
#include "hw_callbacks.h"
#include "hw_inputs.h"
#include "core_cm0.h"

// Macro to check initialization status
#define INIT(x) \
    do { \
        if (g_debug_callback) { \
            char buf[64]; \
            sprintf(buf, "Initializing " #x "..."); \
            g_debug_callback(buf); \
        } \
        if (LCM_SUCCESS != x##_init()) { \
            if (g_debug_callback) { \
                char buf[64]; \
                sprintf(buf, "FAILED: " #x "_init()"); \
                g_debug_callback(buf); \
            } \
            return -1; \
        } \
    } while(0)

int alcm_init(void) {
    lcm_status_t status = LCM_SUCCESS;
    
    if (g_debug_callback) {
        g_debug_callback("alcm_init() starting...");
    }
    
    // Configure systick for 1ms (simulated)
    extern uint32_t SystemCoreClock;
    if (0 != SysTick_Config(SystemCoreClock / 1000)) {
        if (g_debug_callback) {
            g_debug_callback("FAILED: SysTick_Config");
        }
        return -1;
    }
    
    // Initialize ALCM modules (same sequence as main.c)
    INIT(TIM1);
    INIT(command_processor);
    INIT(timer);
    INIT(button_driver);
    INIT(button_events);
    INIT(board_mode);
    INIT(power);
    
#ifdef ENABLE_BUZZER
    INIT(buzzer);
#endif
    
    INIT(headlights);
    INIT(footpads);
    
#ifdef ENABLE_STATUS_LEDS
    INIT(status_leds);
#endif
    
    INIT(vesc_serial);

    // Push boot event (same as main.c)
    event_queue_push(EVENT_COMMAND_BOOT, NULL);
    
    if (g_debug_callback) {
        g_debug_callback("Processing boot event...");
    }
    
    // Process the boot event immediately (simulates first iteration of main loop)
    event_queue_pop_and_notify();
    
    if (g_debug_callback) {
        g_debug_callback("alcm_init() completed successfully");
    }
    
    return 0; // Success
}

void alcm_shutdown(void) {
    // Cleanup if needed
    alcm_reset_tick_count();
}

void alcm_register_callbacks(const alcm_callbacks_t *callbacks) {
    if (callbacks == NULL) {
        return;
    }
    
    // Register debug first so we can use it for logging
    hw_register_debug_callback(callbacks->debug_cb);
    
    // TEST: Send a message immediately to verify callback works
    if (g_debug_callback) {
        g_debug_callback("TEST: Debug callback is working!");
        g_debug_callback("Registering callbacks...");
    }
    
    hw_register_status_led_callback(callbacks->status_led_cb);
    hw_register_headlight_callback(callbacks->headlight_cb);
    hw_register_buzzer_callback(callbacks->buzzer_cb);
    hw_register_power_callback(callbacks->power_cb);
    hw_register_vesc_request_callback(callbacks->vesc_request_cb);
    
    if (g_debug_callback) {
        g_debug_callback("All callbacks registered");
    }
}

// Simulation control
void alcm_tick(float delta_ms) {
    systick_advance_time(delta_ms);
}

int alcm_process_events(void) {
    // Process one event from the queue (same as main loop)
    lcm_status_t result = event_queue_pop_and_notify();
    return (result == LCM_SUCCESS) ? 0 : -1;
}

uint32_t alcm_get_tick_count(void) {
    return systick_get_tick_count();
}

void alcm_reset_tick_count(void) {
    systick_reset_tick_count();
}

// Input control - forward to hw_inputs
void alcm_set_footpad_voltages(float left, float right) {
    hw_set_footpad_voltages(left, right);
}

void alcm_set_button_state(bool pressed) {
    hw_set_button_state(pressed);
}

void alcm_inject_vesc_data(const uint8_t *data, uint16_t length) {
    hw_inject_vesc_data(data, length);
}

// Testing function
void alcm_test_debug_callback(void) {
    if (g_debug_callback) {
        g_debug_callback("alcm_test_debug_callback() was called!");
    }
}

int alcm_are_callbacks_registered(void) {
    int count = 0;
    if (g_status_led_callback != NULL) count++;
    if (g_headlight_callback != NULL) count++;
    if (g_buzzer_callback != NULL) count++;
    if (g_power_callback != NULL) count++;
    if (g_debug_callback != NULL) count++;
    return count;
}

// State query - these would need to be implemented if we want to query state
// For now, the GUI uses callbacks to receive updates
void alcm_get_led_state(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b) {
    // Not implemented - GUI receives updates via callbacks
    if (r) *r = 0;
    if (g) *g = 0;
    if (b) *b = 0;
}

void alcm_get_headlight_state(uint8_t *direction, uint16_t *brightness) {
    // Not implemented - GUI receives updates via callbacks
    if (direction) *direction = 0;
    if (brightness) *brightness = 0;
}
