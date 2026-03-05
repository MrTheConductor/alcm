#ifndef __HK32F030M_USART_H__
#define __HK32F030M_USART_H__

#include "hk32f030m.h"

#ifdef __cplusplus
extern "C" {
#endif

// USART Init Structure
typedef struct {
    uint32_t USART_BaudRate;
    uint16_t USART_WordLength;
    uint16_t USART_StopBits;
    uint16_t USART_Parity;
    uint16_t USART_Mode;
    uint16_t USART_HardwareFlowControl;
} USART_InitTypeDef;

// USART Flags
#define USART_FLAG_TXE   ((uint16_t)0x0080)
#define USART_FLAG_TC    ((uint16_t)0x0040)
#define USART_FLAG_RXNE  ((uint16_t)0x0020)
#define USART_FLAG_IDLE  ((uint16_t)0x0010)

// USART Interrupts
#define USART_IT_TXE     ((uint16_t)0x0727)
#define USART_IT_TC      ((uint16_t)0x0626)
#define USART_IT_RXNE    ((uint16_t)0x0525)
#define USART_IT_IDLE    ((uint16_t)0x0424)

// Function declarations (stubs)
void USART_DeInit(USART_TypeDef *USARTx);
void USART_Init(USART_TypeDef *USARTx, USART_InitTypeDef *USART_InitStruct);
void USART_StructInit(USART_InitTypeDef *USART_InitStruct);
void USART_Cmd(USART_TypeDef *USARTx, uint32_t NewState);
void USART_ITConfig(USART_TypeDef *USARTx, uint16_t USART_IT, uint32_t NewState);
void USART_SendData(USART_TypeDef *USARTx, uint16_t Data);
uint16_t USART_ReceiveData(USART_TypeDef *USARTx);
uint32_t USART_GetFlagStatus(USART_TypeDef *USARTx, uint16_t USART_FLAG);
void USART_ClearFlag(USART_TypeDef *USARTx, uint16_t USART_FLAG);
uint32_t USART_GetITStatus(USART_TypeDef *USARTx, uint16_t USART_IT);
void USART_ClearITPendingBit(USART_TypeDef *USARTx, uint16_t USART_IT);

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_USART_H__ */
