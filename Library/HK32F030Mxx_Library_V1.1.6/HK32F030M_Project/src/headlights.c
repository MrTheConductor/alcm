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
#include "headlights.h"
#include "board_mode.h"
#include "event_queue.h"
#include "headlights_hw.h"
#include "timer.h"
#include "settings.h"
#include "function_generator.h"
#include "hysteresis.h"
#include "vesc_serial.h"
#include "lcm_types.h"

/**
 * @brief Brightness change modes
 */
typedef enum
{
    HEADLIGHTS_SET_FADE,   // Smoothly transition to the new brightness
    HEADLIGHTS_SET_INSTANT // Instantly set the new brightness
} headlights_set_type_t;

/**
 * @brief Logical headlight brightness values
 *
 * The logical brightness values are high, low and off, but the actual
 * hardware values will depend on the operating mode and user settings.
 */
typedef enum
{
    HEADLIGHTS_BRIGHTNESS_HIGH, // Full brightness (active)
    HEADLIGHTS_BRIGHTNESS_LOW,  // Low brightness (idle)
    HEADLIGHTS_BRIGHTNESS_OFF   // Off
} headlights_brightness_t;

/**
 * @brief States used for the flip direction transition
 *
 * The flip direction transition fades the headlights down to
 * zero, changes the direction, then fades them back up.
 */
typedef enum
{
    HEADLIGHTS_FLIP_NONE, // Not currently transitioning
    HEADLIGHTS_FLIP_DOWN, // Fade down
    HEADLIGHTS_FLIP_UP    // Fade back up
} headlights_flip_t;

#define HEADLIGHTS_TIMER_DELAY 20U // ms
#define SLOW_BREATH_PERIOD 4000U   // ms
#define FAST_BREATH_PERIOD 500U    // ms
#define FADE_PERIOD 1000U          // ms
#define FLIP_HALF_PERIOD 250U      // ms
#define RPM_HYSTERISIS 40.0f       // ERPMs

// Static variables
static settings_t *headlights_settings = NULL;
static function_generator_t headlights_fg;
static uint32_t headlights_timer_id = INVALID_TIMER_ID;
static hysteresis_t headlights_rpm_hys;
static headlights_flip_t headlights_flip = HEADLIGHTS_FLIP_NONE;

// Event handlers
EVENT_HANDLER(headlights, state_change);
TIMER_CALLBACK(headlights, animation);

/**
 * @brief Init the headlights variables and hardware
 */
lcm_status_t headlights_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Reset variables
    headlights_timer_id = INVALID_TIMER_ID;
    headlights_flip = HEADLIGHTS_FLIP_NONE;

    // Get the current headlights settings
    headlights_settings = settings_get();
    if (headlights_settings == NULL)
    {
        status = LCM_ERROR;
    }
    else
    {
        // Initialize the hardware
        headlights_hw_init();
        headlights_hw_enable(headlights_settings->enable_headlights);
        headlights_hw_set_direction(HEADLIGHTS_DIRECTION_NONE);

        // Setup RPM hysterisis
        if (LCM_SUCCESS != hysteresis_init(&headlights_rpm_hys, RPM_HYSTERISIS, -RPM_HYSTERISIS))
        {
            status = LCM_ERROR;
        }
        else
        {
            // Initialize state to SET
            headlights_rpm_hys.state = STATE_SET;
        }

        // Subscribe to events
        SUBSCRIBE_EVENT(headlights, EVENT_BOARD_MODE_CHANGED, state_change);
        SUBSCRIBE_EVENT(headlights, EVENT_RPM_CHANGED, state_change);
        SUBSCRIBE_EVENT(headlights, EVENT_COMMAND_TOGGLE_LIGHTS, state_change);
        SUBSCRIBE_EVENT(headlights, EVENT_COMMAND_CONTEXT_CHANGED, state_change);
        SUBSCRIBE_EVENT(headlights, EVENT_COMMAND_SETTINGS_CHANGED, state_change);
    }

    return status;
}

/**
 * @brief Starts the headlights animation timer if it's not already running.
 */
void headlights_start_timer(void)
{
    // Start the timer if it's not already running
    if (headlights_timer_id == INVALID_TIMER_ID || !is_timer_active(headlights_timer_id))
    {
        headlights_timer_id =
            set_timer(HEADLIGHTS_TIMER_DELAY, TIMER_CALLBACK_NAME(headlights, animation), true);
    }
}

