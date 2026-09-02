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
#include <stddef.h>

#include "board_mode.h"
#include "buzzer.h"
#include "buzzer_hw.h"
#include "config.h"
#include "event_queue.h"
#include "footpads.h"
#include "function_generator.h"
#include "settings.h"
#include "timer.h"

#define TICK_INTERVAL_MS 10
#define SEQUENCE_PERIOD_MS 320

// Buzzer sequences
#define ACK_SEQUENCE 0xC000
#define NACK_SEQUENCE 0xCC00
#define SHUTDOWN_SEQUENCE 0xC000
#define DANGER_SEQUENCE 0xF0F0
#define FAULT_SEQUENCE 0xAAAF
#define BOOT_SEQUENCE 0xF300

// Static variables
static settings_t *buzzer_settings = NULL;
static function_generator_t fg = {0};
static timer_id_t buzzer_timer_id = INVALID_TIMER_ID;
#ifdef ENABLE_APP_INTEGRATION
static bool_t inhibited_button_held = false;
#endif

// Forward declarations
// Event handlers
EVENT_HANDLER(buzzer, command);
EVENT_HANDLER(buzzer, board_mode);
#ifdef ENABLE_APP_INTEGRATION
EVENT_HANDLER(buzzer, inhibited_input);
static void update_inhibited_tone(void);
#endif

// Timer callbacks
TIMER_CALLBACK(buzzer, tick);

lcm_status_t buzzer_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Get settings
    buzzer_settings = settings_get();
    if (buzzer_settings == NULL)
    {
        status = LCM_ERROR;
    }
    else
    {
        // Init hardware
        buzzer_hw_init();
        buzzer_hw_enable(buzzer_settings->enable_beep);

        // Subscribe to events
        SUBSCRIBE_EVENT(buzzer, EVENT_COMMAND_ACK, command);
        SUBSCRIBE_EVENT(buzzer, EVENT_COMMAND_NACK, command);
        SUBSCRIBE_EVENT(buzzer, EVENT_COMMAND_TOGGLE_BEEPER, command);
        SUBSCRIBE_EVENT(buzzer, EVENT_BOARD_MODE_CHANGED, board_mode);
#ifdef ENABLE_APP_INTEGRATION
        SUBSCRIBE_EVENT(buzzer, EVENT_BUTTON_DOWN, inhibited_input);
        SUBSCRIBE_EVENT(buzzer, EVENT_BUTTON_UP, inhibited_input);
        SUBSCRIBE_EVENT(buzzer, EVENT_FOOTPAD_CHANGED, inhibited_input);
#endif

        // Set initial state
        buzzer_off();
    }
    return status;
}

/**
 * @brief Resets the current buzzer sequence
 *
 * This function is used to reset the current sequence when it is
 * finished or when a new sequence is started.
 */
void buzzer_reset_sequence(void)
{
    // Stop the timer
    if (buzzer_timer_id != INVALID_TIMER_ID && is_timer_active(buzzer_timer_id))
    {
        cancel_timer(buzzer_timer_id);
    }
    buzzer_timer_id = INVALID_TIMER_ID;
    buzzer_off();
}

/**
 * @brief Timer callback for the buzzer
 *
 * This function is called every TICK_INTERVAL_MS to update the buzzer.
 * It uses the function generator to get the next sample and sets the
 * buzzer accordingly.
 */
TIMER_CALLBACK(buzzer, tick)
{
    // Ignore unused parameter
    (void)system_tick;
    fixed16_t sample = 0;

    if (function_generator_next_sample(&fg, &sample) != LCM_SUCCESS)
    {
        buzzer_reset_sequence();
    }
    else
    {
        if (sample <= 0)
        {
            buzzer_off();
        }
        else
        {
            buzzer_on();
        }
    }
}

/**
 * @brief Plays a buzzer sequence
 *
 * This function plays a buzzer sequence using the function generator.
 */
void buzzer_play_sequence(uint16_t sequence, bool_t repeat)
{
    function_generator_init(&fg, FUNCTION_GENERATOR_SEQUENCE, SEQUENCE_PERIOD_MS, TICK_INTERVAL_MS,
                            FIXED16(0.0), FIXED16(1.0), repeat ? FG_FLAG_REPEAT : 0, sequence);

    // Start the timer
    if (buzzer_timer_id == INVALID_TIMER_ID || !is_timer_active(buzzer_timer_id))
    {
        buzzer_timer_id = set_timer(TICK_INTERVAL_MS, TIMER_CALLBACK_NAME(buzzer, tick), true);
    }
}

