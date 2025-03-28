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

int main(void)
{
    int result = 0;

    result += cmocka_run_group_tests_name("Board Mode Test", board_mode_tests, NULL, NULL);

    return (result);
}