/**
 * @brief Calculates the hardware-specific brightness value for the headlights.
 *
 * This function determines the appropriate hardware brightness level based on
 * the desired logical brightness (`brightness`) and the user's headlight
 * brightness setting. It takes into account whether the headlights should be
 * at full brightness, low brightness, or off.
 *
 * The hardware brightness is calculated as a fraction of the maximum hardware
 * brightness (`HEADLIGHTS_HW_MAX_BRIGHTNESS`), based on the user's
 * `headlight_brightness` setting. If low brightness is selected, the hardware
 * brightness is further reduced to approximately 25% of the full brightness.
 *
 * @param brightness The desired logical brightness level.
 *                   - HEADLIGHTS_BRIGHTNESS_HIGH: Full brightness.
 *                   - HEADLIGHTS_BRIGHTNESS_LOW:  Reduced brightness (approximately 25%).
 *                   - HEADLIGHTS_BRIGHTNESS_OFF:  Headlights off.
 *
 * @return uint16_t The calculated hardware brightness value, ranging from 0 to
 *                  `HEADLIGHTS_HW_MAX_BRIGHTNESS`. A value of 0 indicates the
 *                  headlights should be off.
 *
 * @note The function uses the `headlights_settings` global variable to access
 *       the user's brightness preference.
 * @note If `HEADLIGHTS_BRIGHTNESS_LOW` is selected, the brightness will be
 *       reduced to approximately 25% of the full brightness.
 * @note The hardware brightness is expressed as a fraction of
 *       `HEADLIGHTS_HW_MAX_BRIGHTNESS`.
 * @see HEADLIGHTS_HW_MAX_BRIGHTNESS
 * @see headlights_brightness_t
 * @see headlights_settings
 */
uint16_t headlights_calculate_hw_brightness(headlights_brightness_t brightness)
{
    // Calculate desired hardware brightness based on user setting and level
    uint16_t hw_brightness = 0U;

    if (brightness != HEADLIGHTS_BRIGHTNESS_OFF)
    {
        hw_brightness =
            (uint16_t)(headlights_settings->headlight_brightness * HEADLIGHTS_HW_MAX_BRIGHTNESS);

        if (brightness == HEADLIGHTS_BRIGHTNESS_LOW)
        {
            // Reduce brightness to approximately 25% (divide by 4) for low brightness mode.
            hw_brightness = hw_brightness >> 2;
        }
    }

    return hw_brightness;
}

/**
 * @brief Sets the brightness of the headlights, optionally with a fade transition.
 *
 * This function controls the brightness of the headlights, allowing for both
 * instant brightness changes and smooth fade transitions. The desired brightness
 * level is specified using the `brightness` parameter, which can be set to high,
 * low, or off. The type of transition (instant or fade) is determined by the
 * `type` parameter.
 *
 * The function first calculates the desired hardware brightness level
 * (`hw_brightness`) based on the user's settings and the requested brightness.
 * If low brightness is selected, the brightness is reduced to approximately 25%
 * of the full brightness.
 *
 * If an instant setting is requested, the `headlights_hw_set_brightness()` function
 * is called directly to apply the new brightness immediately.
 *
 * If a fade transition is requested, a sawtooth function generator is initialized
 * to create the fade effect. The direction of the fade (up or down) is
 * automatically determined based on the current and target brightness levels.
 * A timer is then set to trigger the `headlights_animation` callback function,
 * which will use the function generator to gradually update the brightness.
 *
 * This function will immediately return if the requested brightness is already active.
 *
 * @param brightness The desired brightness level for the headlights.
 *                   - HEADLIGHTS_BRIGHTNESS_HIGH: Full brightness.
 *                   - HEADLIGHTS_BRIGHTNESS_LOW:  Reduced brightness (approximately 25%).
 *                   - HEADLIGHTS_BRIGHTNESS_OFF:  Headlights off.
 * @param type       The type of brightness change to apply.
 *                   - HEADLIGHTS_SET_INSTANT: Instantly apply the new brightness.
 *                   - HEADLIGHTS_SET_FADE:    Smoothly transition to the new brightness.
 *
 * @note The function uses the `headlights_settings` global variable to access user preferences.
 * @note The function uses a sawtooth wave to produce the fade effect.
 * @note When `HEADLIGHTS_SET_FADE` is used, the change is driven by the `headlights_animation`
 * timer callback.
 * @note If `HEADLIGHTS_BRIGHTNESS_LOW` is selected, the brightness will be reduced to approximately
 * 25% of the full brightness.
 * @note The transition period is controlled by the `FADE_PERIOD` constant.
 * @note The timer update rate is controlled by the `HEADLIGHTS_TIMER_DELAY` constant.
 * @note If the current brightness is the same as the requested brightness, this function will do
 * nothing.
 * @see headlights_hw_set_brightness()
 * @see headlights_hw_get_brightness()
 * @see function_generator_init()
 * @see headlights_animation()
 */
