/*
 * BoardSimulator - Interrupt handlers stub
 * Replaces hk32f030m_it.c with minimal implementation
 */

#include "hk32f030m.h"
#include "event_queue.h"
#include <stdint.h>

// Systick counter
static volatile uint32_t systick_ms = 0;

// SysTick interrupt handler - called by systick_sim.c
void SysTick_Handler(void) {
    // Increment millisecond counter
    systick_ms++;
    
    // Push system tick event to event queue (timer module subscribes to this)
    event_data_t data = {0};
    data.system_tick = systick_ms;
    event_queue_push(EVENT_SYS_TICK, &data);
}

// USART1 interrupt handler - simulated for VESC serial
// In simulation, this is called from hw_inject_vesc_data after injecting bytes
void USART1_IRQHandler(void) {
    // In real hardware, this reads UART and pushes to ring buffer
    // In simulation, data is injected directly to ring buffer by hw_inputs.c
    // We just need to trigger the IDLE event to notify vesc_serial
    event_queue_push(EVENT_SERIAL_DATA_RX, NULL);
}

// Other interrupt handlers - not used in simulation
void NMI_Handler(void) { }
void HardFault_Handler(void) { while(1); }
void SVC_Handler(void) { }
void PendSV_Handler(void) { }
void WWDG_IRQHandler(void) { }
void PVD_IRQHandler(void) { }
void RTC_IRQHandler(void) { }
void FLASH_IRQHandler(void) { }
void RCC_IRQHandler(void) { }
void EXTI0_1_IRQHandler(void) { }
void EXTI2_3_IRQHandler(void) { }
void EXTI4_15_IRQHandler(void) { }
void DMA1_Channel1_IRQHandler(void) { }
void DMA1_Channel2_3_IRQHandler(void) { }
void DMA1_Channel4_5_IRQHandler(void) { }
void ADC1_IRQHandler(void) { }
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) { }
void TIM1_CC_IRQHandler(void) { }
void TIM3_IRQHandler(void) { }
void TIM14_IRQHandler(void) { }
void TIM16_IRQHandler(void) { }
void TIM17_IRQHandler(void) { }
void I2C1_IRQHandler(void) { }
void SPI1_IRQHandler(void) { }
