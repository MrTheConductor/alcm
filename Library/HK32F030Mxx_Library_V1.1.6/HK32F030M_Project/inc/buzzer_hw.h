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
 *
 */
#ifndef BUZZER_HW_H
#define BUZZER_HW_H

#include <stdbool.h>

/**
 * @brief Initializes the hardware for the buzzer.
 *
 * This function sets up the necessary hardware configurations to enable
 * the buzzer functionality. It should be called before any other buzzer
 * related functions are used.
 */
void buzzer_hw_init(void);

/**
 * @brief Enables or disables the buzzer.
 *
 * @param enable True to enable the buzzer, false to disable it.
 */
void buzzer_hw_enable(bool enable);

/**
 * @brief Turns off the buzzer.
 *
 * This function turns off the buzzer.
 */
void buzzer_off(void);

/**
 * @brief Turns on the buzzer.
 *
 * This function turns on the buzzer.
 */
void buzzer_on(void);

#endif