void headlights_set_brigthness(headlights_brightness_t brightness, headlights_set_type_t type)
{
    uint16_t hw_brightness = headlights_calculate_hw_brightness(brightness);

    if (type == HEADLIGHTS_SET_INSTANT)
    {
        // Set the hardware direction to match RPM
        if (headlights_rpm_hys.state == STATE_SET &&
            headlights_hw_get_direction() != HEADLIGHTS_DIRECTION_FORWARD)
        {
            headlights_hw_set_direction(HEADLIGHTS_DIRECTION_FORWARD);
        }
        if (headlights_rpm_hys.state == STATE_RESET &&
            headlights_hw_get_direction() != HEADLIGHTS_DIRECTION_REVERSE)
        {
            headlights_hw_set_direction(HEADLIGHTS_DIRECTION_REVERSE);
        }
        // Instant setting
        headlights_hw_set_brightness(hw_brightness);
        if (headlights_timer_id != INVALID_TIMER_ID && is_timer_active(headlights_timer_id))
        {
            cancel_timer(headlights_timer_id);
            headlights_timer_id = INVALID_TIMER_ID;
            headlights_flip = HEADLIGHTS_FLIP_NONE;
        }
    }
    else
    {
        uint16_t current_brightness = headlights_hw_get_brightness();

        // Determine if we need to fade up or down
        if (current_brightness < hw_brightness)
        {
            // Fade up
            function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SAWTOOTH, FADE_PERIOD,
                                    HEADLIGHTS_TIMER_DELAY, current_brightness, hw_brightness,
                                    FG_FLAG_NONE, 0U);
        }
        else
        {
            // Fade down
            function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SAWTOOTH, FADE_PERIOD,
                                    HEADLIGHTS_TIMER_DELAY, hw_brightness, current_brightness,
                                    FG_FLAG_INVERT, 0U);
        }
        headlights_start_timer();
    }
}

/**
 * @brief Callback function for the headlights animation timer.
 *
 * This function is invoked repeatedly by the timer to generate and apply
 * the next sample in an animation sequence. It uses the configured
 * function generator to calculate the desired brightness value.
 *
 * When the animation is complete, the timer is stopped.
 *
 * @note This function is intended to be called by the timer module.
 * @note The function uses a function generator.
 * @note The function uses the `headlights_settings` global to determine if the lights are enabled.
 * @note This function is also responsible for stopping the timer.
 * @note If the headlights are disabled, they will be turned off when the animation is complete.
 * @note This function will also handle the `headlights_flip` sequence.
 *
 * @see function_generator_next_sample()
 * @see cancel_timer()
 * @see headlights_start_timer()
 * @see function_generator_t
 */
TIMER_CALLBACK(headlights, animation)
{
    float sample = 0.0f;

    // Get next sample from function generator
    lcm_status_t status = function_generator_next_sample(&headlights_fg, &sample);

    // Cast float to uint16_t for hardware compatibility. Truncation is expected and correct.
    headlights_hw_set_brightness((uint16_t)sample);

    // If function generator is done, cancel timer
    if (status != LCM_SUCCESS)
    {
        cancel_timer(headlights_timer_id);
        headlights_timer_id = INVALID_TIMER_ID;

        // If headlights are disabled, turn them off
        if (!headlights_settings->enable_headlights)
        {
            headlights_hw_enable(false);
        }

        // Handle flipping
        if (headlights_flip == HEADLIGHTS_FLIP_UP)
        {
            // Transition complete
            headlights_flip = HEADLIGHTS_FLIP_NONE;
        }
        else if (headlights_flip == HEADLIGHTS_FLIP_DOWN)
        {
            // Set the hardware direction to match RPM
            if (headlights_rpm_hys.state == STATE_SET)
            {
                headlights_hw_set_direction(HEADLIGHTS_DIRECTION_FORWARD);
            }
            else
            {
                headlights_hw_set_direction(HEADLIGHTS_DIRECTION_REVERSE);
            }

            // Start fade up
            function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SAWTOOTH, FLIP_HALF_PERIOD,
                                    HEADLIGHTS_TIMER_DELAY, 0U,
                                    headlights_calculate_hw_brightness(HEADLIGHTS_BRIGHTNESS_HIGH),
                                    FG_FLAG_NONE, 0U);
            headlights_flip = HEADLIGHTS_FLIP_UP;
            headlights_start_timer();
        }
    }
}

