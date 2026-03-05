#include "system_hk32f030m.h"

// System clock variable (simulated 32MHz like real hardware)
uint32_t SystemCoreClock = 32000000;

void SystemInit(void) {
    // No-op in simulation
}

void SystemCoreClockUpdate(void) {
    // No-op in simulation
}
