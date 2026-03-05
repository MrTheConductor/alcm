#ifndef __HK32F030M_EXTI_H__
#define __HK32F030M_EXTI_H__

#include "hk32f030m.h"

#ifdef __cplusplus
extern "C" {
#endif

// EXTI mode enumeration
typedef enum {
    EXTI_Mode_Interrupt = 0x00,
    EXTI_Mode_Event = 0x04
} EXTIMode_TypeDef;

// EXTI Trigger enumeration
typedef enum {
    EXTI_Trigger_Rising = 0x08,
    EXTI_Trigger_Falling = 0x0C,
    EXTI_Trigger_Rising_Falling = 0x10
} EXTITrigger_TypeDef;

// EXTI Init Structure
typedef struct {
    uint32_t EXTI_Line;
    EXTIMode_TypeDef EXTI_Mode;
    EXTITrigger_TypeDef EXTI_Trigger;
    uint32_t EXTI_LineCmd;
} EXTI_InitTypeDef;

// EXTI Lines
#define EXTI_Line0   ((uint32_t)0x00001)
#define EXTI_Line1   ((uint32_t)0x00002)
#define EXTI_Line2   ((uint32_t)0x00004)
#define EXTI_Line3   ((uint32_t)0x00008)
#define EXTI_Line4   ((uint32_t)0x00010)

// Function declarations (stubs)
void EXTI_DeInit(void);
void EXTI_Init(EXTI_InitTypeDef *EXTI_InitStruct);
void EXTI_StructInit(EXTI_InitTypeDef *EXTI_InitStruct);
uint32_t EXTI_GetFlagStatus(uint32_t EXTI_Line);
void EXTI_ClearFlag(uint32_t EXTI_Line);
uint32_t EXTI_GetITStatus(uint32_t EXTI_Line);
void EXTI_ClearITPendingBit(uint32_t EXTI_Line);

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_EXTI_H__ */
