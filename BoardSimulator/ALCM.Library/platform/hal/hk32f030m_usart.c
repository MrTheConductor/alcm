#include "hk32f030m_usart.h"

// USART stub implementations - no-ops for simulation

void USART_DeInit(USART_TypeDef *USARTx) {
    // No-op
}

void USART_Init(USART_TypeDef *USARTx, USART_InitTypeDef *USART_InitStruct) {
    // No-op
}

void USART_StructInit(USART_InitTypeDef *USART_InitStruct) {
    USART_InitStruct->USART_BaudRate = 9600;
    USART_InitStruct->USART_WordLength = 0;
    USART_InitStruct->USART_StopBits = 0;
    USART_InitStruct->USART_Parity = 0;
    USART_InitStruct->USART_Mode = 0;
    USART_InitStruct->USART_HardwareFlowControl = 0;
}

void USART_Cmd(USART_TypeDef *USARTx, uint32_t NewState) {
    // No-op
}

void USART_ITConfig(USART_TypeDef *USARTx, uint16_t USART_IT, uint32_t NewState) {
    // No-op
}

void USART_SendData(USART_TypeDef *USARTx, uint16_t Data) {
    // No-op
}

uint16_t USART_ReceiveData(USART_TypeDef *USARTx) {
    return 0;
}

uint32_t USART_GetFlagStatus(USART_TypeDef *USARTx, uint16_t USART_FLAG) {
    return 0;
}

void USART_ClearFlag(USART_TypeDef *USARTx, uint16_t USART_FLAG) {
    // No-op
}

uint32_t USART_GetITStatus(USART_TypeDef *USARTx, uint16_t USART_IT) {
    return 0;
}

void USART_ClearITPendingBit(USART_TypeDef *USARTx, uint16_t USART_IT) {
    // No-op
}
