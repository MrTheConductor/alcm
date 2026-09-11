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
#include <stdint.h>
#include <stddef.h>

#include "event_queue.h"
#include "status_leds.h"
#include "footpads.h"
#include "vesc_serial.h"
#include "animations.h"
#include "settings.h"
#include "tiny_math.h"
#include "board_mode.h"
#include "config.h"

/**
 * @brief Structure to represent a color palette
 */
typedef struct
{
    const status_leds_color_t black;
    const status_leds_color_t white;
    const status_leds_color_t red;
    const status_leds_color_t orange;
    const status_leds_color_t green;
    const status_leds_color_t blue;
    const status_leds_color_t magenta;
    const status_leds_color_t light_blue;
} color_palette_t;

// Color palette
// Note: format is {G, R, B}
static const color_palette_t colors = {.black = {0x00, 0x00, 0x00},
                                       .white = {0xff, 0xff, 0xff},
                                       .red = {0x00, 0xff, 0x00},
                                       .orange = {0x7f, 0xff, 0x00},
                                       .green = {0xff, 0x00, 0x00},
                                       .blue = {0x00, 0x00, 0xff},
                                       .magenta = {0x00, 0xff, 0xff},
                                       .light_blue = {0x77, 0x00, 0xb6}};

// Status LED buffer
static status_leds_color_t status_leds_buffer[STATUS_LEDS_COUNT] = {0};
static settings_t *status_leds_settings = NULL;
static status_leds_color_t custom_color;
static uint16_t battery_animation_id = 0U;
static uint16_t ride_animation_id = 0U;

// Forward declarations
EVENT_HANDLER(status_leds, state_changed);
EVENT_HANDLER(status_leds, command);
void status_leds_turn_off(void);
void update_display(event_type_t event);

/**
 * @brief Initializes the status LEDs module.
 *
 * This function initializes the status LEDs hardware, and subscribes to the
 * necessary events to update the status LEDs.
 */
lcm_status_t status_leds_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Get settings
    status_leds_settings = settings_get();

    if (status_leds_settings == NULL)
    {
        // Settings not found, return error
        status = LCM_ERROR_NULL_POINTER;
    }
    else
    {
        // Initialize the hardware
        status_leds_hw_init(status_leds_buffer);

        // Configure brightness
        status_leds_hw_set_brightness(status_leds_settings->status_brightness);

        // Force LEDs off
        status_leds_turn_off();
        status_leds_hw_enable(status_leds_settings->enable_status_leds);

        // Load custom color
        hsl_to_rgb(degrees_to_fixed16(status_leds_settings->personal_color), SATURATION_DEFAULT,
                   LIGHTNESS_DEFAULT, &custom_color);

        // Subscribe to events that trigger status changes
        //
        // Note: We don't subscribe to RPM here because it's handled by the
        // board state machine. Duty cycle is subscribed directly (rather
        // than only reacting to the board-mode-driven danger submode) so the
        // duty cycle gauge can update live as the value changes, not just on
        // submode transitions.
        SUBSCRIBE_EVENT(status_leds, EVENT_BOARD_MODE_CHANGED, state_changed);
        SUBSCRIBE_EVENT(status_leds, EVENT_FOOTPAD_CHANGED, state_changed);
        SUBSCRIBE_EVENT(status_leds, EVENT_BATTERY_LEVEL_CHANGED, state_changed);
        SUBSCRIBE_EVENT(status_leds, EVENT_DUTY_CYCLE_CHANGED, state_changed);
        SUBSCRIBE_EVENT(status_leds, EVENT_COMMAND_TOGGLE_LIGHTS, command);
        SUBSCRIBE_EVENT(status_leds, EVENT_COMMAND_TOGGLE_BEEPER, command);
        SUBSCRIBE_EVENT(status_leds, EVENT_COMMAND_CONTEXT_CHANGED, command);
        SUBSCRIBE_EVENT(status_leds, EVENT_COMMAND_SETTINGS_CHANGED, command);
    }

    return status;
}

uint16_t status_leds_start_animation_option(animation_option_t option)
{
    uint16_t animation_id = 0U;
    uint8_t first_led = 0U;
    uint8_t last_led = STATUS_LEDS_COUNT - 1U;

#ifdef ENABLE_IMU_EVENTS
        if (vesc_serial_get_imu_roll() < 0)
        {
            first_led = STATUS_LEDS_COUNT - 1U;
            last_led = 0U;
        }
#endif
    switch (option)
    {
    case ANIMATION_OPTION_RAINBOW_SCAN:
        animation_id =
            scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_SINE, COLOR_MODE_HSV_DECREASE,
                                 STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
                                 FIXED16(0.0),    // hue min
                                 FIXED16(360.0),  // hue max
                                 3000, // color change speed
                                 SCAN_START_DEFAULT, SCAN_END_NEVER, FIXED16(0.0),
                                 NULL // RGB color (ignored)
            );
        break;
#ifdef ENABLE_KNIGHT_RIDER_ANIMATION
    case ANIMATION_OPTION_KNIGHT_RIDER:
        animation_id = scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_SINE, COLOR_MODE_RGB,
                                            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
                                            FIXED16(0.0), // (not-used)
                                            FIXED16(0.0), // (not-used)
                                            0, // (not-used)
                                            SCAN_START_DEFAULT, SCAN_END_NEVER, FIXED16(0.0),
                                            &colors.red // RGB color
        );
        break;
