/*
 * BoardSimulator - Windows hardware layer for interrupt control
 * Implements interrupts.h interface (no-ops for simulation)
 */

#include "interrupts.h"
#include "core_cm0.h"

void interrupts_enable(void) {
    __enable_irq();
}

void interrupts_disable(void) {
    __disable_irq();
}

void wait_for_event(void) {
    __WFE();
}

void send_event(void) {
    __SEV();
}