/**
 * @brief Handles changes to the VESC motor RPM (rotations per minute).
 *
 * This function is called when the VESC motor RPM changes. It applies
 * hysteresis to the RPM value to determine if the headlight direction
 * should change. If the direction needs to change, it initiates a
 * transition that fades the headlights down, changes the direction, and then fades them back up.
 *
 * This function implements a headlight flip transition to change
 * direction, as well as hysteresis to prevent rapid flipping.
 *
 * @note This function uses the `headlights_rpm_hys` hysteresis struct.
 * @note This function uses `headlights_flip` to indicate the direction of the flip transition.
 * @note This function uses `headlights_start_timer` and `function_generator_init` to start the
 * transition.
 * @note This function uses `vesc_serial_get_rpm` to get the current RPM.
 * @see apply_hysteresis()
 * @see vesc_serial_get_rpm()
 * @see function_generator_init()
 * @see headlights_start_timer()
 * @see hysteresis_t
 */
void headlights_rpm_changed(void)
{
    // ERPM is used to determine direction
    hys_state_t state = apply_hysteresis(&headlights_rpm_hys, (float)vesc_serial_get_rpm());
    headlights_direction_t direction = headlights_hw_get_direction();

    if (headlights_flip == HEADLIGHTS_FLIP_NONE &&
        ((state == STATE_SET && direction != HEADLIGHTS_DIRECTION_FORWARD) ||
         (state == STATE_RESET && direction != HEADLIGHTS_DIRECTION_REVERSE)))
    {
        // Fade down from wherever we are
        function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SAWTOOTH, FLIP_HALF_PERIOD,
                                HEADLIGHTS_TIMER_DELAY, 0U, headlights_hw_get_brightness(),
                                FG_FLAG_INVERT, 0U);
        headlights_flip = HEADLIGHTS_FLIP_DOWN;
        headlights_start_timer();
    }
    // No else needed: nothing to do
}

/**
 * @brief Event handler for the headlights module.
 *
 * This function is called when subscribed events occur and is used to
 * control the state of the headlights. It handles events related to:
 *  - Board mode changes (e.g., riding, charging, idle, fault).
 *  - RPM changes.
 *  - User commands to toggle the headlights.
 *  - User commands to change the brightness setting.
 *  - Entering and exiting the headlights brightness setting context.
 *
 * For each event, this function determines the appropriate action to take
 * and calls other functions to set the brightness, direction, or start an
 * animation.
 *
 * @param event The type of event that occurred.
 * @param data  A pointer to optional event-specific data.
 *
 * @note This function is registered to receive events via `SUBSCRIBE_EVENT`.
 * @note This function will call `headlights_set_brigthness` to change the brightness.
 * @note This function will use `function_generator_init` and `headlights_start_timer` to generate
 * animations.
 * @note This function will call `headlights_rpm_changed` when the `EVENT_RPM_CHANGED` event occurs.
 * @note This function will use the `board_mode_get` and `board_submode_get` to determine the proper
 * modes.
 * @see SUBSCRIBE_EVENT()
 * @see board_mode_get()
 * @see board_submode_get()
 * @see headlights_set_brigthness()
 * @see function_generator_init()
 * @see headlights_start_timer()
 * @see headlights_rpm_changed()
 */
