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
 * @file mock_board_mode.c
 * @brief CMocka-based mock implementation for board_mode.h functions.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "board_mode.h"

/**
 * @brief Mock implementation of board_mode_init.
 */
lcm_status_t board_mode_init(void)
{
    function_called();
    return LCM_SUCCESS;
}

/**
 * @brief Mock implementation of board_mode_get.
 * @return The current board mode.
 */
board_mode_t board_mode_get(void)
{
    return (board_mode_t)mock();
}

/**
 * @brief Mock implementation of board_submode_get.
 * @return The current board submode.
 */
board_submode_t board_submode_get(void)
{
    return (board_submode_t)mock();
}