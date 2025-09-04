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
#include "function_generator.h"
#include "settings.h"
#include "timer.h"

#define TICK_INTERVAL_MS 10
#define SEQUENCE_PERIOD_MS 320

// Buzzer sequences
#define ACK_SEQUENCE 0xC000
#define NACK_SEQUENCE 0xCC00
#define SHUTDOWN_SEQUENCE 0xC000
#define WARNING_SEQUENCE 0xF000
#define DANGER_SEQUENCE 0xF0F0
#define FAULT_SEQUENCE 0xAAAF
#define BOOT_SEQUENCE 0xF300

// Static variables
static settings_t *buzzer_settings = NULL;
static function_generator_t fg = {0};
static timer_id_t buzzer_timer_id = INVALID_TIMER_ID;

// Forward declarations
// Event handlers
EVENT_HANDLER(buzzer, command);
EVENT_HANDLER(buzzer, board_mode);

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
    float sample = 0.0f;

    if (function_generator_next_sample(&fg, &sample) != LCM_SUCCESS)
    {
        buzzer_reset_sequence();
    }
    else
    {
        if (sample <= 0.0f)
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
                            0.0f, 1.0f, repeat ? FG_FLAG_REPEAT : 0, sequence);

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

/**
 * @brief Event handler for the buzzer module board mode events
 *
 * This function plays different sequences based on the current board mode.
 */
EVENT_HANDLER(buzzer, board_mode)
{
    switch (data->board_mode.mode)
    {
    case BOARD_MODE_IDLE:
        if (data->board_mode.submode == BOARD_SUBMODE_IDLE_SHUTTING_DOWN)
        {
            // Play the shutdown sequence on shutdown
            buzzer_play_sequence(SHUTDOWN_SEQUENCE, true);
        }
        else if (data->board_mode.previous_submode == BOARD_SUBMODE_IDLE_SHUTTING_DOWN)
        {
            // Stop the shutdown sequence if it was playing
            buzzer_reset_sequence();
        }
        else if (data->board_mode.previous_mode == BOARD_MODE_BOOTING)
        {
            // Play the boot sequence on boot
            buzzer_play_sequence(BOOT_SEQUENCE, false);
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
#ifdef BUZZER_ENABLE_WARNING
        case BOARD_SUBMODE_RIDING_WARNING:
            buzzer_play_sequence(WARNING_SEQUENCE, true);
            break;
#endif // BUZZER_ENABLE_WARNING
        default:
            buzzer_reset_sequence();
            break;
        }
        break;
    default:
        buzzer_reset_sequence();
        break;
    }
}