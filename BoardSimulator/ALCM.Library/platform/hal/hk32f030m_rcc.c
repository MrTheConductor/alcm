#include "hk32f030m_rcc.h"

// RCC stub implementations - no-ops for simulation

void RCC_AHBPeriphClockCmd(uint32_t RCC_AHBPeriph, uint32_t NewState) {
    // No-op
}

void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, uint32_t NewState) {
    // No-op
}

void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, uint32_t NewState) {
    // No-op
}

void RCC_APB2PeriphResetCmd(uint32_t RCC_APB2Periph, uint32_t NewState) {
    // No-op
}

void RCC_APB1PeriphResetCmd(uint32_t RCC_APB1Periph, uint32_t NewState) {
    // No-op
}
