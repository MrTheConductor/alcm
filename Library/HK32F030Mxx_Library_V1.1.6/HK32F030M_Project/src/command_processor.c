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
#include "command_processor.h"
#include "event_queue.h"
#include "timer.h"
#include "settings.h"
#include "function_generator.h"
#include "footpads.h"
#include "lcm_types.h"

/**
 * @brief Enumerates the adjustment types for the command processor
 */
typedef enum
{
    COMMAND_PROCESSOR_STOP,     // Stop adjustment
    COMMAND_PROCESSOR_INCREASE, // Increase adjustment
    COMMAND_PROCESSOR_DECREASE  // Decrease adjustment
} command_processor_adjustment_t;

#define COLOR_RANGE_MS 10000U        // How long it takes to go from min to max
#define COLOR_INCREMENT_MS 20U       // How long between increments
#define BRIGHTNESS_RANGE_MS 3000U    // How long it takes to go from min to max
#define BRIGHTNESS_INCREMENT_MS 50U  // How long between increments
#define ANIMATION_INCREMENT_MS 1000U // How long to preview animation before changing

static command_processor_context_t current_context = COMMAND_PROCESSOR_CONTEXT_DEFAULT;
static timer_id_t repeat_timer_id = INVALID_TIMER_ID;
static settings_t *command_processor_settings = NULL;
static function_generator_t command_processor_fg = {0};
static animation_option_t *animation_setting = NULL;
static command_processor_adjustment_t command_processor_adjustment = COMMAND_PROCESSOR_STOP;

/*
 * Generally speaking, we don't want modules to directly observe the button
 * state. Instead, we want to have a command processor that processes button
 * input and generates events based on the current context. This allows us to
 * do multiple things with one button.
 */
EVENT_HANDLER(command_processor, button);
EVENT_HANDLER(command_processor, board_mode);
TIMER_CALLBACK(command_procesor, animation_repeat);
TIMER_CALLBACK(command_procesor, brightness_repeat);
TIMER_CALLBACK(command_procesor, color_repeat);

/**
 * @brief Initializes the command processor
 */
lcm_status_t command_processor_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Initial context
    current_context = COMMAND_PROCESSOR_CONTEXT_DEFAULT;

    // Get command processor settings
    command_processor_settings = settings_get();
    if (command_processor_settings == NULL)
    {
        status = LCM_ERROR;
    }
    else
    {
        // Subscribe to button events
        SUBSCRIBE_EVENT(command_processor, EVENT_BUTTON_CLICK, button);
        SUBSCRIBE_EVENT(command_processor, EVENT_BUTTON_HOLD, button);
        SUBSCRIBE_EVENT(command_processor, EVENT_BUTTON_UP, button);

        // Subscribe to footpads
        SUBSCRIBE_EVENT(command_processor, EVENT_FOOTPAD_CHANGED, button);

        // Subscribe to board mode events
        SUBSCRIBE_EVENT(command_processor, EVENT_BOARD_MODE_CHANGED, board_mode);
    }

    return status;
}

/**
 * @brief Sets the current context of the command processor
 *
 * @param context The new context
 */
void command_processor_set_context(command_processor_context_t context)
{
    event_data_t event_data = {0};
    event_data.context = context;
    current_context = context;
    event_queue_push(EVENT_COMMAND_CONTEXT_CHANGED, &event_data);
}

/**
 * @brief Handles one-button navigation for the command processor
 *
 * @param event The event type
 * @param count The number of clicks
 */
void command_processor_one_button_navigation(event_type_t event, uint8_t count)
{
    event_data_t event_data = {0};

    switch (event)
    {
    case EVENT_BUTTON_CLICK:
        switch (count)
        {
        case 1:
            command_processor_set_context((current_context + 1) % COMMAND_PROCESSOR_CONTEXT_COUNT);
            event_queue_push(EVENT_COMMAND_ACK, NULL);
            break;
        case 2:
            // Wrap around
            if (current_context == (command_processor_context_t)0)
            {
                command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_COUNT - 1);
            }
            else
            {
                command_processor_set_context(current_context - 1);
            }
            event_queue_push(EVENT_COMMAND_ACK, NULL);
            break;
        default:
            event_queue_push(EVENT_COMMAND_NACK, NULL);
            break;
        }
        break;
    case EVENT_BUTTON_HOLD:
        if (count == 1)
        {
            // Request config mode change
            event_data.enable = false;
            event_queue_push(EVENT_COMMAND_MODE_CONFIG, &event_data);
        }
        else
        {
            event_queue_push(EVENT_COMMAND_NACK, NULL);
        }
        break;
    default:
        break;
    }
}

