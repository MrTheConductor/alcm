#include "hk32f030m_tim.h"

// TIM stub implementations - no-ops for simulation

void TIM_DeInit(TIM_TypeDef *TIMx) {
    // No-op
}

void TIM_TimeBaseInit(TIM_TypeDef *TIMx, TIM_TimeBaseInitTypeDef *TIM_TimeBaseInitStruct) {
    // No-op
}

void TIM_TimeBaseStructInit(TIM_TimeBaseInitTypeDef *TIM_TimeBaseInitStruct) {
    TIM_TimeBaseInitStruct->TIM_Period = 0xFFFF;
    TIM_TimeBaseInitStruct->TIM_Prescaler = 0x0000;
    TIM_TimeBaseInitStruct->TIM_ClockDivision = 0x0000;
    TIM_TimeBaseInitStruct->TIM_CounterMode = 0x0000;
    TIM_TimeBaseInitStruct->TIM_RepetitionCounter = 0x0000;
}

void TIM_Cmd(TIM_TypeDef *TIMx, uint32_t NewState) {
    // No-op
}

void TIM_CtrlPWMOutputs(TIM_TypeDef *TIMx, uint32_t NewState) {
    // No-op
}

void TIM_OC1Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct) {
    // No-op
}

void TIM_OC2Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct) {
    // No-op
}

void TIM_OC3Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct) {
    // No-op
}

void TIM_OC4Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct) {
    // No-op
}

void TIM_OCStructInit(TIM_OCInitTypeDef *TIM_OCInitStruct) {
    TIM_OCInitStruct->TIM_OCMode = TIM_OCMode_Timing;
    TIM_OCInitStruct->TIM_OutputState = TIM_OutputState_Disable;
    TIM_OCInitStruct->TIM_OutputNState = TIM_OutputState_Disable;
    TIM_OCInitStruct->TIM_Pulse = 0x00000000;
    TIM_OCInitStruct->TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStruct->TIM_OCNPolarity = TIM_OCPolarity_High;
    TIM_OCInitStruct->TIM_OCIdleState = 0x0000;
    TIM_OCInitStruct->TIM_OCNIdleState = 0x0000;
}

void TIM_SetCompare1(TIM_TypeDef *TIMx, uint32_t Compare) {
    // No-op
}

void TIM_SetCompare2(TIM_TypeDef *TIMx, uint32_t Compare) {
    // No-op
}

void TIM_SetCompare3(TIM_TypeDef *TIMx, uint32_t Compare) {
    // No-op
}

void TIM_SetCompare4(TIM_TypeDef *TIMx, uint32_t Compare) {
    // No-op
}

void TIM_BDTRConfig(TIM_TypeDef *TIMx, TIM_BDTRInitTypeDef *TIM_BDTRInitStruct) {
    // No-op
}

void TIM_BDTRStructInit(TIM_BDTRInitTypeDef *TIM_BDTRInitStruct) {
    TIM_BDTRInitStruct->TIM_OSSRState = 0x0000;
    TIM_BDTRInitStruct->TIM_OSSIState = 0x0000;
    TIM_BDTRInitStruct->TIM_LOCKLevel = 0x0000;
    TIM_BDTRInitStruct->TIM_DeadTime = 0x00;
    TIM_BDTRInitStruct->TIM_Break = 0x0000;
    TIM_BDTRInitStruct->TIM_BreakPolarity = 0x0000;
    TIM_BDTRInitStruct->TIM_AutomaticOutput = 0x0000;
}
