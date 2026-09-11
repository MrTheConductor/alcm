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
#include <string.h>

#include "buzzer.h"
#include "board_mode.h"
#include "event_queue.h"
#include "footpads.h"
#include "mock_event_queue.h"
#include "mock_timer.h"
#include "settings.h"

// buzzer.c's own internals aren't declared in buzzer.h - non-static so
// tests can call/drive them directly, matching this codebase's convention.
void buzzer_reset_sequence(void);
void buzzer_play_sequence(uint16_t sequence, bool_t repeat);
EVENT_HANDLER(buzzer, command);
EVENT_HANDLER(buzzer, board_mode);
EVENT_HANDLER(buzzer, inhibited_input);
TIMER_CALLBACK(buzzer, tick);

#define BUZZER_TICK_INTERVAL_MS 10U

settings_t *settings_init_and_get(void)
{
    settings_init();
    return settings_get();
}

int buzzer_setup(void **state)
{
    (void)state;

    event_queue_init();
    timer_init();

    settings_t *settings = settings_init_and_get();
    settings->enable_beep = true;

    expect_function_call(buzzer_hw_init);
    expect_value(buzzer_hw_enable, enable, true);
    expect_value(subscribe_event, event, EVENT_COMMAND_ACK);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_NACK);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_COMMAND_TOGGLE_BEEPER);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BUTTON_DOWN);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_BUTTON_UP);
    expect_any(subscribe_event, callback);
    expect_value(subscribe_event, event, EVENT_FOOTPAD_CHANGED);
    expect_any(subscribe_event, callback);
    expect_function_call(buzzer_off);

    buzzer_init();

    return 0;
}

static void expect_play_sequence_starts_timer(void)
{
    expect_value(set_timer, timeout, BUZZER_TICK_INTERVAL_MS);
    expect_any(set_timer, callback);
    expect_value(set_timer, repeat, true);
}

void test_buzzer_init(void **state)
{
    (void)state;
}

/**
 * @brief One continuous, precisely hand-traced narrative covering
 * EVENT_HANDLER(buzzer, command) and TIMER_CALLBACK(buzzer, tick).
 *
 * buzzer_timer_id is a module static that buzzer_init() never touches, so
 * its value carries forward between separate test functions in ways that
 * are only knowable by tracing every call that can change it. Rather than
 * fight that across many small tests, this single test starts from the
 * module's true process-initial state (buzzer_timer_id == INVALID_TIMER_ID,
 * since nothing before it in this suite - test_buzzer_init - touches
 * buzzer.c's timer logic at all) and walks forward step by step, with each
 * step's comment recording buzzer_timer_id's value BEFORE that step so the
 * expectations that follow are exactly accounted for.
 */
