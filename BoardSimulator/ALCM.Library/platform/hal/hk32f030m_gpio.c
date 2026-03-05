#include "hk32f030m_gpio.h"

// GPIO stub implementations - no-ops for simulation

void GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_InitStruct) {
    // No-op
}

void GPIO_DeInit(GPIO_TypeDef *GPIOx) {
    // No-op
}

void GPIO_StructInit(GPIO_InitTypeDef *GPIO_InitStruct) {
    GPIO_InitStruct->GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct->GPIO_Speed = GPIO_Speed_Level_1;
    GPIO_InitStruct->GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct->GPIO_PuPd = GPIO_PuPd_NOPULL;
}

void GPIO_SetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    // No-op
}

void GPIO_ResetBits(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    // No-op
}

void GPIO_WriteBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, BitAction BitVal) {
    // No-op
}

void GPIO_Write(GPIO_TypeDef *GPIOx, uint16_t PortVal) {
    // No-op
}

uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    return 0;
}

uint16_t GPIO_ReadInputData(GPIO_TypeDef *GPIOx) {
    return 0;
}

uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    return 0;
}

uint16_t GPIO_ReadOutputData(GPIO_TypeDef *GPIOx) {
    return 0;
}

void GPIO_PinAFConfig(GPIO_TypeDef *GPIOx, uint16_t GPIO_PinSource, uint8_t GPIO_AF) {
    // No-op
}
