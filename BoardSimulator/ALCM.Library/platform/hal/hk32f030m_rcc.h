#ifndef __HK32F030M_RCC_H__
#define __HK32F030M_RCC_H__

#include "hk32f030m.h"

#ifdef __cplusplus
extern "C" {
#endif

// RCC AHB Peripheral Clock Enable/Disable
#define RCC_AHBPeriph_GPIOA   ((uint32_t)0x00020000)
#define RCC_AHBPeriph_GPIOB   ((uint32_t)0x00040000)
#define RCC_AHBPeriph_GPIOC   ((uint32_t)0x00080000)
#define RCC_AHBPeriph_GPIOD   ((uint32_t)0x00100000)

// RCC APB2 Peripheral Clock Enable/Disable  
#define RCC_APB2Periph_USART1 ((uint32_t)0x00004000)
#define RCC_APB2Periph_ADC1   ((uint32_t)0x00000200)
#define RCC_APB2Periph_TIM1   ((uint32_t)0x00000800)
#define RCC_APB2Periph_SYSCFG ((uint32_t)0x00000001)

// RCC APB1 Peripheral Clock Enable/Disable
#define RCC_APB1Periph_TIM3   ((uint32_t)0x00000002)

// Function declarations (stubs)
void RCC_AHBPeriphClockCmd(uint32_t RCC_AHBPeriph, uint32_t NewState);
void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, uint32_t NewState);
void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, uint32_t NewState);
void RCC_APB2PeriphResetCmd(uint32_t RCC_APB2Periph, uint32_t NewState);
void RCC_APB1PeriphResetCmd(uint32_t RCC_APB1Periph, uint32_t NewState);

#define ENABLE  1
#define DISABLE 0

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_RCC_H__ */