#endif
    case ANIMATION_OPTION_RAINBOW_MIRROR:
        animation_id = fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_INCREASE,
                                            BRIGHTNESS_MODE_STATIC, FILL_MODE_HSV_GRADIENT_MIRROR,
                                            0U, STATUS_LEDS_COUNT - 1U,
                                            FIXED16(0.0),    // hue min
                                            FIXED16(360.0),  // hue max
                                            1500, // color change speed
                                            FIXED16(0.0),    // brightness min
                                            FIXED16(1.0),    // brightness max
                                            0,    // brightness change speed
                                            0U,
                                            NULL // RGB color (ignored)
        );
        break;
#ifdef ENABLE_EXPANDING_PULSE_ANIMATION
    case ANIMATION_OPTION_EXPANDING_PULSE:
        animation_id = scan_animation_setup(
            status_leds_buffer, SCAN_DIRECTION_LEFT_TO_RIGHT_MIRROR, COLOR_MODE_HSV_SINE,
            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
            degrees_to_fixed16(status_leds_settings->personal_color), // hue min
            degrees_to_fixed16(
                (uint16_t)CLAMP((int32_t)status_leds_settings->personal_color + 15, 0, 360)), // hue max
            3000, SCAN_START_DEFAULT, SCAN_END_NEVER, FIXED16(0.0), NULL);
        break;
#endif
#ifdef ENABLE_THE_FUZZ_ANIMATION
    case ANIMATION_OPTION_THE_FUZZ:
        animation_id = fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_SQUARE,
                                            BRIGHTNESS_MODE_SEQUENCE, FILL_MODE_SOLID, 0U,
                                            STATUS_LEDS_COUNT - 1U,
                                            FIXED16(0.0),    // hue min
                                            FIXED16(240.0),  // hue max
                                            1000, // color change speed
                                            FIXED16(0.0),    // brightness min
                                            FIXED16(1.0),    // brightness max
                                            500,  // brightness speed
                                            0xAA00,  // bright sequence
                                            NULL     // RGB color (ignored)
        );
        break;
#endif
    case ANIMATION_OPTION_120_SCROLL:
        animation_id = fill_animation_setup(
            status_leds_buffer, COLOR_MODE_HSV_INCREASE, BRIGHTNESS_MODE_STATIC,
            FILL_MODE_HSV_GRADIENT, first_led, last_led,
            degrees_to_fixed16(status_leds_settings->personal_color), // hue min
            degrees_to_fixed16(
                (uint16_t)CLAMP((int32_t)status_leds_settings->personal_color + 120, 0, 360)), // hue max
            2000, // color change speed
            FIXED16(0.0),    // brightness min
            FIXED16(1.0),    // brightness max
            0,    // brightness change speed
            0U,
            NULL // RGB color (ignored)
        );
        break;
#ifdef ENABLE_IMPLODING_PULSE_ANIMATION
    case ANIMATION_OPTION_IMPLODING_PULSE:
        animation_id = scan_animation_setup(
            status_leds_buffer, SCAN_DIRECTION_RIGHT_TO_LEFT_MIRROR, COLOR_MODE_HSV_SINE,
            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
            degrees_to_fixed16(status_leds_settings->personal_color), // hue min
            degrees_to_fixed16(
                (uint16_t)CLAMP((int32_t)status_leds_settings->personal_color + 15, 0, 360)), // hue max
            3000, SCAN_START_DEFAULT, SCAN_END_NEVER, FIXED16(0.0), NULL);
        break;
