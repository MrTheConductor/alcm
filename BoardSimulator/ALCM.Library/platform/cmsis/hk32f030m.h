#ifndef __HK32F030M_H__
#define __HK32F030M_H__

#include <stdint.h>
#include "core_cm0.h"

// Minimal HK32F030M device header for Windows compilation
// Provides peripheral register structures as stubs

#ifdef __cplusplus
extern "C" {
#endif

// Interrupt numbers (not used in simulation)
typedef enum IRQn {
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn = -13,
    SVC_IRQn = -5,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1,
    WWDG_IRQn = 0,
    PVD_IRQn = 1,
    RTC_IRQn = 2,
    FLASH_IRQn = 3,
    RCC_IRQn = 4,
    EXTI0_1_IRQn = 5,
    EXTI2_3_IRQn = 6,
    EXTI4_15_IRQn = 7,
    DMA1_Channel1_IRQn = 9,
    DMA1_Channel2_3_IRQn = 10,
    DMA1_Channel4_5_IRQn = 11,
    ADC1_IRQn = 12,
    TIM1_BRK_UP_TRG_COM_IRQn = 13,
    TIM1_CC_IRQn = 14,
    TIM3_IRQn = 16,
    TIM14_IRQn = 19,
    TIM16_IRQn = 21,
    TIM17_IRQn = 22,
    I2C1_IRQn = 23,
    SPI1_IRQn = 25,
    USART1_IRQn = 27
} IRQn_Type;

// Peripheral register structures (minimal stubs)
typedef struct {
    uint32_t MODER;
    uint32_t OTYPER;
    uint32_t RESERVED0;
    uint32_t PUPDR;
    uint32_t IDR;
    uint32_t ODR;
    uint32_t BSRR;
    uint32_t LCKR;
    uint32_t AFR[2];
    uint32_t BRR;
} GPIO_TypeDef;

typedef struct {
    uint32_t CR;
    uint32_t CFGR;
    uint32_t CIR;
    uint32_t APB2RSTR;
    uint32_t APB1RSTR;
    uint32_t AHBENR;
    uint32_t APB2ENR;
    uint32_t APB1ENR;
    uint32_t BDCR;
    uint32_t CSR;
    uint32_t AHBRSTR;
    uint32_t CFGR2;
    uint32_t CFGR3;
    uint32_t CR2;
} RCC_TypeDef;

typedef struct {
    uint32_t CR1;
    uint32_t CR2;
    uint32_t SMCR;
    uint32_t DIER;
    uint32_t SR;
    uint32_t EGR;
    uint32_t CCMR1;
    uint32_t CCMR2;
    uint32_t CCER;
    uint32_t CNT;
    uint32_t PSC;
    uint32_t ARR;
    uint32_t RCR;
    uint32_t CCR1;
    uint32_t CCR2;
    uint32_t CCR3;
    uint32_t CCR4;
    uint32_t BDTR;
    uint32_t DCR;
    uint32_t DMAR;
} TIM_TypeDef;

typedef struct {
    uint32_t ISR;
    uint32_t RESERVED0;
    uint32_t CR;
    uint32_t CFGR1;
    uint32_t CFGR2;
    uint32_t SMPR;
    uint32_t RESERVED1[2];
    uint32_t TR;
    uint32_t RESERVED2;
    uint32_t CHSELR;
    uint32_t RESERVED3[5];
    uint32_t DR;
} ADC_TypeDef;

typedef struct {
    uint32_t SR;
    uint32_t DR;
    uint32_t BRR;
    uint32_t CR1;
    uint32_t CR2;
    uint32_t CR3;
    uint32_t GTPR;
} USART_TypeDef;

typedef struct {
    uint32_t IMR;
    uint32_t EMR;
    uint32_t RTSR;
    uint32_t FTSR;
    uint32_t SWIER;
    uint32_t PR;
} EXTI_TypeDef;

typedef struct {
    uint32_t ACR;
    uint32_t KEYR;
    uint32_t OPTKEYR;
    uint32_t SR;
    uint32_t CR;
    uint32_t AR;
    uint32_t RESERVED;
    uint32_t OBR;
    uint32_t WRPR;
} FLASH_TypeDef;

// Peripheral memory map (dummy addresses for simulation)
#define FLASH_BASE          0x08000000UL
#define SRAM_BASE           0x20000000UL
#define PERIPH_BASE         0x40000000UL

#define GPIOA_BASE          (PERIPH_BASE + 0x10000)
#define GPIOB_BASE          (PERIPH_BASE + 0x10400)
#define GPIOC_BASE          (PERIPH_BASE + 0x10800)
#define GPIOD_BASE          (PERIPH_BASE + 0x10C00)
#define RCC_BASE            (PERIPH_BASE + 0x21000)
#define FLASH_R_BASE        (PERIPH_BASE + 0x22000)
#define EXTI_BASE           (PERIPH_BASE + 0x10400)
#define ADC1_BASE           (PERIPH_BASE + 0x12400)
#define TIM1_BASE           (PERIPH_BASE + 0x12C00)
#define USART1_BASE         (PERIPH_BASE + 0x13800)

// Peripheral declarations
#define GPIOA               ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *)GPIOD_BASE)
#define RCC                 ((RCC_TypeDef *)RCC_BASE)
#define FLASH               ((FLASH_TypeDef *)FLASH_R_BASE)
#define EXTI                ((EXTI_TypeDef *)EXTI_BASE)
#define ADC1                ((ADC_TypeDef *)ADC1_BASE)
#define TIM1                ((TIM_TypeDef *)TIM1_BASE)
#define USART1              ((USART_TypeDef *)USART1_BASE)

// System clock
extern uint32_t SystemCoreClock;

void SystemInit(void);
void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_H__ */
