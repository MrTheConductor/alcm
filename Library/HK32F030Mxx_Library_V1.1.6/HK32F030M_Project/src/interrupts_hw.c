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
#include "interrupts.h"
#include "hk32f030m.h"

/**
 * @brief Enables interrupts
 */
void interrupts_enable(void)
{
    __enable_irq();
}

/**
 * @brief Disables interrupts
 */
void interrupts_disable(void)
{
    __disable_irq();
}

/**
 * @brief Use the WFE instruction to wait for an event
 */
void wait_for_event(void)
{
    __WFE();
}

/**
 * @brief Use the SEV instruction to send an event
 */
void send_event(void)
{
    __SEV();
}

// Newline at end of file
