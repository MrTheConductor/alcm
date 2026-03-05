#include "core_cm0.h"
#include "systick_sim.h"
#include <stdint.h>

// SysTick simulation for BoardSimulator
// Provides time control for debugging and testing

static volatile uint32_t systick_tick_count = 0;
static uint32_t systick_reload_value = 0;
static volatile float systick_accumulator = 0.0f;

// External SysTick interrupt handler (implemented in hk32f030m_it.c)
extern void SysTick_Handler(void);

uint32_t SysTick_Config(uint32_t ticks) {
    systick_reload_value = ticks;
    return 0; // Success
}

// Called from simulation engine to advance time
// delta_ms: Time to advance in milliseconds (can be fractional)
void systick_advance_time(float delta_ms) {
    systick_accumulator += delta_ms;
    
    // Process whole milliseconds
    while (systick_accumulator >= 1.0f) {
        systick_accumulator -= 1.0f;
        systick_tick_count++;
        
        // Call the SysTick interrupt handler
        SysTick_Handler();
    }
}

// Get current tick count (for diagnostics)
uint32_t systick_get_tick_count(void) {
    return systick_tick_count;
}

// Reset tick count (for testing)
void systick_reset_tick_count(void) {
    systick_tick_count = 0;
    systick_accumulator = 0.0f;
}
