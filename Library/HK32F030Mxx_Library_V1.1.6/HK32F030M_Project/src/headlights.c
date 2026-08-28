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
#include "config.h"
#include "tiny_math.h"

#define HEADLIGHTS_TIMER_DELAY 20U // How frequent to update the headlights in ms

// Mode animations control the mode control factor
// and are generally tied to the board mode (i.e., idle, dozing, etc.)
typedef enum
{
    HEADLIGHTS_MODE_ANIMATION_NONE,
    HEADLIGHTS_MODE_ANIMATION_IDLE_FADE,
#ifdef HEADLIGHTS_ENABLE_DOZING
    HEADLIGHTS_MODE_ANIMATION_IDLE_DOZING,
#endif
#ifdef HEADLIGHTS_ENABLE_SHUTTING_DOWN
    HEADLIGHTS_MODE_ANIMATION_IDLE_SHUTTING_DOWN,
#endif
    HEADLIGHTS_MODE_ANIMATION_FLASH,
} headlights_mode_animation_t;

// Enable animations control the enable control factor
// and are generally tied to the enable state of the headlights
typedef enum
{
    HEADLIGHTS_ENABLE_ANIMATION_NONE,
    HEADLIGHTS_ENABLE_ANIMATION_FADE_OUT,
} headlights_enable_animation_t;

// Static variables
static settings_t *headlights_settings = NULL;
static function_generator_t headlights_mode_fg;
static function_generator_t headlights_enable_fg;
static function_generator_t headlights_direction_fg;
static uint32_t headlights_mode_animation_timer_id = INVALID_TIMER_ID;
static uint32_t headlights_enable_animation_timer_id = INVALID_TIMER_ID;
static uint32_t headlights_direction_animation_timer_id = INVALID_TIMER_ID;
static hysteresis_t headlights_rpm_hys;

// Control factors - scale8()-domain fractions, 0-255 = 0.0-1.0
uint8_t enable_control = 255U; // Enable control factor
#ifdef ENABLE_IMU_EVENTS
uint8_t pitch_control = 255U; // Pitch control factor
#endif
uint8_t mode_control = 255U;  // Mode control factor
uint8_t direction_control = 255U; // Direction control factor

// Event handlers
EVENT_HANDLER(headlights, state_change);
TIMER_CALLBACK(headlights, mode_animation);
TIMER_CALLBACK(headlights, enable_animation);
TIMER_CALLBACK(headlights, direction_animation);

/**
 * @brief Init the headlights variables and hardware
 */
lcm_status_t headlights_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Reset variables
    headlights_mode_animation_timer_id = INVALID_TIMER_ID;
    headlights_enable_animation_timer_id = INVALID_TIMER_ID;
    headlights_direction_animation_timer_id = INVALID_TIMER_ID;

    // Get the current headlights settings
    headlights_settings = settings_get();
    if (headlights_settings == NULL)
    {
        status = LCM_ERROR;
    }
    else
    {
        // Initialize enable control based on settings
        if (headlights_settings->enable_headlights)
        {
            enable_control = 255U;
        }
        else
        {
            enable_control = 0U;
        }

        // Initialize the hardware
        headlights_hw_init();
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
#ifdef ENABLE_IMU_EVENTS
        SUBSCRIBE_EVENT(headlights, EVENT_IMU_PITCH_CHANGED, state_change);
#endif
    }

    return status;
}

void headlights_set_hw_brightness()
{
    uint16_t hw_brightness = 0U;
    uint8_t combined = scale8(headlights_settings->headlight_brightness, enable_control);
    combined = scale8(combined, pitch_control);
    combined = scale8(combined, mode_control);
    combined = scale8(combined, direction_control);

    // Divisor is a compile-time constant (255), so this is a free
    // multiply-by-reciprocal, not a runtime division call.
    hw_brightness = (uint16_t)(((uint32_t)combined * HEADLIGHTS_HW_MAX_BRIGHTNESS) / 255U);

    headlights_hw_set_brightness(hw_brightness);
}

TIMER_CALLBACK(headlights, mode_animation)
{
    // Get next sample from function generator
    fixed16_t sample = 0;
    lcm_status_t status = function_generator_next_sample(&headlights_mode_fg, &sample);
    mode_control = fixed16_to_frac8(sample);
    headlights_set_hw_brightness();

    // If function generator is done, cancel timer
    if (status != LCM_SUCCESS)
    {
        cancel_timer(headlights_mode_animation_timer_id);
        headlights_mode_animation_timer_id = INVALID_TIMER_ID;
    }
}

TIMER_CALLBACK(headlights, enable_animation)
{
    // Get next sample from function generator
    fixed16_t sample = 0;
    lcm_status_t status = function_generator_next_sample(&headlights_enable_fg, &sample);
    enable_control = fixed16_to_frac8(sample);
    headlights_set_hw_brightness();

    // If function generator is done, cancel timer
    if (status != LCM_SUCCESS)
    {
        cancel_timer(headlights_enable_animation_timer_id);
        headlights_enable_animation_timer_id = INVALID_TIMER_ID;
    }
}

