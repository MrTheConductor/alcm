/*
 * Copyright (c) 2024-2025, Mitchell White <mitchell.n.white@gmail.com>
 *
 * This file is part of Advanced LCM (ALCM) project.
 *
 * ALCM is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ALCM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with ALCM. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file main.c
 * @brief Main file for the Advanced LCM (ALCM) project.
 *
 * This file contains the main function and system initialization code for
 * the ALCM project. It sets up the system components and enters an
 * infinite loop to process events.
 */
#include "board_mode.h"
#include "button_driver.h"
#include "button_events.h"
#include "buzzer.h"
#include "command_processor.h"
#include "config.h"
#include "event_queue.h"
#include "footpads.h"
#include "headlights.h"
#include "hk32f030m.h"
#include "main.h"
#include "power.h"
#include "status_leds.h"
#include "tim1.h"
#include "timer.h"
#include "vesc_serial.h"

// Macro to check the return status of initialization functions and set the status variable
// accordingly
#define INIT(x)                                                                                    \
    if (LCM_SUCCESS != x##_init())                                                                 \
    status = LCM_ERROR

/**
 * @brief Initializes the system by setting up the systick timer and the various
 *        system components.
 */
lcm_status_t system_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Configure systick for 1ms
    if (0U != SysTick_Config(SystemCoreClock / 1000))
    {
        status = LCM_ERROR;
    }

    INIT(TIM1);
    INIT(command_processor);
    INIT(timer);
    INIT(button_driver);
    INIT(button_events);
    INIT(board_mode);
    INIT(power);
#ifdef ENABLE_BUZZER
    INIT(buzzer);
#endif // ENABLE_BUZZER
    INIT(headlights);
    INIT(footpads);
#ifdef ENABLE_STATUS_LEDS
    INIT(status_leds);
#endif
    INIT(vesc_serial);

    return status;
}

/**
 * @brief Main function of the program.
 *
 * This function initializes the system and enters an infinite loop
 * to continually process the message pump.
 *
 * @return int Exit status of the program (not expected to return).
 */
int main(void)
{
    if (LCM_SUCCESS != system_init())
    {
        // System initialization failed
        NVIC_SystemReset(); // Reset the system
    }

    // The button press latches the power to the LCM, so we just boot
    // immediately
    event_queue_push(EVENT_COMMAND_BOOT, NULL);

    // Infinite loop
    while (1)
    {
        // If the message pump returns an error, we should probably
        // reboot the system, but that would disable power to the VESC
        // and potentially injure the rider.
        //
        // Instead, we will attempt to put an error on the queue and
        // try to recover.
        if (LCM_ERROR == event_queue_pop_and_notify())
        {
            // Error processing the event queue
            fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
        }
    }

    // Never reached
    return 0;
}

// Newline at end of file