static void test_buzzer_commands_and_ticks(void **state)
{
    (void)state;
    event_data_t data = {0};

    // Step 1: buzzer_timer_id == INVALID. ACK's own guard short-circuits
    // (first operand true), and so does play_sequence's inner guard (same
    // still-INVALID value) - is_timer_active is never called either time.
    expect_play_sequence_starts_timer();
    buzzer_command_event_handler(EVENT_COMMAND_ACK, &data);
    // buzzer_timer_id is now a real, non-invalid id.

    // Step 2: buzzer_timer_id != INVALID. ACK's outer guard calls
    // is_timer_active once; "busy" (true) short-circuits the OR, skipping
    // play_sequence (and its inner guard) entirely.
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    buzzer_command_event_handler(EVENT_COMMAND_ACK, &data);
    // buzzer_timer_id unchanged (still the same non-invalid id).

    // Step 3: "not busy" (false) this time - ACK's outer guard calls
    // is_timer_active once (false), the OR is true, play_sequence runs and
    // its OWN inner guard calls is_timer_active a second time (also
    // false), so a new timer starts.
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_play_sequence_starts_timer();
    buzzer_command_event_handler(EVENT_COMMAND_ACK, &data);
    // buzzer_timer_id is now a new non-invalid id.

    // Step 4: NACK has no outer guard - it calls play_sequence
    // unconditionally, which calls is_timer_active once. "busy" (true)
    // means the existing timer is left running (no new set_timer call) -
    // NACK's sequence still "plays" in the sense that its own waveform
    // (fg) was reinitialized, it just doesn't restart the ticking timer.
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    buzzer_command_event_handler(EVENT_COMMAND_NACK, &data);
    // buzzer_timer_id unchanged.

    // Step 5: TOGGLE_BEEPER doesn't touch buzzer_timer_id at all.
    settings_get()->enable_beep = false;
    expect_value(buzzer_hw_enable, enable, false);
    buzzer_command_event_handler(EVENT_COMMAND_TOGGLE_BEEPER, &data);

    // Step 6: an event this handler doesn't care about is a pure no-op.
    buzzer_command_event_handler(EVENT_BATTERY_LEVEL_CHANGED, &data);

    // Step 7: drive the tick callback directly with a hand-picked
    // sequence to hit both the "on" and "off" sample branches
    // deterministically, independent of buzzer_timer_id bookkeeping -
    // buzzer_play_sequence() called directly still evaluates its own
    // guard exactly as above (buzzer_timer_id != INVALID -> one
    // is_timer_active call).
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_play_sequence_starts_timer();
    buzzer_play_sequence(0xFFFFU, true); // all-on, repeating
    expect_function_call(buzzer_on);
    buzzer_tick_timer_callback(0U);

    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_play_sequence_starts_timer();
    buzzer_play_sequence(0x0000U, true); // all-off, repeating
    expect_function_call(buzzer_off);
    buzzer_tick_timer_callback(0U);

    // Step 8: a non-repeating sequence eventually exhausts (the function
    // generator stops producing samples), which resets the sequence -
    // buzzer_on/off fire an unknown number of times along the way (don't
    // care, at least once each), but the reset's own is_timer_active/
    // cancel_timer/buzzer_off calls happen exactly once, at the moment of
    // exhaustion (buzzer_timer_id becomes INVALID then, so every tick
    // after that re-enters reset_sequence but short-circuits before
    // touching is_timer_active/cancel_timer again - only buzzer_off()
    // fires again, unconditionally, each remaining tick).
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_play_sequence_starts_timer();
    buzzer_play_sequence(0xFFFFU, false); // non-repeating
    expect_function_call_any(buzzer_on);
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true); // at the moment of exhaustion
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_function_call_any(buzzer_off);
    // Comfortably past SEQUENCE_PERIOD_MS/TICK_INTERVAL_MS (320/10 = 32
    // samples) so the generator is guaranteed to have run out.
    for (uint32_t tick = 0U; tick < 50U; tick++)
    {
        buzzer_tick_timer_callback(tick * BUZZER_TICK_INTERVAL_MS);
    }
    // buzzer_timer_id is now INVALID again.
}

/**
 * @brief EVENT_HANDLER(buzzer, board_mode) - another precisely traced
 * narrative, continuing from buzzer_timer_id == INVALID (this test's
 * setup doesn't reset it, but nothing before this test in the group
 * array leaves it any other way - see the comment on the previous test).
 */
static void test_buzzer_board_mode_dispatch(void **state)
{
    (void)state;

    // 1) IDLE/SHUTTING_DOWN plays the shutdown sequence. buzzer_timer_id
    // starts INVALID here, so play_sequence's guard short-circuits - no
    // is_timer_active call.
    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_SHUTTING_DOWN;
    data.board_mode.previous_mode = BOARD_MODE_RIDING;
    expect_play_sequence_starts_timer();
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id is now non-invalid.

    // 2) IDLE, previous_mode BOOTING plays the boot sequence -
    // buzzer_timer_id is non-invalid now, so this needs one
    // is_timer_active answer.
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    data.board_mode.previous_mode = BOARD_MODE_BOOTING;
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_play_sequence_starts_timer();
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id is non-invalid (a new id).

    // 3) IDLE, neither shutting-down nor boot -> silence (reset_sequence).
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    data.board_mode.previous_mode = BOARD_MODE_IDLE;
    data.board_mode.previous_submode = BOARD_SUBMODE_IDLE_DEFAULT;
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_function_call(buzzer_off);
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id is now INVALID.

    // 4) FAULT/INTERNAL plays the fault sequence - INVALID again, so no
    // is_timer_active call.
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_FAULT;
    data.board_mode.submode = BOARD_SUBMODE_FAULT_INTERNAL;
    expect_play_sequence_starts_timer();
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id is non-invalid.

    // 5) FAULT/VESC (anything other than INTERNAL) plays the danger
    // sequence - non-invalid now, needs one is_timer_active answer.
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_FAULT;
    data.board_mode.submode = BOARD_SUBMODE_FAULT_VESC;
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_play_sequence_starts_timer();
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id is non-invalid.

    // 6) RIDING/DANGER plays the danger sequence.
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_RIDING;
    data.board_mode.submode = BOARD_SUBMODE_RIDING_DANGER;
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, false);
    expect_play_sequence_starts_timer();
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id is non-invalid.

    // 7) RIDING, anything else -> silence.
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_RIDING;
    data.board_mode.submode = BOARD_SUBMODE_RIDING_NORMAL;
    expect_any(is_timer_active, timer_id);
    will_return(is_timer_active, true);
    expect_any(cancel_timer, timer_id);
    will_return(cancel_timer, LCM_SUCCESS);
    expect_function_call(buzzer_off);
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id is now INVALID.

    // 8) Any other mode (e.g. OFF) -> silence. Already INVALID, so no
    // is_timer_active call this time.
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_OFF;
    data.board_mode.previous_mode = BOARD_MODE_IDLE;
    data.board_mode.previous_submode = BOARD_SUBMODE_IDLE_SHUTTING_DOWN;
    expect_function_call(buzzer_off);
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    // buzzer_timer_id remains INVALID.

    // 9) Entering config mode forces the beeper on, independent of the
    // mode-specific dispatch (which, for IDLE/ACTIVE with a non-BOOTING
    // previous mode, still lands on "silence" - already INVALID, so no
    // is_timer_active call).
    settings_get()->enable_beep = false;
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_CONFIG;
    data.board_mode.previous_mode = BOARD_MODE_IDLE;
    data.board_mode.previous_submode = BOARD_SUBMODE_IDLE_ACTIVE;
    expect_value(buzzer_hw_enable, enable, true);
    expect_function_call(buzzer_off);
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);

    // 10) Leaving config mode restores the user's (disabled) setting.
    memset(&data, 0, sizeof(data));
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    data.board_mode.previous_mode = BOARD_MODE_IDLE;
    data.board_mode.previous_submode = BOARD_SUBMODE_IDLE_CONFIG;
    expect_value(buzzer_hw_enable, enable, false);
    expect_function_call(buzzer_off);
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
}