TIMER_CALLBACK(headlights, direction_animation)
{
    // Get next sample from function generator
    fixed16_t sample = 0;
    lcm_status_t status = function_generator_next_sample(&headlights_direction_fg, &sample);
    direction_control = fixed16_to_frac8(sample);
    headlights_set_hw_brightness();

    // If function generator is done, cancel timer
    if (status != LCM_SUCCESS)
    {
        cancel_timer(headlights_direction_animation_timer_id);
        headlights_direction_animation_timer_id = INVALID_TIMER_ID;

        // If the direction_control is ~0, we can switch directions and start fading back up
        if (direction_control <= 26U) // ~0.1 * 255
        {
            if (headlights_rpm_hys.state == STATE_SET)
            {
                // Set the direction to forward
                headlights_hw_set_direction(HEADLIGHTS_DIRECTION_FORWARD);
            }
            else if (headlights_rpm_hys.state == STATE_RESET)
            {
                // Set the direction to reverse
                headlights_hw_set_direction(HEADLIGHTS_DIRECTION_REVERSE);
            }

            // Reinitialize the function generator to fade back up
            function_generator_init(&headlights_direction_fg,
                                    FUNCTION_GENERATOR_SAWTOOTH,
                                    FADE_PERIOD/2,
                                    HEADLIGHTS_TIMER_DELAY,
                                    FIXED16(0.0),
                                    FIXED16(1.0),
                                    FG_FLAG_NONE,
                                    0U);
            headlights_direction_animation_timer_id =
                set_timer(HEADLIGHTS_TIMER_DELAY, TIMER_CALLBACK_NAME(headlights, direction_animation), true);
        }
    }
}

/**
 * @brief Set the headlights animation
 *
 * This function sets the headlights animation based on the provided animation type.
 * It initializes the function generator with the appropriate parameters and starts
 * a timer to update the headlights brightness.
 *
 * @param animation The type of animation to set.
 */
void headlights_set_mode_animation(headlights_mode_animation_t animation)
{
    switch(animation)
    {
    case HEADLIGHTS_MODE_ANIMATION_IDLE_FADE:
        // Initialize the function generator for idle fade animation
        function_generator_init(&headlights_mode_fg,
                                FUNCTION_GENERATOR_SAWTOOTH,
                                FADE_PERIOD,
                                HEADLIGHTS_TIMER_DELAY,
                                FIXED16(HEADLIGHTS_IDLE_BRIGHTNESS),
                                FIXED16(1.0),
                                FG_FLAG_INVERT,
                                0U);
        break;
#ifdef HEADLIGHTS_ENABLE_DOZING
    case HEADLIGHTS_MODE_ANIMATION_IDLE_DOZING:
        // Initialize the function generator for idle dozing animation
        function_generator_init(&headlights_mode_fg,
                                FUNCTION_GENERATOR_SINE,
                                SLOW_BREATH_PERIOD,
                                HEADLIGHTS_TIMER_DELAY,
                                FIXED16(0.05),
                                FIXED16(HEADLIGHTS_IDLE_BRIGHTNESS),
                                FG_FLAG_REPEAT,
                                0U);
        break;
#endif
#ifdef HEADLIGHTS_ENABLE_SHUTTING_DOWN
    case HEADLIGHTS_MODE_ANIMATION_IDLE_SHUTTING_DOWN:
        // Initialize the function generator for idle shutting down animation
        function_generator_init(&headlights_mode_fg,
                                FUNCTION_GENERATOR_SINE,
                                FAST_BREATH_PERIOD,
                                HEADLIGHTS_TIMER_DELAY,
                                FIXED16(0.0),
                                FIXED16(1.0),
                                FG_FLAG_REPEAT,
                                0U);
        break;
#endif
    case HEADLIGHTS_MODE_ANIMATION_FLASH:
        // Initialize the function generator for flash animation
        function_generator_init(&headlights_mode_fg,
                                FUNCTION_GENERATOR_SQUARE,
                                FADE_PERIOD,
                                HEADLIGHTS_TIMER_DELAY,
                                FIXED16(0.0),
                                FIXED16(1.0),
                                FG_FLAG_REPEAT,
                                0U);
        break;
    default:
        break;
    }

    if (animation == HEADLIGHTS_MODE_ANIMATION_NONE)
    {
        // Stop the animation
        if (headlights_mode_animation_timer_id != INVALID_TIMER_ID)
        {
            cancel_timer(headlights_mode_animation_timer_id);
            headlights_mode_animation_timer_id = INVALID_TIMER_ID;
            headlights_set_hw_brightness();
        }
    }
    else
    {
        headlights_mode_animation_timer_id =
            set_timer(HEADLIGHTS_TIMER_DELAY, TIMER_CALLBACK_NAME(headlights, mode_animation), true);
    }
}

