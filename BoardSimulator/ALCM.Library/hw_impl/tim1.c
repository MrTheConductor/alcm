/*
 * BoardSimulator - TIM1 stub for simulation
 * Replaces tim1.c with no-op implementation
 */

#include "tim1.h"
#include "hk32f030m_tim.h"
#include "hk32f030m_gpio.h"
#include "hk32f030m_rcc.h"
#include "lcm_types.h"

// TIM1 is used for headlight PWM - we stub it out completely
lcm_status_t TIM1_init(void) {
    // No-op - headlights_hw.c handles the actual headlight control
    return LCM_SUCCESS;
}
