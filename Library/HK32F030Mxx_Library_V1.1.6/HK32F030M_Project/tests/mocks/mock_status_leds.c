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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "status_leds.h"

lcm_status_t status_leds_init(void) {
    function_called();
    return mock_type(lcm_status_t);
}

lcm_status_t status_leds_set_color(
    const status_leds_color_t *color,
    uint8_t begin,
    uint8_t end
) {
    check_expected_ptr(color);
    check_expected(begin);
    check_expected(end);
    function_called();
    return mock_type(lcm_status_t);
}

lcm_status_t status_leds_refresh(void) {
    function_called();
    return mock_type(lcm_status_t);
}