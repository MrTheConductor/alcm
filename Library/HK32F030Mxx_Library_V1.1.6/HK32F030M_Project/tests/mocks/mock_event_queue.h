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
#ifndef _MOCK_EVENT_QUEUE_H_
#define _MOCK_EVENT_QUEUE_H_
#include <stdint.h>
#include "event_queue.h"

void event_queue_call_mocked_callback(event_type_t event, const event_data_t* data);
void event_queue_test_bad_event(event_type_t expected, event_type_t actual, const event_data_t* data);

// Validation functions
int validate_footpads_state(const uintmax_t data, const uintmax_t check_data);
int validate_board_mode_event_data(const uintmax_t data, const uintmax_t check_data);

#endif