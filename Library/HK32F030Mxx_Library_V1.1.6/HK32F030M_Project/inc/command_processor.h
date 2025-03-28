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
#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "lcm_types.h"

/**
 * @brief Enumeration for command processor contexts
 */
typedef enum
{
    COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS,
    COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS,
    COMMAND_PROCESSOR_CONTEXT_PERSONAL_COLOR,
    COMMAND_PROCESSOR_CONTEXT_BOOT_ANIMATION,
    COMMAND_PROCESSOR_CONTEXT_IDLE_ANIMATION,
    COMMAND_PROCESSOR_CONTEXT_DOZING_ANIMATION,
    COMMAND_PROCESSOR_CONTEXT_RIDING_ANIMATION,
    COMMAND_PROCESSOR_CONTEXT_SHUTDOWN_ANIMATION,

    // Count of all non-default contexts
    COMMAND_PROCESSOR_CONTEXT_COUNT,
    COMMAND_PROCESSOR_CONTEXT_DEFAULT
} command_processor_context_t;

/**
 * @brief Enumeration for command processor events
 */
lcm_status_t command_processor_init(void);

#endif /* COMMAND_PROCESSOR_H */