#endif
    case ANIMATION_OPTION_RAINBOW_BAR:
        animation_id = fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_INCREASE,
                                            BRIGHTNESS_MODE_STATIC, FILL_MODE_HSV_GRADIENT, first_led, last_led,
                                            FIXED16(0.0),    // hue min
                                            FIXED16(360.0),  // hue max
                                            1000, // color change speed
                                            FIXED16(0.0),    // brightness min
                                            FIXED16(1.0),    // brightness max
                                            0,    // brightness change speed
                                            0U,
                                            NULL // RGB color (ignored)
        );
        break;
    case ANIMATION_OPTION_COMPLEMENTARY_WAVE: {
        uint16_t opposite = (status_leds_settings->personal_color + 180U) % 360U;
        uint16_t hue_min_deg = MIN(status_leds_settings->personal_color, opposite);
        uint16_t hue_max_deg = MAX(status_leds_settings->personal_color, opposite);
        animation_id = fill_animation_setup(
            status_leds_buffer, COLOR_MODE_HSV_SQUARE, BRIGHTNESS_MODE_STATIC,
            FILL_MODE_HSV_GRADIENT_MIRROR, 0U, STATUS_LEDS_COUNT - 1U,
            degrees_to_fixed16(hue_min_deg), degrees_to_fixed16(hue_max_deg),
            2000, // color change speed
            FIXED16(1.0),    // brightness min
            FIXED16(1.0),    // brightness max
            0,    // brightness change speed
            0U,
            NULL // RGB color
        );
        break;
    }
    case ANIMATION_OPTION_PERSONAL_SCAN:
        animation_id = scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_SINE, COLOR_MODE_RGB,
                                            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
                                            FIXED16(0.0), // (not-used)
                                            FIXED16(0.0), // (not-used)
                                            0, // (not-used)
                                            SCAN_START_DEFAULT, SCAN_END_NEVER, FIXED16(0.0),
                                            &custom_color // RGB color
        );
        break;
#ifdef ENABLE_FIRE_ANIMATION
    case ANIMATION_OPTION_FIRE:
        animation_id = fire_animation_setup(status_leds_buffer);
        break;
#endif
    case ANIMATION_OPTION_FLOATWHEEL_CLASSIC:
        animation_id = scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_LEFT_TO_RIGHT_FILL,
                                            COLOR_MODE_RGB,
                                            5500, // scan speed in milliseconds
                                            FIXED16(0.0), // (not-used)
                                            FIXED16(0.0), // (not-used)
                                            0, // (not-used)
                                            SCAN_START_DEFAULT, SCAN_END_MAX_MU, FIXED16(0.0),
                                            &custom_color // RGB color
        );
        break;
    case ANIMATION_OPTION_NONE:
        // Fade out the lights and then disable
        animation_id =
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT, NULL);
        break;
    case ANIMATION_OPTION_COUNT:
        // Fall through intentional
    default:
        fault(EMERGENCY_FAULT_INVALID_STATE);
        break;
    }

    return animation_id;
}

/**
 * @brief Fills the status LEDs as a proportional bar for a tenths-of-a-percent
 * value, in the given color.
 *
 * This function uses the first 10 status LEDs as a bar graph. The LEDs are
 * divided into 10 equal parts, with each part representing 10% of the value's
 * range. Shared by the battery gauge and the duty cycle gauge.
 *
 * @param level_tenths The value to display, tenths of a percent
 *                      (0-1000 = 0.0%-100.0%)
 * @param color The color to fill the bar with
 */
static void display_gauge_bar(int16_t level_tenths, const status_leds_color_t *color)
{
    // Map tenths-of-a-percent [0,1000] to an LED index [-1,9]: same as
    // (level_tenths/10.0f/10.0f) - 1.0f in the original float formula, done
    // as one Q16.16 scale (divisor 100 is a compile-time constant, so this
    // is a free multiply-by-reciprocal).
    fixed16_t init_mu = (fixed16_t)(((int32_t)level_tenths * 65536) / 100) - FIXED16(1.0);
    stop_animation();

    scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_LEFT_TO_RIGHT_FILL, COLOR_MODE_RGB,
                         500, // scan speed in milliseconds
                         FIXED16(0.0), // (not-used)
                         FIXED16(0.0), // (not-used)
                         0, // (not-used)
                         SCAN_START_MU, SCAN_END_SINGLE_TICK,
                         init_mu,
                         color // RGB color
    );
}

/**
 * @brief Displays the current battery level on the status LEDs
 *
 * @param battery_level The current battery level, tenths of a percent
 *                       (0-1000 = 0.0%-100.0%)
 */
void display_battery(int16_t battery_level)
{
    if (battery_level <= CRITICAL_BATTERY_THRESHOLD)
    {
        // Check if we need to start a new animation
        if (get_animation_id() != battery_animation_id)
        {
            // Stop any existing animation
            stop_animation();

            // Start a red flash animation
            battery_animation_id = fill_animation_setup(status_leds_buffer, COLOR_MODE_RGB,
                                                        BRIGHTNESS_MODE_SINE, FILL_MODE_SOLID,
                                                        0U,     // fisrt LED to animate
                                                        0U,     // last LED to animate
                                                        FIXED16(0.0),   // hue min (ignored)
                                                        FIXED16(0.0),   // hue max (ignored)
                                                        0,   // color change speed (ignored)
                                                        FIXED16(0.1),   // brightness min
                                                        FIXED16(1.0),   // brightness max
                                                        500, // brightness change speed
                                                        0U,
                                                        &colors.red // RGB color
            );
        }
    }
    else
    {
        const status_leds_color_t *color = &colors.white;

        if (battery_level <= LOW_BATTERY_THRESHOLD)
        {
            color = &colors.orange;
        }

        display_gauge_bar(battery_level, color);
    }
}

