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

volatile static uint8_t inhibit_disable_count = 0;

/**
 * @brief Inhibit disabling interrupts.
 */
void interrupts_inhibit_disable(void)
{
    if (inhibit_disable_count < 255)
    {
        inhibit_disable_count++;
    }
}

/**
 * @brief Uninhibit disabling interrupts.
 */
void interrupts_uninhibit_disable(void)
{
    if (inhibit_disable_count > 0)
    {
        inhibit_disable_count--;
    }
}

/**
 * @brief Enables interrupts
 */
void interrupts_enable(void)
{
    __set_PRIMASK(0);
}

/**
 * @brief Disables interrupts
 */
void interrupts_disable(void)
{
    uint32_t timeout = 1000000; // Timeout for busy wait

    while ((inhibit_disable_count > 0) &&
           (timeout > 0))
    {
        // Busy wait until the inhibit_disable_count is zero
        timeout--;
    }
    __set_PRIMASK(1);
}

// Newline at end of file