void headlights_set_enable_animation(headlights_enable_animation_t animation)
{
    if (animation == HEADLIGHTS_ENABLE_ANIMATION_FADE_OUT)
    {
        // Initialize the function generator for fade out animation
        function_generator_init(&headlights_enable_fg,
                                FUNCTION_GENERATOR_SAWTOOTH,
                                FADE_PERIOD,
                                HEADLIGHTS_TIMER_DELAY,
                                FIXED16(0.0),
                                FIXED16(1.0),
                                FG_FLAG_INVERT,
                                0U);

        headlights_enable_animation_timer_id =
            set_timer(HEADLIGHTS_TIMER_DELAY, TIMER_CALLBACK_NAME(headlights, enable_animation), true);
    } else {
        // Stop the animation
        if (headlights_enable_animation_timer_id != INVALID_TIMER_ID)
        {
            cancel_timer(headlights_enable_animation_timer_id);
            headlights_enable_animation_timer_id = INVALID_TIMER_ID;
        }
    }
}    

void headlights_rpm_changed(void)
{
    // ERPM is used to determine direction
    hys_state_t state = apply_hysteresis(&headlights_rpm_hys, vesc_serial_get_rpm());
    headlights_direction_t direction = headlights_hw_get_direction();
    if ((state == STATE_SET && direction != HEADLIGHTS_DIRECTION_FORWARD) ||
        (state == STATE_RESET && direction != HEADLIGHTS_DIRECTION_REVERSE))
    {
        // Change direction
        function_generator_init(&headlights_direction_fg,
                                FUNCTION_GENERATOR_SAWTOOTH,
                                FADE_PERIOD/2,
                                HEADLIGHTS_TIMER_DELAY,
                                FIXED16(0.0),
                                FIXED16(1.0),
                                FG_FLAG_INVERT,
                                0U);
        function_generator_initial_sample(&headlights_direction_fg, frac8_to_fixed16(direction_control));
        headlights_direction_animation_timer_id =
            set_timer(HEADLIGHTS_TIMER_DELAY, TIMER_CALLBACK_NAME(headlights, direction_animation), true);
    }
}

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
            mode_control = 255U;
            headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_NONE);
            break;
        case BOARD_MODE_DISABLED:
            // Fall through intentional
        case BOARD_MODE_CHARGING:
            // Fall through intentional
        case BOARD_MODE_OFF:
            mode_control = 0U;
            headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_NONE);
            break;
        case BOARD_MODE_FAULT:
            // If in internal fault, flash the headlights
            if (board_submode_get() == BOARD_SUBMODE_FAULT_INTERNAL) {
                headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_FLASH);
            }
            else
            {
                // Otherwise, stop any animations
                headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_NONE);
            }
            break;
        case BOARD_MODE_IDLE:
            switch (board_submode_get())
            {
            case BOARD_SUBMODE_IDLE_CONFIG:
                // Fall through intentional
            case BOARD_SUBMODE_IDLE_ACTIVE:
                mode_control = 255U;
                headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_NONE);
                break;
            case BOARD_SUBMODE_IDLE_DEFAULT:
                headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_IDLE_FADE);
                break;
#ifdef HEADLIGHTS_ENABLE_DOZING
            case BOARD_SUBMODE_IDLE_DOZING:
                headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_IDLE_DOZING);
                break;
#endif
#ifdef HEADLIGHTS_ENABLE_SHUTTING_DOWN
            case BOARD_SUBMODE_IDLE_SHUTTING_DOWN:
                headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_IDLE_SHUTTING_DOWN);
                break;
#endif
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
            enable_control = 255U;
            headlights_set_enable_animation(HEADLIGHTS_ENABLE_ANIMATION_NONE);
        }
        else
        {
            headlights_set_enable_animation(HEADLIGHTS_ENABLE_ANIMATION_FADE_OUT);
        }
        break;
    // Handle entering and exiting the headlights brightness settings context
    case EVENT_COMMAND_CONTEXT_CHANGED:
        if (data->context == COMMAND_PROCESSOR_CONTEXT_HEADLIGHT_BRIGHTNESS)
        {
            headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_FLASH);
        }
        else
        {
            headlights_set_mode_animation(HEADLIGHTS_MODE_ANIMATION_NONE);
        }
        break;
#ifdef ENABLE_IMU_EVENTS
    case EVENT_IMU_PITCH_CHANGED:
        // Update the pitch control factor based on the IMU pitch.
        // imu_pitch is in millidegrees (see event_queue.h), so 60 degrees
        // is 60000 here, not 60.
        {
            if (data->imu_pitch >= 60000 || data->imu_pitch <= -60000) {
                pitch_control = 0U;
            } else {
                pitch_control = 255U;
            }
        }
        break;
#endif
    default:
        // Nothing to do
        break;
    }

    // Update the brightness setting
    headlights_set_hw_brightness();
}