/**
 * @brief Event handler for the buzzer module command events
 *
 * This function plays different sequences based on the command event.
 */
EVENT_HANDLER(buzzer, command)
{
    switch (event)
    {
    case EVENT_COMMAND_ACK:
        // Play the ack only if nothing else is currently playing
        if (buzzer_timer_id == INVALID_TIMER_ID || !is_timer_active(buzzer_timer_id))
        {
            buzzer_play_sequence(ACK_SEQUENCE, false);
        }
        break;
    case EVENT_COMMAND_NACK:
        buzzer_play_sequence(NACK_SEQUENCE, false);
        break;
    case EVENT_COMMAND_TOGGLE_BEEPER:
        buzzer_hw_enable(buzzer_settings->enable_beep);
        break;
    default:
        break;
    }
}

#ifdef ENABLE_APP_INTEGRATION
/**
 * @brief Starts or stops the inhibited-input buzzer tone
 *
 * Bypasses the sequence/timer machinery entirely (safe here since
 * BOARD_MODE_DISABLED entry already cancels any prior sequence via
 * buzzer_reset_sequence() below) - a direct buzzer_on()/buzzer_off() call is
 * all a constant tone needs. Footpad state is read live via
 * footpads_get_state() rather than cached, so a foot already resting on a
 * pad when locked sounds the tone immediately, not just on the next press.
 */
static void update_inhibited_tone(void)
{
    if (inhibited_button_held || footpads_get_state() != NONE_FOOTPAD)
    {
        buzzer_on();
    }
    else
    {
        buzzer_off();
    }
}
#endif

/**
 * @brief Event handler for the buzzer module board mode events
 *
 * This function plays different sequences based on the current board mode.
 */
EVENT_HANDLER(buzzer, board_mode)
{
#ifdef ENABLE_APP_INTEGRATION
    // Any mode transition invalidates held-input tracking, so a stale "held"
    // flag can never leak across a disable/enable cycle
    inhibited_button_held = false;
#endif

    switch (data->board_mode.mode)
    {
    case BOARD_MODE_IDLE:
        if (data->board_mode.submode == BOARD_SUBMODE_IDLE_SHUTTING_DOWN)
        {
            // Play the shutdown sequence on shutdown
            buzzer_play_sequence(SHUTDOWN_SEQUENCE, true);
        }
        else if (data->board_mode.previous_mode == BOARD_MODE_BOOTING)
        {
            // Play the boot sequence on boot
            buzzer_play_sequence(BOOT_SEQUENCE, false);
        }
        else
        {
            // Otherwise, silence the buzzer 
            buzzer_reset_sequence();
        }
        break;
    case BOARD_MODE_FAULT:
        if (data->board_mode.submode == BOARD_SUBMODE_FAULT_INTERNAL)
        {
            // Play the fault sequence on internal fault
            buzzer_play_sequence(FAULT_SEQUENCE, true);
        }
        else
        {
            // Otherwise, play the danger sequence on VESC fault
            buzzer_play_sequence(DANGER_SEQUENCE, true);
        }
        break;
    case BOARD_MODE_RIDING:
        switch (data->board_mode.submode)
        {
#ifdef BUZZER_ENABLE_DANGER
        case BOARD_SUBMODE_RIDING_DANGER:
            buzzer_play_sequence(DANGER_SEQUENCE, true);
            break;
#endif // BUZZER_ENABLE_DANGER
        default:
            buzzer_reset_sequence();
            break;
        }
        break;
#ifdef ENABLE_APP_INTEGRATION
    case BOARD_MODE_DISABLED:
        // Silent unless an input is already held (e.g. a foot already
        // resting on a pad when the lock command arrives)
        update_inhibited_tone();
        break;
#endif
    default:
        buzzer_reset_sequence();
        break;
    }
}

#ifdef ENABLE_APP_INTEGRATION
/**
 * @brief Event handler for inhibited input while the board is disabled (locked)
 *
 * While locked, button/footpad input is inhibited everywhere else
 * (command_processor.c ignores it entirely) - this handler is the sole
 * response: a continuous tone for as long as either input is held.
 */
EVENT_HANDLER(buzzer, inhibited_input)
{
    if (board_mode_get() != BOARD_MODE_DISABLED)
    {
        return;
    }

    if (event == EVENT_BUTTON_DOWN)
    {
        inhibited_button_held = true;
    }
    else if (event == EVENT_BUTTON_UP)
    {
        inhibited_button_held = false;
    }
    // Else: EVENT_FOOTPAD_CHANGED just needs to trigger a re-check below -
    // footpad state itself is read live in update_inhibited_tone()

    update_inhibited_tone();
}
#endif