/**
 * @brief Repeats the brightness adjustment using a timer
 */
TIMER_CALLBACK(command_processor, brightness_repeat)
{
    event_data_t event_data = {0};
    event_data.context = current_context;

    if (LCM_SUCCESS != function_generator_next_sample(
                           &command_processor_fg,
                           current_context == COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS
                               ? &(command_processor_settings->status_brightness)
                               : &(command_processor_settings->headlight_brightness)))
    {
        // Beep at the limit
        event_queue_push(EVENT_COMMAND_NACK, NULL);
    }
    event_queue_push(EVENT_COMMAND_SETTINGS_CHANGED, &event_data);
}

/**
 * @brief Repeats the color adjustment using a timer
 */
TIMER_CALLBACK(command_processor, color_repeat)
{
    event_data_t event_data = {0};
    event_data.context = current_context;

    if (LCM_SUCCESS != function_generator_next_sample(
                           &command_processor_fg, &(command_processor_settings->personal_color)))
    {
        // Shouldn't happen unless we forgot to enable repeating
        event_queue_push(EVENT_COMMAND_NACK, NULL);
    }
    event_queue_push(EVENT_COMMAND_SETTINGS_CHANGED, &event_data);
}

/**
 * @brief Repeats the animation adjustment using a timer
 */
TIMER_CALLBACK(command_processor, animation_repeat)
{
    event_data_t event_data = {0};
    event_data.context = current_context;

    if (animation_setting != NULL)
    {
        if (command_processor_adjustment == COMMAND_PROCESSOR_INCREASE)
        {
            *animation_setting = (*animation_setting + 1) % ANIMATION_OPTION_COUNT;
            event_queue_push(EVENT_COMMAND_ACK, NULL);
        }
        else
        {
            *animation_setting =
                *animation_setting == 0 ? ANIMATION_OPTION_COUNT - 1 : *animation_setting - 1;
            event_queue_push(EVENT_COMMAND_ACK, NULL);
        }
    }
    event_queue_push(EVENT_COMMAND_SETTINGS_CHANGED, &event_data);
}

/**
 * @brief Adjusts the current setting
 *
 * @param adjustment The adjustment type
 */
