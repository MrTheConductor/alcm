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
#ifndef POWER_HW_H
#define POWER_HW_H

/**
 * @enum power_hw_t
 * @brief Enumerates the power hardware states.
 */
typedef enum
{
    POWER_HW_ON = 0, /**< Power hardware is on */
    POWER_HW_OFF     /**< Power hardware is off */
} power_hw_t;

/**
 * @brief Initializes the power hardware module.
 */
void power_hw_init(void);

/**
 * @brief Sets the power state of the hardware.
 * @param power_hw The desired power state.
 */
void power_hw_set_power(power_hw_t power_hw);

/**
 * @brief Sets the charge state of the hardware.
 * @param power_hw The desired charge state.
 */
void power_hw_set_charge(power_hw_t power_hw);

#endif