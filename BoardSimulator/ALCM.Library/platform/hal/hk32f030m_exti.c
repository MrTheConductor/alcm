#include "hk32f030m_exti.h"

// EXTI stub implementations - no-ops for simulation

void EXTI_DeInit(void) {
    // No-op
}

void EXTI_Init(EXTI_InitTypeDef *EXTI_InitStruct) {
    // No-op
}

void EXTI_StructInit(EXTI_InitTypeDef *EXTI_InitStruct) {
    EXTI_InitStruct->EXTI_Line = 0;
    EXTI_InitStruct->EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct->EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStruct->EXTI_LineCmd = 0;
}

uint32_t EXTI_GetFlagStatus(uint32_t EXTI_Line) {
    return 0;
}

void EXTI_ClearFlag(uint32_t EXTI_Line) {
    // No-op
}

uint32_t EXTI_GetITStatus(uint32_t EXTI_Line) {
    return 0;
}

void EXTI_ClearITPendingBit(uint32_t EXTI_Line) {
    // No-op
}