/**
 * @brief Displays the current duty cycle on the status LEDs as a proportional
 * bar, matching the battery gauge's 1-LED-per-10% scale.
 *
 * Below #DUTY_CYCLE_DANGER_THRESHOLD the bar is green; at or above it, the
 * bar turns red to match the buzzer's danger alarm.
 *
 * @param duty_cycle The current duty cycle, tenths of a percent
 *                    (0-1000 = 0.0%-100.0%)
 */
void display_duty_cycle(int16_t duty_cycle)
{
    const status_leds_color_t *color =
        (duty_cycle >= DUTY_CYCLE_DANGER_THRESHOLD) ? &colors.red : &colors.green;

    display_gauge_bar(duty_cycle, color);
}

/**
 * @brief Displays the footpad state on the status LEDs.
 *
 * This function updates the status LEDs to reflect the current state of the
 * footpads. If the left footpad is pressed, the left half of the LEDs will
 * be illuminated in light blue. If the right footpad is pressed, the right
 * half will be illuminated. If both footpads are pressed, all LEDs will
 * be illuminated. Any current animations are stopped before updating the
 * display.
 *
 * @param footpad The current state of the footpads, specified as a bitwise OR
 *                of #LEFT_FOOTPAD and #RIGHT_FOOTPAD.
 */
void display_footpad(footpads_state_t footpad)
{
    // Stop any current animation
    stop_animation();

    // Clear the current display
    if (LCM_SUCCESS != status_leds_set_color(&colors.black, 0U, STATUS_LEDS_COUNT - 1U))
    {
        fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
    }

    switch (footpad)
    {
    case LEFT_FOOTPAD:
        if (LCM_SUCCESS != status_leds_set_color(&custom_color, 0U, (STATUS_LEDS_COUNT / 2U) - 1U))
        {
            fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
        }
        break;
    case RIGHT_FOOTPAD:
        if (LCM_SUCCESS !=
            status_leds_set_color(&custom_color, (STATUS_LEDS_COUNT / 2), STATUS_LEDS_COUNT - 1U))
        {
            fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
        }
        break;
    case LEFT_FOOTPAD | RIGHT_FOOTPAD:
        if (LCM_SUCCESS != status_leds_set_color(&custom_color, 0U, STATUS_LEDS_COUNT - 1U))
        {
            fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
        }
        break;
    default:
        // Do nothing
        break;
    }

    if (LCM_SUCCESS != status_leds_refresh())
    {
        fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
    }
}

void status_leds_disable_beeper_callback(void)
{
    update_display(EVENT_COMMAND_TOGGLE_BEEPER);
}

void status_leds_boot_callback(void)
{
    status_leds_start_animation_option(status_leds_settings->boot_animation);
}

void status_leds_shutdown_callback(void)
{
    status_leds_start_animation_option(status_leds_settings->shutdown_animation);
}

void status_leds_idle_dozing_callback(void)
{
    status_leds_start_animation_option(status_leds_settings->dozing_animation);
}

void status_leds_idle_default_callback(void)
{
    status_leds_start_animation_option(status_leds_settings->idle_animation);
}

void status_leds_riding_callback(void)
{
    status_leds_start_animation_option(status_leds_settings->ride_animation);
}

void status_leds_color_callback(void)
{
    stop_animation();
    hsl_to_rgb(degrees_to_fixed16(status_leds_settings->personal_color), SATURATION_DEFAULT,
               LIGHTNESS_DEFAULT, &custom_color);
    status_leds_set_color(&custom_color, 0U, STATUS_LEDS_COUNT - 1U);
    status_leds_refresh();
}

/**
 * @brief Handler for BOARD_MODE_OFF state.
 *
 * @param event The event to handle in the BOARD_MODE_OFF state.
 */
void status_leds_handle_off(event_type_t event)
{
    // Turn off the status LEDs if the event is a board mode change
    if (event == EVENT_BOARD_MODE_CHANGED)
    {
        status_leds_turn_off();
    }
    // No else needed, no events to handle in this state
}

/**
 * @brief Handles the BOARD_MODE_BOOTING state.
 *
 * @param event The event to handle in the BOARD_MODE_BOOTING state.
 */
void status_leds_handle_booting(event_type_t event)
{
    if (event == EVENT_BOARD_MODE_CHANGED)
    {
        status_leds_start_animation_option(status_leds_settings->boot_animation);
    }
    // No else needed, no other events to handle in this state
}

/**
 * @brief Handles the BOARD_MODE_FAULT state.
 *
 * @param event The event to handle in the BOARD_MODE_FAULT state.
 */
