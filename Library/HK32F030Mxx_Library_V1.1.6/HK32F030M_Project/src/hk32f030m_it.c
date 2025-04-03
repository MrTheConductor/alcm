/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    hk32f030m_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************

  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "hk32f030m_it.h"
#include "hk32f030m_exti.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "button_driver.h"
#include "event_queue.h"
#include "timer.h"
#include "vesc_serial.h"
#include "interrupts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint32_t systick_ms = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void)
{
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void)
{
    /* USER CODE BEGIN HardFault_IRQn 0 */

    /* USER CODE END HardFault_IRQn 0 */
    while (1)
    {
        /* USER CODE BEGIN W1_HardFault_IRQn 0 */
        /* USER CODE END W1_HardFault_IRQn 0 */
    }
}

/**
 * @brief This function handles System service call via SWI instruction.
 */
void SVC_Handler(void)
{
    /* USER CODE BEGIN SVC_IRQn 0 */

    /* USER CODE END SVC_IRQn 0 */
    /* USER CODE BEGIN SVC_IRQn 1 */

    /* USER CODE END SVC_IRQn 1 */
}

/**
 * @brief This function handles Pendable request for system service.
 */
void PendSV_Handler(void)
{
    /* USER CODE BEGIN PendSV_IRQn 0 */

    /* USER CODE END PendSV_IRQn 0 */
    /* USER CODE BEGIN PendSV_IRQn 1 */

    /* USER CODE END PendSV_IRQn 1 */
}

/**
 * @brief This function handles System tick timer. We increment the tick counter
 * every millisecond. It will wrap around after 49 days, but no one will have
 * their board on that long. We also push an event to the event queue so the
 * timers can be processed outside of the interrupt context.
 */
void SysTick_Handler(void)
{
    systick_ms++;
    event_data_t data = {0};
    data.system_tick = systick_ms;
    event_queue_push(EVENT_SYS_TICK, &data);
}

/******************************************************************************/
/* hk32f030m Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_hk32f030m.s).                    */
/******************************************************************************/

/**
 * @brief This function handles the EXTI3 interrupt which is connected to the
 * wakeup button. We clear the interrupt pending bit and push an event to the
 * event queue with the current time.
 */
void EXTI3_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line3) != RESET)
    {
        event_data_t data = {0};
        data.button_data.time = systick_ms;
        event_queue_push(EVENT_BUTTON_WAKEUP, &data);
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}

static ring_buffer_t *usart_rx_buffer = NULL;
/**
 * @brief USART1 Interrupt Request Handler
 *
 * Called when an interrupt is triggered from USART1. Handles the following
 * interrupt sources:
 * - RXNE: Receive data register not empty
 * - IDLE: IDLE line detected
 * - ORE: Overrun error
 *
 * When RXNE is triggered, the received data is pushed onto the VESC serial
 * receive buffer. The user is responsible for processing the received data
 * outside of the interrupt context.
 *
 * When IDLE is triggered, an EVENT_SERIAL_DATA_RX event is pushed onto the
 * event queue, indicating that all data has been received.
 *
 * When ORE is triggered, the overrun error flag is cleared and the error can
 * be handled by the user.
 */
#undef UART_DEBUG
#ifdef UART_DEBUG
static uint16_t rxne_count = 0;
static uint16_t idle_count = 0;
static uint16_t ore_count = 0;
#endif
void USART1_IRQHandler(void)
{
    // Get the RX buffer if first time through
    if (usart_rx_buffer == NULL)
    {
        usart_rx_buffer = vesc_serial_get_rx_buffer();
    }

    if (USART1->ISR & USART_ISR_RXNE)
    {
        uint8_t data = USART1->RDR; // Read data clears RXNE flag
#ifdef UART_DEBUG
        rxne_count++;
#endif
        if (!ring_buffer_push(usart_rx_buffer, data))
        {
            // Buffer overflow: handle error, e.g., set an error flag
        }
    }

    if (USART1->ISR & USART_ISR_IDLE)
    {
#ifdef UART_DEBUG
        idle_count++;
#endif
        USART1->ICR = USART_ICR_IDLECF; // Clear IDLE flag
        interrupts_uninhibit_disable(); // Allow disabling interrupts again
        event_queue_push(EVENT_SERIAL_DATA_RX, NULL);
    }

    if (USART1->ISR & USART_ISR_ORE)
    {
        volatile uint8_t dummy = USART1->RDR; // Clear ORE flag by reading RDR
        USART1->ICR = USART_ICR_ORECF;        // Clear ORE flag
#ifdef UART_DEBUG
        ore_count++;
#endif
    }
}

/* USER CODE END 1 */
/************************ (C) COPYRIGHT HKMicroChip *****END OF FILE****/
