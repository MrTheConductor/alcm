#ifndef __HK32F030M_TIM_H__
#define __HK32F030M_TIM_H__

#include "hk32f030m.h"

#ifdef __cplusplus
extern "C" {
#endif

// TIM Output Compare modes
#define TIM_OCMode_Timing       ((uint16_t)0x0000)
#define TIM_OCMode_Active       ((uint16_t)0x0010)
#define TIM_OCMode_Inactive     ((uint16_t)0x0020)
#define TIM_OCMode_Toggle       ((uint16_t)0x0030)
#define TIM_OCMode_PWM1         ((uint16_t)0x0060)
#define TIM_OCMode_PWM2         ((uint16_t)0x0070)

// TIM Output Compare Polarity
#define TIM_OCPolarity_High     ((uint16_t)0x0000)
#define TIM_OCPolarity_Low      ((uint16_t)0x0002)

// TIM Output Compare States
#define TIM_OutputState_Disable ((uint16_t)0x0000)
#define TIM_OutputState_Enable  ((uint16_t)0x0001)

// TIM structures
typedef struct {
    uint16_t TIM_Prescaler;
    uint16_t TIM_CounterMode;
    uint32_t TIM_Period;
    uint16_t TIM_ClockDivision;
    uint8_t TIM_RepetitionCounter;
} TIM_TimeBaseInitTypeDef;

typedef struct {
    uint16_t TIM_OCMode;
    uint16_t TIM_OutputState;
    uint16_t TIM_OutputNState;
    uint32_t TIM_Pulse;
    uint16_t TIM_OCPolarity;
    uint16_t TIM_OCNPolarity;
    uint16_t TIM_OCIdleState;
    uint16_t TIM_OCNIdleState;
} TIM_OCInitTypeDef;

typedef struct {
    uint16_t TIM_OSSRState;
    uint16_t TIM_OSSIState;
    uint16_t TIM_LOCKLevel;
    uint16_t TIM_DeadTime;
    uint16_t TIM_Break;
    uint16_t TIM_BreakPolarity;
    uint16_t TIM_AutomaticOutput;
} TIM_BDTRInitTypeDef;

// Function declarations (stubs)
void TIM_DeInit(TIM_TypeDef *TIMx);
void TIM_TimeBaseInit(TIM_TypeDef *TIMx, TIM_TimeBaseInitTypeDef *TIM_TimeBaseInitStruct);
void TIM_TimeBaseStructInit(TIM_TimeBaseInitTypeDef *TIM_TimeBaseInitStruct);
void TIM_Cmd(TIM_TypeDef *TIMx, uint32_t NewState);
void TIM_CtrlPWMOutputs(TIM_TypeDef *TIMx, uint32_t NewState);
void TIM_OC1Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct);
void TIM_OC2Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct);
void TIM_OC3Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct);
void TIM_OC4Init(TIM_TypeDef *TIMx, TIM_OCInitTypeDef *TIM_OCInitStruct);
void TIM_OCStructInit(TIM_OCInitTypeDef *TIM_OCInitStruct);
void TIM_SetCompare1(TIM_TypeDef *TIMx, uint32_t Compare);
void TIM_SetCompare2(TIM_TypeDef *TIMx, uint32_t Compare);
void TIM_SetCompare3(TIM_TypeDef *TIMx, uint32_t Compare);
void TIM_SetCompare4(TIM_TypeDef *TIMx, uint32_t Compare);
void TIM_BDTRConfig(TIM_TypeDef *TIMx, TIM_BDTRInitTypeDef *TIM_BDTRInitStruct);
void TIM_BDTRStructInit(TIM_BDTRInitTypeDef *TIM_BDTRInitStruct);

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_TIM_H__ */