void status_leds_handle_fault(event_type_t event)
{
    if (event == EVENT_BOARD_MODE_CHANGED)
    {
        // Default to red color
        const status_leds_color_t *color = &colors.red;

        // If this is an internal fault, use magenta color
        if (board_submode_get() == BOARD_SUBMODE_FAULT_INTERNAL)
        {
            // If this is an internal fault, use yellow color
            color = &colors.magenta;
        }

        // Start the red/yellow fault animation
        fill_animation_setup(status_leds_buffer, COLOR_MODE_RGB, BRIGHTNESS_MODE_SEQUENCE,
                            FILL_MODE_SOLID, 0U, STATUS_LEDS_COUNT - 1U,
                            FIXED16(0.0),   // hue min
                            FIXED16(0.0),   // hue max
                            0,   // color change speed
                            FIXED16(0.0),   // brightness min
                            FIXED16(1.0),   // brightness max
                            250, // brightness speed
                            0xF000, // bright sequence
                            color   // RGB color
        );
    }
    // No else needed, no other events to handle in this state
}

/**
 * @brief Handles the BOARD_MODE_IDLE state when the board is active.
 *
 * @param event The event to handle in the BOARD_MODE_IDLE state.
 */
void status_leds_handle_idle_active(event_type_t event)
{
    // Display the battery level unless the footpads are pressed
    footpads_state_t footpads = footpads_get_state();
    if (footpads == NONE_FOOTPAD)
    {
        display_battery(vesc_serial_get_battery_level());
    }
    else
    {
        display_footpad(footpads);
    }
}

/**
 * @brief Handles the BOARD_MODE_IDLE state when the board is shutting down.
 *
 * @param event The event to handle in the BOARD_MODE_IDLE state.
 */
void status_leds_handle_idle_shutting_down(event_type_t event)
{
    switch (event)
    {
    case EVENT_BOARD_MODE_CHANGED:
        // Fall through intentional
    case EVENT_COMMAND_TOGGLE_LIGHTS:
        // Fall through intentional
    case EVENT_COMMAND_TOGGLE_BEEPER:
        status_leds_start_animation_option(status_leds_settings->shutdown_animation);
        break;
    default:
        // If there is no shutdown animation, do the same as idle active
        if (status_leds_settings->shutdown_animation == ANIMATION_OPTION_NONE)
        {
            status_leds_handle_idle_active(event);
        }
        // No else needed, animation keeps running
        break;
    }
}

void status_leds_handle_idle_default(event_type_t event)
{
    // If there's no idle animation, this is the same as idle active
    if (status_leds_settings->idle_animation == ANIMATION_OPTION_NONE)
    {
        status_leds_handle_idle_active(event);
    }
    else
    {
        // Otherwise, only start the animation if this is a board mode or
        // settings change
        if ((event == EVENT_BOARD_MODE_CHANGED) || (event == EVENT_COMMAND_TOGGLE_LIGHTS) ||
            (event == EVENT_COMMAND_TOGGLE_BEEPER))
        {
            status_leds_start_animation_option(status_leds_settings->idle_animation);
        }
    }
}

void status_leds_handle_idle_dozing(event_type_t event)
{
    switch (event)
    {
    case EVENT_BOARD_MODE_CHANGED:
        // Fall through intentional
    case EVENT_COMMAND_TOGGLE_LIGHTS:
        // Fall through intentional
    case EVENT_COMMAND_TOGGLE_BEEPER:
        status_leds_start_animation_option(status_leds_settings->dozing_animation);
        break;
    default:
        // Do nothing
        break;
    }
}

void status_leds_handle_idle_config(event_type_t event)
{
    switch (event)
    {
    case EVENT_BOARD_MODE_CHANGED:
        // Fall through intentional
    case EVENT_COMMAND_CONTEXT_CHANGED:
        // Stop any current animations and display magenta
        stop_animation();
        if (LCM_SUCCESS != status_leds_set_color(&colors.magenta, 0U, STATUS_LEDS_COUNT - 1U))
        {
            // Failed to set color, return early
            return;
        }
        status_leds_refresh();
        break;
    default:
        // Do nothing
        break;
    }
}

/**
 * @brief Shared solid-red sine-brightness "pulse/breathe" animation, used by
 * both riding-danger (fast pulse) and disabled/locked (slow breathe) - the
 * only difference between them is how fast the brightness cycles.
 */
static void status_leds_handle_solid_red_pulse(event_type_t event, uint32_t period_ms)
{
    switch (event)
    {
    case EVENT_BOARD_MODE_CHANGED:
        fill_animation_setup(status_leds_buffer, COLOR_MODE_RGB, BRIGHTNESS_MODE_SINE,
                             FILL_MODE_SOLID, 0U, STATUS_LEDS_COUNT - 1U,
                             FIXED16(0.0),   // hue min
                             FIXED16(0.0),   // hue max
                             0,   // color change speed
                             FIXED16(0.1),   // brightness min
                             FIXED16(1.0),   // brightness max
                             period_ms, // brightness change speed
                             0U,
                             &colors.red // RGB color
        );
        break;
    default:
        // Do nothing
        break;
    }
}

