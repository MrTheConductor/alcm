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
#ifndef _BOARD_MODE_H_
#define _BOARD_MODE_H_

#include <stdint.h>
#include "lcm_types.h"

/**
 * @enum board_mode_t
 * @brief Represents the various operational modes of the board.
 */
typedef enum
{
    BOARD_MODE_UNKNOWN = 0, /**< Mode is unknown. */
    BOARD_MODE_OFF,         /**< Board is turned off. */
    BOARD_MODE_BOOTING,     /**< Board is booting up. */
    BOARD_MODE_IDLE,        /**< Board is idle. */
    BOARD_MODE_RIDING,      /**< Board is active and in riding mode. */
    BOARD_MODE_CHARGING,    /**< Board is charging. */
    BOARD_MODE_FAULT        /**< Board has encountered a fault. */
} board_mode_t;

/**
 * @enum board_submode_t
 * @brief Represents the various submodes within each operational mode.
 */
typedef enum
{
    BOARD_SUBMODE_UNDEFINED = 0, // Submode is undefined

    // Idle submodes
    BOARD_SUBMODE_IDLE_ACTIVE,        // Board is active in idle mode
    BOARD_SUBMODE_IDLE_DEFAULT,       // Board is in default idle mode
    BOARD_SUBMODE_IDLE_DOZING,        // Board is dozing
    BOARD_SUBMODE_IDLE_SHUTTING_DOWN, // Board is shutting down
    BOARD_SUBMODE_IDLE_CONFIG,        // Board is in configuration mode

    // Riding submodes
    BOARD_SUBMODE_RIDING_STOPPED, // Stopped riding submode
    BOARD_SUBMODE_RIDING_SLOW,    // Slow riding submode
    BOARD_SUBMODE_RIDING_NORMAL,  // Normal riding submode
    BOARD_SUBMODE_RIDING_WARNING, // Fast riding submode (duty cycle > 80%)
    BOARD_SUBMODE_RIDING_DANGER,  // Danger riding submode (duty cycle > 90%)

    // Fault submodes
    BOARD_SUBMODE_FAULT_INTERNAL,   // Internal fault submode
    BOARD_SUBMODE_FAULT_VESC        // VESC fault submode
} board_submode_t;

// Public functions
lcm_status_t board_mode_init(void);

/*
 * The board has several high-level modes, defined in board_mode_t.
 * There are also sub-modes within each mode, defined in board_submode_t.
 *
 * Whenever the mode or submode changes, the board_mode_changed event is raised.
 */
board_mode_t board_mode_get(void);
board_submode_t board_submode_get(void);

#endif