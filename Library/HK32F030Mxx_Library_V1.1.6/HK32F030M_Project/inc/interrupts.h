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
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/**
 * @brief Inhibit disabling interrupts.
 */
void interrupts_inhibit_disable(void);

/**
 * @brief Uninhibit disabling interrupts.
 */
void interrupts_uninhibit_disable(void);

/**
 * @brief Enable interrupts
 */
void interrupts_enable(void);

/**
 * @brief Disable interrupts
 */
void interrupts_disable(void);

#endif