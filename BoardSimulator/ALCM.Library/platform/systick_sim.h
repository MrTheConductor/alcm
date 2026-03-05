#ifndef __SYSTICK_SIM_H__
#define __SYSTICK_SIM_H__

#ifdef __cplusplus
extern "C" {
#endif

// Advance simulation time (internal function)
// delta_ms: Time to advance in milliseconds (can be fractional for smooth real-time)
void systick_advance_time(float delta_ms);

// Get current tick count (milliseconds since reset)
unsigned int systick_get_tick_count(void);

// Reset tick count to zero
void systick_reset_tick_count(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTICK_SIM_H__ */
