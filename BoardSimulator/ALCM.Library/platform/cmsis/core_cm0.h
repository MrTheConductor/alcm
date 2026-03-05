#ifndef __CORE_CM0_H__
#define __CORE_CM0_H__

#include <stdint.h>

// Minimal CMSIS Core definitions for ARM Cortex-M0 compatibility
// This stub allows ALCM to compile on Windows without the actual ARM headers

#ifdef __cplusplus
extern "C" {
#endif

// Interrupt control functions (no-ops on Windows)
static inline void __enable_irq(void) { }
static inline void __disable_irq(void) { }
static inline void __WFE(void) { }
static inline void __SEV(void) { }
static inline void __NOP(void) { }
static inline void __WFI(void) { }

// Memory barriers (no-ops on x86/x64)
static inline void __DMB(void) { }
static inline void __DSB(void) { }
static inline void __ISB(void) { }

// NVIC (Nested Vectored Interrupt Controller) - stub
typedef struct {
    uint32_t ISER[1];
    uint32_t RESERVED0[31];
    uint32_t ICER[1];
    uint32_t RESERVED1[31];
    uint32_t ISPR[1];
    uint32_t RESERVED2[31];
    uint32_t ICPR[1];
    uint32_t RESERVED3[31];
    uint32_t RESERVED4[64];
    uint32_t IP[8];
} NVIC_Type;

// SysTick Timer - stub
typedef struct {
    uint32_t CTRL;
    uint32_t LOAD;
    uint32_t VAL;
    uint32_t CALIB;
} SysTick_Type;

// Peripheral base addresses (not used in simulation)
#define SCS_BASE            (0xE000E000UL)
#define SysTick_BASE        (SCS_BASE + 0x0010UL)
#define NVIC_BASE           (SCS_BASE + 0x0100UL)

#define SysTick             ((SysTick_Type *)SysTick_BASE)
#define NVIC                ((NVIC_Type *)NVIC_BASE)

// SysTick functions (implemented in systick_sim.c)
extern uint32_t SysTick_Config(uint32_t ticks);

// NVIC functions (no-ops)
static inline void NVIC_EnableIRQ(int32_t IRQn) { }
static inline void NVIC_DisableIRQ(int32_t IRQn) { }
static inline void NVIC_SetPriority(int32_t IRQn, uint32_t priority) { }
static inline void NVIC_SystemReset(void) { }

#ifdef __cplusplus
}
#endif

#endif /* __CORE_CM0_H__ */