#ifdef ENABLE_APP_INTEGRATION
/**
 * @brief Handles the disabled (locked) status LEDs based on the given event.
 *
 * Slowly breathing solid red bar, same shape as riding-danger but at a much
 * slower period. The overall brightness floor is forced separately (see
 * EVENT_HANDLER(status_leds, state_changed)) so this indicator can't be
 * made invisible by the configured status brightness.
 *
 * @param event The event type that triggers the LED status change.
 */
void status_leds_handle_disabled(event_type_t event)
{
    status_leds_handle_solid_red_pulse(event, DISABLED_BREATH_PERIOD);
}
#endif

/**
 * @brief Handles the status LEDs display when riding slowly.
 *
 * We give the rider an opportunity to check the battery level when riding
 * slowly.
 */
void status_leds_handle_riding_slow(event_type_t event)
{
    int16_t battery_level = vesc_serial_get_battery_level();
    display_battery(battery_level);
}

/**
 * @brief Handles the status LEDs display when riding normally.
 *
 * This function updates the status LEDs display when the board is riding
 * normally. It will display the battery level if it is below a certain
 * threshold, and otherwise will not display anything unless the always on
 * ride animation is set.
 *
 * @param event The event that triggered the update.
 */
void status_leds_handle_riding_normal(event_type_t event)
{
    int16_t battery_level = vesc_serial_get_battery_level();

    if (battery_level <= LOW_BATTERY_THRESHOLD)
    {
        display_battery(battery_level);
    }
    else if (get_animation_id() != ride_animation_id)
    {
        // Check if the always on ride animation is set
        if (status_leds_settings->ride_animation != ANIMATION_OPTION_NONE)
        {
            // Start the ride animation
            ride_animation_id =
                status_leds_start_animation_option(status_leds_settings->ride_animation);
        }
        else
        {
            // Don't display anything while riding
            ride_animation_id = fade_animation_setup(
                status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT, status_leds_turn_off);
        }
    }
}

/**
 * @brief Updates the status LEDs display based on the current board state.
 *
 * This function updates the status LEDs display based on the current board
 * state. It is called whenever the board state changes, and is responsible
 * for setting the appropriate colors on the status LEDs.
 *
 * @param event The event that triggered the update.
 */
void update_display(event_type_t event)
{
    // Determine the current state
    switch (board_mode_get())
    {
    case BOARD_MODE_OFF:
        status_leds_handle_off(event);
        break;
    case BOARD_MODE_BOOTING:
        status_leds_handle_booting(event);
        break;
    case BOARD_MODE_FAULT:
        status_leds_handle_fault(event);
        break;
#ifdef ENABLE_APP_INTEGRATION
    case BOARD_MODE_DISABLED:
        status_leds_handle_disabled(event);
        break;
#endif
    case BOARD_MODE_IDLE:
        switch (board_submode_get())
        {
        case BOARD_SUBMODE_IDLE_SHUTTING_DOWN:
            status_leds_handle_idle_shutting_down(event);
            break;
        case BOARD_SUBMODE_IDLE_ACTIVE:
            status_leds_handle_idle_active(event);
            break;
        case BOARD_SUBMODE_IDLE_DEFAULT:
            status_leds_handle_idle_default(event);
            break;
        case BOARD_SUBMODE_IDLE_DOZING:
            status_leds_handle_idle_dozing(event);
            break;
        case BOARD_SUBMODE_IDLE_CONFIG:
            status_leds_handle_idle_config(event);
            break;
        default:
            fault(EMERGENCY_FAULT_INVALID_STATE);
            break;
        }
        break;
    case BOARD_MODE_RIDING:
    {
        int16_t duty_cycle = vesc_serial_get_duty_cycle();

        // Duty cycle takes priority over the RPM-driven submodes below: once
        // it crosses the gauge threshold, show a proportional bar (green,
        // turning red at the danger threshold) instead of the normal
        // battery/ride display. BOARD_SUBMODE_RIDING_DANGER's hysteresis
        // floor is always above the gauge threshold, so that submode is
        // always covered by this branch.
        if (duty_cycle >= DUTY_CYCLE_GAUGE_THRESHOLD)
        {
            display_duty_cycle(duty_cycle);
        }
        else
        {
            switch (board_submode_get())
            {
            case BOARD_SUBMODE_RIDING_NORMAL:
                status_leds_handle_riding_normal(event);
                break;
            case BOARD_SUBMODE_RIDING_SLOW:
                status_leds_handle_riding_slow(event);
                break;
            case BOARD_SUBMODE_RIDING_STOPPED:
                // Riding stopped is the same behavior as idle active
                status_leds_handle_idle_active(event);
                break;
            default:
                fault(EMERGENCY_FAULT_INVALID_STATE);
                break;
            }
        }
        break;
    }
    case BOARD_MODE_CHARGING:
        // I need an ADV to implement this ;)
        break;
    case BOARD_MODE_UNKNOWN:
        // Fall through to default
    default:
        fault(EMERGENCY_FAULT_INVALID_STATE);
        break;
    }
}

