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
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h> // For malloc and free
#include <string.h>
#include <cmocka.h>

#include "test_board_mode.h"
#include "test_buzzer.h"
#include "test_footpads.h"
#include "test_power.h"
#include "test_vesc_serial.h"
#include "test_ring_buffer.h"
#include "test_crc_ccitt.h"
#include "test_function_generator.h"
#include "test_animations.h"

int main(void)
{
    int result = 0;

    result += cmocka_run_group_tests_name("Animations Tests", animations_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("Power Tests", power_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("Buzzer Tests", buzzer_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("Footpad Tests", footpads_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("VESC Serial Test", vesc_serial_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("Ring Buffer Test", ring_buffer_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("CRC CCITT Test", crc16_ccitt_tests, NULL, NULL);
    result += cmocka_run_group_tests_name("Function Generator Test", function_generator_tests, NULL,
                                          NULL);

    return (result);
}