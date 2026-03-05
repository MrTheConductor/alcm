#ifndef __HK32F030M_GPIO_H__
#define __HK32F030M_GPIO_H__

#include "hk32f030m.h"

#ifdef __cplusplus
extern "C" {
#endif

// GPIO Mode enumeration
typedef enum {
    GPIO_Mode_IN = 0x00,
    GPIO_Mode_OUT = 0x01,
    GPIO_Mode_AF = 0x02,
    GPIO_Mode_AN = 0x03
} GPIOMode_TypeDef;

// GPIO Output type enumeration
typedef enum {
    GPIO_OType_PP = 0x00,
    GPIO_OType_OD = 0x01
} GPIOOType_TypeDef;

// GPIO Speed enumeration
typedef enum {
    GPIO_Speed_Level_1 = 0x00,
    GPIO_Speed_Level_2 = 0x01,
    GPIO_Speed_Level_3 = 0x02,
    GPIO_Speed_Level_4 = 0x03
} GPIOSpeed_TypeDef;

// GPIO Pull-Up/Pull-Down enumeration
typedef enum {
    GPIO_PuPd_NOPULL = 0x00,
    GPIO_PuPd_UP = 0x01,
    GPIO_PuPd_DOWN = 0x02
} GPIOPuPd_TypeDef;

// GPIO Bit SET and Bit RESET enumeration
typedef enum {
    Bit_RESET = 0,
    Bit_SET
} BitAction;

// GPIO Init structure
typedef struct {
    uint32_t GPIO_Pin;
    GPIOMode_TypeDef GPIO_Mode;
    GPIOSpeed_TypeDef GPIO_Speed;
    GPIOOType_TypeDef GPIO_OType;
    GPIOPuPd_TypeDef GPIO_PuPd;
} GPIO_InitTypeDef;

// GPIO Pin definitions
#define GPIO_Pin_0     ((uint16_t)0x0001)
#define GPIO_Pin_1     ((uint16_t)0x0002)
#define GPIO_Pin_2     ((uint16_t)0x0004)
#define GPIO_Pin_3     ((uint16_t)0x0008)
#define GPIO_Pin_4     ((uint16_t)0x0010)
#define GPIO_Pin_5     ((uint16_t)0x0020)
#define GPIO_Pin_6     ((uint16_t)0x0040)
#define GPIO_Pin_7     ((uint16_t)0x0080)
#define GPIO_Pin_8     ((uint16_t)0x0100)
#define GPIO_Pin_9     ((uint16_t)0x0200)
#define GPIO_Pin_10    ((uint16_t)0x0400)
#define GPIO_Pin_11    ((uint16_t)0x0800)
#define GPIO_Pin_12    ((uint16_t)0x1000)
#define GPIO_Pin_13    ((uint16_t)0x2000)
#define GPIO_Pin_14    ((uint16_t)0x4000)
#define GPIO_Pin_15    ((uint16_t)0x8000)
#define GPIO_Pin_All   ((uint16_t)0xFFFF)

// Function declarations (stubs)
void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct);
void GPIO_DeInit(GPIO_TypeDef *GPIOx);
void GPIO_StructInit(GPIO_InitTypeDef *GPIO_InitStruct);
void GPIO_SetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void GPIO_ResetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void GPIO_WriteBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, BitAction BitVal);
void GPIO_Write(GPIO_TypeDef *GPIOx, uint16_t PortVal);
uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
uint16_t GPIO_ReadInputData(GPIO_TypeDef *GPIOx);
uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
uint16_t GPIO_ReadOutputData(GPIO_TypeDef *GPIOx);
void GPIO_PinAFConfig(GPIO_TypeDef *GPIOx, uint16_t GPIO_PinSource, uint8_t GPIO_AF);

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_GPIO_H__ */