/**
 * @brief Sets the color of the status LEDs from begin to end.
 *
 * Sets the color of the status LEDs from begin to end. If the range is invalid,
 * returns LCM_ERROR. Otherwise, returns LCM_SUCCESS.
 *
 * @param color The color to set the LEDs to.
 * @param begin The starting index of the LEDs to set.
 * @param end The ending index of the LEDs to set.
 *
 * @return LCM_SUCCESS on success, LCM_ERROR on failure.
 */
lcm_status_t status_leds_set_color(const status_leds_color_t *color, uint8_t begin, uint8_t end)
{
    lcm_status_t result = LCM_SUCCESS;

    if ((begin > end) || (end > (STATUS_LEDS_COUNT - 1U)) || (color == NULL))
    {
        result = LCM_ERROR;
    }
    else
    {
        for (uint8_t i = begin; i <= end; i++)
        {
            // Deep copy the color struct
            status_leds_buffer[i].r = color->r;
            status_leds_buffer[i].g = color->g;
            status_leds_buffer[i].b = color->b;
        }
    }

    return result;
}

/**
 * @brief Refreshes the status LEDs display.
 *
 * This function updates the status LEDs hardware with the current color
 * buffer, ensuring that the LEDs show the latest colors set by the system.
 */

lcm_status_t status_leds_refresh(void)
{
    status_leds_hw_refresh();
    return LCM_SUCCESS;
}

/**
 * @brief Turn off all status LEDs.
 *
 * This function sets all LEDs to black and refreshes the status LEDs
 * hardware. It is intended to be used when the board is turned off or
 * actively riding.
 */
void status_leds_turn_off(void)
{
    // Stop any animations
    stop_animation();

    if (LCM_SUCCESS != status_leds_set_color(&colors.black, 0U, STATUS_LEDS_COUNT - 1U))
    {
        fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
    }

    if (LCM_SUCCESS != status_leds_refresh())
    {
        fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
    }
}

void status_leds_disable_lights_callback(void)
{
    status_leds_turn_off();
    status_leds_hw_enable(false);
}

EVENT_HANDLER(status_leds, state_changed)
{
    bool_t leds_enabled = status_leds_settings->enable_status_leds;
    bool_t is_config =
        (board_mode_get() == BOARD_MODE_IDLE) && (board_submode_get() == BOARD_SUBMODE_IDLE_CONFIG);

    if (event == EVENT_BOARD_MODE_CHANGED)
    {
        // Configuration mode is confusing to navigate with no visible
        // feedback, so the status LEDs are forced on for its duration
        // regardless of the user's enable_status_leds setting, then restored
        // to that setting on exit
        if (is_config)
        {
            status_leds_hw_enable(true);
        }
        else if ((data->board_mode.previous_mode == BOARD_MODE_IDLE) &&
                 (data->board_mode.previous_submode == BOARD_SUBMODE_IDLE_CONFIG))
        {
            status_leds_hw_enable(leds_enabled);
        }
    }

    // The config-mode indicator must stay visible even if status LEDs are
    // otherwise disabled by the user
    leds_enabled = leds_enabled || is_config;

#ifdef ENABLE_APP_INTEGRATION
    if (event == EVENT_BOARD_MODE_CHANGED)
    {
        if (data->board_mode.mode == BOARD_MODE_DISABLED)
        {
            // Force visible, ignoring the configured brightness, so the
            // locked indicator can never be made invisible
            status_leds_hw_set_brightness(255U);
        }
        else if (data->board_mode.previous_mode == BOARD_MODE_DISABLED)
        {
            // Restore the configured brightness on unlock
            status_leds_hw_set_brightness(status_leds_settings->status_brightness);
        }
    }

    // The locked indicator must stay visible even if status LEDs are
    // otherwise disabled by the user
    leds_enabled = leds_enabled || (board_mode_get() == BOARD_MODE_DISABLED);
#endif

    if (leds_enabled)
    {
        update_display(event);
    }
    // No else needed, status LEDs are disabled
}