EVENT_HANDLER(headlights, state_change)
{
    switch (event)
    {
    // Handle headlight conditions related to board mode
    case EVENT_BOARD_MODE_CHANGED:
        switch (board_mode_get())
        {
        case BOARD_MODE_BOOTING:
            headlights_hw_set_direction(HEADLIGHTS_DIRECTION_FORWARD);
            // Fall through intentional
        case BOARD_MODE_RIDING:
            // Set the headlights to full brightness when riding starts (i.e.,
            // the rider has stepped on the board or started moving). There's
            // a small corner case where the rider is changing direction and
            // the board mode may go to idle for a moment, so we only set the
            // brightness if the flip transition is not active.
            if (headlights_flip == HEADLIGHTS_FLIP_NONE)
            {
                // Headlights to full brightness immediately
                headlights_set_brigthness(HEADLIGHTS_BRIGHTNESS_HIGH, HEADLIGHTS_SET_INSTANT);
            }
            // No else needed: the flip transition will handle the brightness
            break;
        case BOARD_MODE_CHARGING:
            // Fall through intentional
        case BOARD_MODE_OFF:
            // Headlights off immediately
            headlights_set_brigthness(HEADLIGHTS_BRIGHTNESS_OFF, HEADLIGHTS_SET_INSTANT);
            break;
        case BOARD_MODE_FAULT:
            function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SEQUENCE, 500U,
                                    HEADLIGHTS_TIMER_DELAY, 0U, HEADLIGHTS_HW_MAX_BRIGHTNESS,
                                    FG_FLAG_REPEAT, 0xCCC0U);
            headlights_start_timer();
            break;
        case BOARD_MODE_IDLE:
            switch (board_submode_get())
            {
            case BOARD_SUBMODE_IDLE_CONFIG:
                // Fall through intentional
            case BOARD_SUBMODE_IDLE_ACTIVE:
                headlights_set_brigthness(HEADLIGHTS_BRIGHTNESS_HIGH, HEADLIGHTS_SET_INSTANT);
                break;
            case BOARD_SUBMODE_IDLE_DEFAULT:
                headlights_set_brigthness(HEADLIGHTS_BRIGHTNESS_LOW, HEADLIGHTS_SET_FADE);
                break;
            case BOARD_SUBMODE_IDLE_DOZING:
                function_generator_init(
                    &headlights_fg, FUNCTION_GENERATOR_SINE, SLOW_BREATH_PERIOD,
                    HEADLIGHTS_TIMER_DELAY, 0U,
                    headlights_calculate_hw_brightness(HEADLIGHTS_BRIGHTNESS_LOW), FG_FLAG_REPEAT,
                    0U);
                headlights_start_timer();
                break;
            case BOARD_SUBMODE_IDLE_SHUTTING_DOWN:
                function_generator_init(
                    &headlights_fg, FUNCTION_GENERATOR_SINE, FAST_BREATH_PERIOD,
                    HEADLIGHTS_TIMER_DELAY, 0U,
                    headlights_calculate_hw_brightness(HEADLIGHTS_BRIGHTNESS_HIGH), FG_FLAG_REPEAT,
                    0U);
                headlights_start_timer();
                break;
            default:
                // Nothing to do
                break;
            }
            break;
        default:
            // Nothing to do
            break;
        }
        break;
    // Handle RPM changes (direction)
    case EVENT_RPM_CHANGED:
        headlights_rpm_changed();
        break;
    // Enable/disable headlights
    case EVENT_COMMAND_TOGGLE_LIGHTS:
        if (headlights_settings->enable_headlights)
        {
            // Enable immediately
            headlights_hw_enable(headlights_settings->enable_headlights);
            headlights_set_brigthness(HEADLIGHTS_BRIGHTNESS_HIGH, HEADLIGHTS_SET_INSTANT);
        }
        else
        {
            // Fade out headlights
            function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SAWTOOTH, FADE_PERIOD / 2,
                                    HEADLIGHTS_TIMER_DELAY, 0U, headlights_hw_get_brightness(),
                                    FG_FLAG_INVERT, 0U);
            headlights_start_timer();
        }
        break;
    // Handle updating the brightness setting
    case EVENT_COMMAND_SETTINGS_CHANGED:
        if (data->context == COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS)
        {
            // Flash
            function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SQUARE, FADE_PERIOD / 2,
                                    HEADLIGHTS_TIMER_DELAY, 0U,
                                    headlights_calculate_hw_brightness(HEADLIGHTS_BRIGHTNESS_HIGH),
                                    FG_FLAG_REPEAT, 0U);
            headlights_start_timer();
        }
        break;
    // Handle entering and exiting the headlights brightness settings context
    case EVENT_COMMAND_CONTEXT_CHANGED:
        if (data->context == COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS)
        {
            // In context: flash
            function_generator_init(&headlights_fg, FUNCTION_GENERATOR_SQUARE, FADE_PERIOD / 2,
                                    HEADLIGHTS_TIMER_DELAY, 0U,
                                    headlights_calculate_hw_brightness(HEADLIGHTS_BRIGHTNESS_HIGH),
                                    FG_FLAG_REPEAT, 0U);
            headlights_start_timer();
        }
        else
        {
            // Out of context: full brightness
            headlights_set_brigthness(HEADLIGHTS_BRIGHTNESS_HIGH, HEADLIGHTS_SET_INSTANT);
        }
        break;
    default:
        // Nothing to do
        break;
    }
}