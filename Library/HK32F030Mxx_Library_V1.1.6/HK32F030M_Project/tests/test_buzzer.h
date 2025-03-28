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
#ifndef TEST_BUZZER_H
#define TEST_BUZZER_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "buzzer.h"
#include "event_queue.h"
#include "mock_event_queue.h"
#include "mock_timer.h"
#include "settings.h"

settings_t *settings_init_and_get(void)
{
    settings_init();
    return settings_get();
}

int buzzer_setup(void **state)
{
    (void)state; // Unused

    // Reset event queue and timer
    event_queue_init();
    timer_init();

    settings_t *settings = settings_init_and_get();
    settings->enable_beep = true;

    // Expect the hardware initialization and enable functions to be called
    expect_function_call(buzzer_hw_init);
    expect_value(buzzer_hw_enable, enable, true);

    // Expect the event subscription functions to be called
    expect_value(subscribe_event, event, EVENT_COMMAND_ACK);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_ACK_2);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_NACK);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_TOGGLE_BEEPER);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);

    // Expect the buzzer to be turned off
    expect_function_call(buzzer_off);

    buzzer_init();

    return 0;
}

void test_buzzer_init(void **state)
{
    (void)state; // Unused
}

const struct CMUnitTest buzzer_tests[] = {cmocka_unit_test_setup(test_buzzer_init, buzzer_setup)};
#endif // TEST_BUZZER_H