/**
 * @brief footpads_get_state() here is the REAL footpads.c (linked into
 * this binary), not a plain will_return()-style mock - its internal state
 * defaults to NONE_FOOTPAD and nothing in this suite ever drives its
 * debounce timer, so it stays NONE_FOOTPAD for every test in this file.
 */
static void test_board_mode_disabled_silent_when_nothing_held(void **state)
{
    (void)state;

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_DISABLED;
    data.board_mode.previous_mode = BOARD_MODE_IDLE;
    data.board_mode.previous_submode = BOARD_SUBMODE_IDLE_ACTIVE;
    expect_function_call(buzzer_off);
    buzzer_board_mode_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
}

// ---------------------------------------------------------------------
// EVENT_HANDLER(buzzer, inhibited_input) - never touches buzzer_timer_id.
// ---------------------------------------------------------------------

static void test_inhibited_input_ignored_unless_disabled(void **state)
{
    (void)state;

    will_return(board_mode_get, BOARD_MODE_IDLE);
    event_data_t data = {0};
    buzzer_inhibited_input_event_handler(EVENT_BUTTON_DOWN, &data);
}

static void test_inhibited_input_button_down_sounds_tone(void **state)
{
    (void)state;

    will_return(board_mode_get, BOARD_MODE_DISABLED);
    expect_function_call(buzzer_on);
    event_data_t data = {0};
    buzzer_inhibited_input_event_handler(EVENT_BUTTON_DOWN, &data);
}

static void test_inhibited_input_button_up_silences(void **state)
{
    (void)state;

    will_return(board_mode_get, BOARD_MODE_DISABLED);
    expect_function_call(buzzer_on);
    event_data_t down_data = {0};
    buzzer_inhibited_input_event_handler(EVENT_BUTTON_DOWN, &down_data);

    will_return(board_mode_get, BOARD_MODE_DISABLED);
    expect_function_call(buzzer_off);
    event_data_t up_data = {0};
    buzzer_inhibited_input_event_handler(EVENT_BUTTON_UP, &up_data);
}

static void test_inhibited_input_footpad_rechecks_without_changing_held(void **state)
{
    (void)state;

    will_return(board_mode_get, BOARD_MODE_DISABLED);
    expect_function_call(buzzer_off);
    event_data_t data = {0};
    data.footpads_state = LEFT_FOOTPAD;
    buzzer_inhibited_input_event_handler(EVENT_FOOTPAD_CHANGED, &data);
}

const struct CMUnitTest buzzer_tests[] = {
    cmocka_unit_test_setup(test_buzzer_init, buzzer_setup),
    cmocka_unit_test_setup(test_buzzer_commands_and_ticks, buzzer_setup),
    cmocka_unit_test_setup(test_buzzer_board_mode_dispatch, buzzer_setup),
    cmocka_unit_test_setup(test_board_mode_disabled_silent_when_nothing_held, buzzer_setup),
    cmocka_unit_test_setup(test_inhibited_input_ignored_unless_disabled, buzzer_setup),
    cmocka_unit_test_setup(test_inhibited_input_button_down_sounds_tone, buzzer_setup),
    cmocka_unit_test_setup(test_inhibited_input_button_up_silences, buzzer_setup),
    cmocka_unit_test_setup(test_inhibited_input_footpad_rechecks_without_changing_held, buzzer_setup),
};
#endif // TEST_BUZZER_H