EVENT_HANDLER(status_leds, command)
{
    switch (event)
    {
    case EVENT_COMMAND_TOGGLE_LIGHTS:
        if (status_leds_settings->enable_status_leds)
        {
            status_leds_hw_enable(true);
            update_display(event);
        }
        else
        {
            // Fade out the lights and then disable
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_disable_lights_callback);
        }
        break;
    case EVENT_COMMAND_TOGGLE_BEEPER:
        if (!status_leds_settings->enable_beep)
        {
            if (LCM_SUCCESS != status_leds_set_color(&colors.red, 0U, STATUS_LEDS_COUNT - 1U))
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_disable_beeper_callback);
        }
        break;
    case EVENT_COMMAND_CONTEXT_CHANGED:
        // When the context changes, briefly flash a unique color on the status
        // LEDs to indicate to the user which context is active
        switch (data->context)
        {
        case COMMAND_PROCESSOR_CONTEXT_BOOT_ANIMATION:
            if (LCM_SUCCESS !=
                status_leds_set_color(&colors.light_blue, 0U, STATUS_LEDS_COUNT - 1U))
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_boot_callback);
            break;
        case COMMAND_PROCESSOR_CONTEXT_IDLE_ANIMATION:
            if (LCM_SUCCESS != status_leds_set_color(&colors.green, 0U, STATUS_LEDS_COUNT - 1U))
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_idle_default_callback);
            break;
        case COMMAND_PROCESSOR_CONTEXT_DOZING_ANIMATION:
            if (LCM_SUCCESS != status_leds_set_color(&colors.orange, 0U, STATUS_LEDS_COUNT - 1U))
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_idle_dozing_callback);
            break;
        case COMMAND_PROCESSOR_CONTEXT_SHUTDOWN_ANIMATION:
            if (LCM_SUCCESS != status_leds_set_color(&colors.red, 0U, STATUS_LEDS_COUNT - 1U))
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_shutdown_callback);
            break;
        case COMMAND_PROCESSOR_CONTEXT_RIDING_ANIMATION:
            if (LCM_SUCCESS != status_leds_set_color(&colors.white, 0U, STATUS_LEDS_COUNT - 1U))
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_riding_callback);
            break;
        case COMMAND_PROCESSOR_CONTEXT_PERSONAL_COLOR:
            if (LCM_SUCCESS !=
                status_leds_set_color(&colors.red, 0U, 2U)) // Set first 3 LEDs to red
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            if (LCM_SUCCESS !=
                status_leds_set_color(&colors.green, 3U, 6U)) // Set middle 4 LEDs to green
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            if (LCM_SUCCESS !=
                status_leds_set_color(&colors.blue, 7U, 9U)) // Set last 3 LEDs to blue
            {
                fault(EMERGENCY_FAULT_UNEXPECTED_ERROR);
            }
            fade_animation_setup(status_leds_buffer, STATUS_LEDS_FADE_TO_BLACK_TIMEOUT,
                                 status_leds_color_callback);
            break;
        case COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS:
            // Turn the status LEDs white and flashing so the user can
            // see the brightness change
            fill_animation_setup(status_leds_buffer, COLOR_MODE_RGB, BRIGHTNESS_MODE_FLASH,
                                 FILL_MODE_SOLID, 0U, STATUS_LEDS_COUNT - 1U,
                                 FIXED16(0.0),   // hue min
                                 FIXED16(0.0),   // hue max
                                 0,   // color change speed
                                 FIXED16(0.0),   // brightness min
                                 FIXED16(1.0),   // brightness max
                                 500, // brightness change speed
                                 0U,
                                 &colors.white // RGB color
            );
            break;
        default:
            update_display(event);
            break;
        }
        break;
    case EVENT_COMMAND_SETTINGS_CHANGED:
        switch (data->context)
        {
        case COMMAND_PROCESSOR_CONTEXT_STATUS_BAR_BRIGHTNESS:
            status_leds_hw_set_brightness(status_leds_settings->status_brightness);
            status_leds_refresh();
            break;
        case COMMAND_PROCESSOR_CONTEXT_BOOT_ANIMATION:
            status_leds_start_animation_option(status_leds_settings->boot_animation);
            break;
        case COMMAND_PROCESSOR_CONTEXT_IDLE_ANIMATION:
            status_leds_start_animation_option(status_leds_settings->idle_animation);
            break;
        case COMMAND_PROCESSOR_CONTEXT_DOZING_ANIMATION:
            status_leds_start_animation_option(status_leds_settings->dozing_animation);
            break;
        case COMMAND_PROCESSOR_CONTEXT_SHUTDOWN_ANIMATION:
            status_leds_start_animation_option(status_leds_settings->shutdown_animation);
            break;
        case COMMAND_PROCESSOR_CONTEXT_RIDING_ANIMATION:
            status_leds_start_animation_option(status_leds_settings->ride_animation);
            break;
        case COMMAND_PROCESSOR_CONTEXT_PERSONAL_COLOR:
            status_leds_color_callback();
            break;
        default:
            update_display(event);
            break;
        }
        break;
    default:
        break;
    }
}