void command_processor_adjust_setting(command_processor_adjustment_t adjustment)
{
    command_processor_adjustment = adjustment;

    // Handle stop adjustment
    if (adjustment == COMMAND_PROCESSOR_STOP)
    {
        if (repeat_timer_id != INVALID_TIMER_ID && is_timer_active(repeat_timer_id))
        {
            cancel_timer(repeat_timer_id);
            repeat_timer_id = INVALID_TIMER_ID;
        }
    }
    else
    {
        // Otherwise, init state and start timer
        switch (current_context)
        {
        case COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS:
            // Fall-through intentional
        case COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS:
            // Setup generator
            function_generator_init(
                &command_processor_fg, FUNCTION_GENERATOR_SAWTOOTH, BRIGHTNESS_RANGE_MS,
                BRIGHTNESS_INCREMENT_MS, 0.0f, 1.0f,
                adjustment == COMMAND_PROCESSOR_INCREASE ? FG_FLAG_NONE : FG_FLAG_INVERT, 0);

            // Set initial value to the current setting
            function_generator_initial_sample(
                &command_processor_fg,
                current_context == COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS
                    ? command_processor_settings->status_brightness
                    : command_processor_settings->headlight_brightness);

            // Start timer
            repeat_timer_id =
                set_timer(BRIGHTNESS_INCREMENT_MS,
                          TIMER_CALLBACK_NAME(command_processor, brightness_repeat), true);
            break;
        case COMMAND_PROCESSOR_CONTEXT_PERSONAL_COLOR:
            function_generator_init(&command_processor_fg, FUNCTION_GENERATOR_SAWTOOTH,
                                    COLOR_RANGE_MS, COLOR_INCREMENT_MS, 0.0f, 360.0f,
                                    adjustment == COMMAND_PROCESSOR_INCREASE
                                        ? FG_FLAG_REPEAT
                                        : FG_FLAG_REPEAT | FG_FLAG_INVERT,
                                    0);

            // Set initial value to the current setting
            function_generator_initial_sample(&command_processor_fg,
                                              command_processor_settings->personal_color);

            // Start timer
            repeat_timer_id = set_timer(COLOR_INCREMENT_MS,
                                        TIMER_CALLBACK_NAME(command_processor, color_repeat), true);
            break;
        case COMMAND_PROCESSOR_CONTEXT_BOOT_ANIMATION:
            animation_setting = &command_processor_settings->boot_animation;
            TIMER_CALLBACK_NAME(command_processor, animation_repeat)(0U);
            repeat_timer_id =
                set_timer(ANIMATION_INCREMENT_MS,
                          TIMER_CALLBACK_NAME(command_processor, animation_repeat), true);
            break;
        case COMMAND_PROCESSOR_CONTEXT_IDLE_ANIMATION:
            animation_setting = &command_processor_settings->idle_animation;
            TIMER_CALLBACK_NAME(command_processor, animation_repeat)(0U);
            repeat_timer_id =
                set_timer(ANIMATION_INCREMENT_MS,
                          TIMER_CALLBACK_NAME(command_processor, animation_repeat), true);
            break;
        case COMMAND_PROCESSOR_CONTEXT_DOZING_ANIMATION:
            animation_setting = &command_processor_settings->dozing_animation;
            TIMER_CALLBACK_NAME(command_processor, animation_repeat)(0U);
            repeat_timer_id =
                set_timer(ANIMATION_INCREMENT_MS,
                          TIMER_CALLBACK_NAME(command_processor, animation_repeat), true);
            break;
        case COMMAND_PROCESSOR_CONTEXT_SHUTDOWN_ANIMATION:
            animation_setting = &command_processor_settings->shutdown_animation;
            TIMER_CALLBACK_NAME(command_processor, animation_repeat)(0U);
            repeat_timer_id =
                set_timer(ANIMATION_INCREMENT_MS,
                          TIMER_CALLBACK_NAME(command_processor, animation_repeat), true);
            break;
        case COMMAND_PROCESSOR_CONTEXT_RIDING_ANIMATION:
            animation_setting = &command_processor_settings->ride_animation;
            TIMER_CALLBACK_NAME(command_processor, animation_repeat)(0U);
            repeat_timer_id =
                set_timer(ANIMATION_INCREMENT_MS,
                          TIMER_CALLBACK_NAME(command_processor, animation_repeat), true);
            break;
        default:
            // Ignore anything else
            break;
        }
    }
}

/**
 * @brief Handles context-specific button events
 */
void command_processor_context_handler(event_type_t event, const event_data_t *data)
{
    event_data_t event_data = {0};
    event_data.context = current_context;

    switch (event)
    {
    case EVENT_BUTTON_UP:
        // Button release, stop adjustment
        command_processor_adjust_setting(COMMAND_PROCESSOR_STOP);
        break;
    case EVENT_BUTTON_CLICK:
        command_processor_one_button_navigation(event, data->click_count);
        break;
    case EVENT_FOOTPAD_CHANGED:
        // Footpad release, stop adjustment
        if (data->footpads_state == NONE_FOOTPAD)
        {
            command_processor_adjust_setting(COMMAND_PROCESSOR_STOP);
        }
        else if (data->footpads_state == LEFT_FOOTPAD)
        {
            // Left footpad: increase
            command_processor_adjust_setting(COMMAND_PROCESSOR_INCREASE);
        }
        else if (data->footpads_state == RIGHT_FOOTPAD)
        {
            // Right footpad: increase
            command_processor_adjust_setting(COMMAND_PROCESSOR_DECREASE);
        }
        break;
    case EVENT_BUTTON_HOLD:
        switch (data->click_count)
        {
        case 1:
            command_processor_one_button_navigation(event, data->click_count);
            break;
        case 2:
            // Click-hold: increase
            command_processor_adjust_setting(COMMAND_PROCESSOR_INCREASE);
            break;
        case 3:
            // Click-click-hold: decrease
            command_processor_adjust_setting(COMMAND_PROCESSOR_DECREASE);
            break;
        default:
            // Ignore anything else
            event_queue_push(EVENT_COMMAND_NACK, NULL);
            break;
        }
        break;
    default:
        break;
    }
}

