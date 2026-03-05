#ifndef __HW_INPUTS_H__
#define __HW_INPUTS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hardware input state (set from GUI, read by hardware layer)

// Footpad voltages (0.0 - 3.3V)
extern float g_footpad_left_voltage;
extern float g_footpad_right_voltage;

// Button state
extern bool g_button_pressed;

// Input control functions (called from C# via API)
void hw_set_footpad_voltages(float left, float right);
void hw_set_button_state(bool pressed);
void hw_inject_vesc_data(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* __HW_INPUTS_H__ */