/**
 * @brief Handles default button events
 */
void command_processor_default_handler(event_type_t event, const event_data_t *data)
{
    event_data_t event_data = {0};

    switch (event)
    {
    case EVENT_BUTTON_CLICK:
        switch (data->click_count)
        {
        case 1:
            // Click: toggle lights
            command_processor_settings->enable_headlights =
                !command_processor_settings->enable_headlights;
            command_processor_settings->enable_status_leds =
                !command_processor_settings->enable_status_leds;
            event_queue_push(EVENT_COMMAND_TOGGLE_LIGHTS, NULL);
            event_queue_push(EVENT_COMMAND_ACK, NULL);
            break;
        case 2:
            // Click-click: toggle beeper
            command_processor_settings->enable_beep = !command_processor_settings->enable_beep;
            event_queue_push(EVENT_COMMAND_TOGGLE_BEEPER, NULL);
            event_queue_push(EVENT_COMMAND_ACK, NULL);
            break;
        default:
            event_queue_push(EVENT_COMMAND_NACK, NULL);
            break;
        }
        break;
    case EVENT_BUTTON_HOLD:
        switch (data->click_count)
        {
        case 1:
            // Hold: shutdown
            event_queue_push(EVENT_COMMAND_SHUTDOWN, NULL);
            break;
        case 2: // Click-hold: request config mode change from board_mode
            event_data.enable = true;
            event_queue_push(EVENT_COMMAND_MODE_CONFIG, &event_data);
            break;
        default:
            event_queue_push(EVENT_COMMAND_NACK, NULL);
            break;
        }
        break;
    default:
        break;
    }
}

/**
 * @brief Handles button events
 */
EVENT_HANDLER(command_processor, button)
{
    // Ignore events if the board is booting or in a fault state
    //
    // TBD: Maybe this should be an inhibited context instead?
    board_mode_t mode = board_mode_get();
    if (mode == BOARD_MODE_BOOTING ||
        mode == BOARD_MODE_FAULT ||
        mode == BOARD_MODE_OFF)
    {
        return;
    }
    else if (current_context == COMMAND_PROCESSOR_CONTEXT_DEFAULT)
    {
        command_processor_default_handler(event, data);
    }
    else
    {
        command_processor_context_handler(event, data);
    }
}

/**
 * @brief Handles board mode events
 */
EVENT_HANDLER(command_processor, board_mode)
{
    if (current_context != COMMAND_PROCESSOR_CONTEXT_DEFAULT && event == EVENT_BOARD_MODE_CHANGED &&
        (data->board_mode.mode != BOARD_MODE_IDLE ||
         data->board_mode.submode != BOARD_SUBMODE_IDLE_CONFIG))
    {
        event_queue_push(EVENT_COMMAND_NACK, NULL);
        command_processor_adjust_setting(COMMAND_PROCESSOR_STOP);
        command_processor_set_context(COMMAND_PROCESSOR_CONTEXT_DEFAULT);
    }
    else if (current_context == COMMAND_PROCESSOR_CONTEXT_DEFAULT &&
             event == EVENT_BOARD_MODE_CHANGED && data->board_mode.mode == BOARD_MODE_IDLE &&
             data->board_mode.submode == BOARD_SUBMODE_IDLE_CONFIG)
    {
        event_queue_push(EVENT_COMMAND_ACK, NULL);
        command_processor_set_context((command_processor_context_t)0); // The first context
    }
    // No else needed, ignore any other events
}