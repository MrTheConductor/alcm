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
        hsl_to_rgb(status_leds_settings->personal_color, SATURATION_DEFAULT, LIGHTNESS_DEFAULT,
                   &custom_color);

        // Subscribe to events that trigger status changes
        //
        // Note: We don't subscribe to RPM or duty cycle here because they are
        // handled by the board state machine.
        SUBSCRIBE_EVENT(status_leds, EVENT_BOARD_MODE_CHANGED, state_changed);
        SUBSCRIBE_EVENT(status_leds, EVENT_FOOTPAD_CHANGED, state_changed);
        SUBSCRIBE_EVENT(status_leds, EVENT_BATTERY_LEVEL_CHANGED, state_changed);
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

#ifdef ENABLE_ROLL_EVENTS
        if (vesc_serial_get_imu_roll() < 0.0f)
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
                                 SIGMA_DEFAULT,
                                 0.0f,    // hue min
                                 360.0f,  // hue max
                                 3000.0f, // color change speed
                                 SCAN_START_DEFAULT, SCAN_END_NEVER, 0.0f,
                                 NULL // RGB color (ignored)
            );
        break;
#ifdef ENABLE_KNIGHT_RIDER_ANIMATION
    case ANIMATION_OPTION_KNIGHT_RIDER:
        animation_id = scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_SINE, COLOR_MODE_RGB,
                                            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
                                            SIGMA_DEFAULT,
                                            0.0f, // (not-used)
                                            0.0f, // (not-used)
                                            0.0f, // (not-used)
                                            SCAN_START_DEFAULT, SCAN_END_NEVER, 0.0f,
                                            &colors.red // RGB color
        );
        break;
#endif
    case ANIMATION_OPTION_RAINBOW_MIRROR:
        animation_id = fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_INCREASE,
                                            BRIGHTNESS_MODE_STATIC, FILL_MODE_HSV_GRADIENT_MIRROR,
                                            0U, STATUS_LEDS_COUNT - 1U,
                                            0.0f,    // hue min
                                            360.0f,  // hue max
                                            1500.0f, // color change speed
                                            0.0f,    // brightness min
                                            1.0f,    // brightness max
                                            0.0f,    // brightness change speed
                                            0U,
                                            NULL // RGB color (ignored)
        );
        break;
#ifdef ENABLE_EXPANDING_PULSE_ANIMATION
    case ANIMATION_OPTION_EXPANDING_PULSE:
        animation_id = scan_animation_setup(
            status_leds_buffer, SCAN_DIRECTION_LEFT_TO_RIGHT_MIRROR, COLOR_MODE_HSV_SINE,
            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
            SIGMA_DEFAULT,
            status_leds_settings->personal_color,                              // hue min
            CLAMP(status_leds_settings->personal_color + 15.0f, 0.0f, 360.0f), // hue max
            3000.0f, SCAN_START_DEFAULT, SCAN_END_NEVER, 0.0f, NULL);
        break;
#endif
#ifdef ENABLE_THE_FUZZ_ANIMATION
    case ANIMATION_OPTION_THE_FUZZ:
        animation_id = fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_SQUARE,
                                            BRIGHTNESS_MODE_SEQUENCE, FILL_MODE_SOLID, 0U,
                                            STATUS_LEDS_COUNT - 1U,
                                            0.0f,    // hue min
                                            240.0f,  // hue max
                                            1000.0f, // color change speed
                                            0.0f,    // brightness min
                                            1.0f,    // brightness max
                                            500.0f,  // brightness speed
                                            0xAA00,  // bright sequence
                                            NULL     // RGB color (ignored)
        );
        break;
#endif
    case ANIMATION_OPTION_120_SCROLL:
        animation_id = fill_animation_setup(
            status_leds_buffer, COLOR_MODE_HSV_INCREASE, BRIGHTNESS_MODE_STATIC,
            FILL_MODE_HSV_GRADIENT, first_led, last_led,
            status_leds_settings->personal_color,                               // hue min
            CLAMP(status_leds_settings->personal_color + 120.0f, 0.0f, 360.0f), // hue max
            2000.0f, // color change speed
            0.0f,    // brightness min
            1.0f,    // brightness max
            0.0f,    // brightness change speed
            0U,
            NULL // RGB color (ignored)
        );
        break;
#ifdef ENABLE_IMPLODING_PULSE_ANIMATION
    case ANIMATION_OPTION_IMPLODING_PULSE:
        animation_id = scan_animation_setup(
            status_leds_buffer, SCAN_DIRECTION_RIGHT_TO_LEFT_MIRROR, COLOR_MODE_HSV_SINE,
            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
            SIGMA_DEFAULT,
            status_leds_settings->personal_color,                              // hue min
            CLAMP(status_leds_settings->personal_color + 15.0f, 0.0f, 360.0f), // hue max
            3000.0f, SCAN_START_DEFAULT, SCAN_END_NEVER, 0.0f, NULL);
        break;
#endif
    case ANIMATION_OPTION_RAINBOW_BAR:
        animation_id = fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_INCREASE,
                                            BRIGHTNESS_MODE_STATIC, FILL_MODE_HSV_GRADIENT, first_led, last_led,
                                            0.0f,    // hue min
                                            360.0f,  // hue max
                                            1000.0f, // color change speed
                                            0.0f,    // brightness min
                                            1.0f,    // brightness max
                                            0.0f,    // brightness change speed
                                            0U,
                                            NULL // RGB color (ignored)
        );
        break;
    case ANIMATION_OPTION_COMPLEMENTARY_WAVE:
        animation_id = fill_animation_setup(
            status_leds_buffer, COLOR_MODE_HSV_SQUARE, BRIGHTNESS_MODE_STATIC,
            FILL_MODE_HSV_GRADIENT_MIRROR, 0U, STATUS_LEDS_COUNT - 1U,
            MIN(status_leds_settings->personal_color,
                tiny_fmodf(status_leds_settings->personal_color + 180.0f, 360.0f)),
            MAX(status_leds_settings->personal_color,
                tiny_fmodf(status_leds_settings->personal_color + 180.0f, 360.0f)),
            2000.0f, // color change speed
            1.0f,    // brightness min
            1.0f,    // brightness max
            0.0f,    // brightness change speed
            0U,
            NULL // RGB color
        );
        break;
    case ANIMATION_OPTION_PERSONAL_SCAN:
        animation_id = scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_SINE, COLOR_MODE_RGB,
                                            STATUS_LEDS_SCAN_SPEED, // scan speed in milliseconds
                                            SIGMA_DEFAULT,
                                            0.0f, // (not-used)
                                            0.0f, // (not-used)
                                            0.0f, // (not-used)
                                            SCAN_START_DEFAULT, SCAN_END_NEVER, 0.0f,
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
                                            5500.0f, // scan speed in milliseconds
                                            SIGMA_DEFAULT,
                                            0.0f, // (not-used)
                                            0.0f, // (not-used)
                                            0.0f, // (not-used)
                                            SCAN_START_DEFAULT, SCAN_END_MAX_MU, 0.0f,
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
 * @brief Displays the current battery level on the status LEDs
 *
 * This function uses the first 10 status LEDs to display the current battery
 * level. The LEDs are divided into 10 equal parts, with each part representing
 * 10% of the battery capacity.
 *
 * @param battery_level The current battery level, specified as a float
 *                       between 100.0 and 0.0
 */
void display_battery(float32_t battery_level)
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
                                                        0.0f,   // hue min (ignored)
                                                        0.0f,   // hue max (ignored)
                                                        0.0f,   // color change speed (ignored)
                                                        0.1f,   // brightness min
                                                        1.0f,   // brightness max
                                                        500.0f, // brightness change speed
                                                        0U,
                                                        &colors.red // RGB color
            );
        }
    }
    else
    {
        const status_leds_color_t *color = &colors.white;
        stop_animation();

        if (battery_level <= LOW_BATTERY_THRESHOLD)
        {
            color = &colors.orange;
        }

        scan_animation_setup(status_leds_buffer, SCAN_DIRECTION_LEFT_TO_RIGHT_FILL, COLOR_MODE_RGB,
                             500.0f, // scan speed in milliseconds
                             SIGMA_DEFAULT,
                             0.0f, // (not-used)
                             0.0f, // (not-used)
                             0.0f, // (not-used)
                             SCAN_START_MU, SCAN_END_SINGLE_TICK,
                             (float32_t)((battery_level / 10.0f) - 1.0f),
                             color // RGB color
        );
    }
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
    hsl_to_rgb(status_leds_settings->personal_color, SATURATION_DEFAULT, LIGHTNESS_DEFAULT,
               &custom_color);
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
        // Start the red/yellow fault animation
        fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_SQUARE, BRIGHTNESS_MODE_SEQUENCE,
                             FILL_MODE_SOLID, 0U, STATUS_LEDS_COUNT - 1U,
                             0.0f,   // hue min
                             60.0f,  // hue max
                             500.0f, // color change speed
                             0.0f,   // brightness min
                             1.0f,   // brightness max
                             250.0f, // brightness speed
                             0xF0F0, // bright sequence
                             NULL    // RGB color (ignored)
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

void status_leds_handle_riding_danger(event_type_t event)
{
    switch (event)
    {
    case EVENT_BOARD_MODE_CHANGED:
        fill_animation_setup(status_leds_buffer, COLOR_MODE_RGB, BRIGHTNESS_MODE_SINE,
                             FILL_MODE_SOLID, 0U, STATUS_LEDS_COUNT - 1U,
                             0.0f,   // hue min
                             0.0f,   // hue max
                             0.0f,   // color change speed
                             0.1f,   // brightness min
                             1.0f,   // brightness max
                             250.0f, // brightness change speed
                             0U,
                             &colors.red // RGB color
        );
        break;
    default:
        // Do nothing
        break;
    }
}

/**
 * @brief Handles the riding warning status LEDs based on the given event.
 *
 * This function sets up the LED animation when the board mode changes.
 *
 * @param event The event type that triggers the LED status change.
 *
 * Supported events:
 * - EVENT_BOARD_MODE_CHANGED: Sets up an HSV gradient mirror animation with sine brightness mode.
 */
void status_leds_handle_riding_warning(event_type_t event)
{
    switch (event)
    {
    case EVENT_BOARD_MODE_CHANGED:
        fill_animation_setup(status_leds_buffer, COLOR_MODE_HSV_SQUARE, BRIGHTNESS_MODE_SINE,
                             FILL_MODE_HSV_GRADIENT_MIRROR, 0U, STATUS_LEDS_COUNT - 1U,
                             10.0f,  // hue min
                             40.0f,  // hue max
                             350.0f, // color change speed
                             0.7f,   // brightness min
                             1.0f,   // brightness max
                             175.0f, // brightness change speed
                             0U,
                             NULL // RGB color (ignored)
        );
        break;
    default:
        // Do nothing
        break;
    }
}

/**
 * @brief Handles the status LEDs display when riding slowly.
 *
 * We give the rider an opportunity to check the battery level when riding
 * slowly.
 */
void status_leds_handle_riding_slow(event_type_t event)
{
    float32_t battery_level = vesc_serial_get_battery_level();
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
    float32_t battery_level = vesc_serial_get_battery_level();

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
        switch (board_submode_get())
        {
        case BOARD_SUBMODE_RIDING_DANGER:
            status_leds_handle_riding_danger(event);
            break;
        case BOARD_SUBMODE_RIDING_WARNING:
            status_leds_handle_riding_warning(event);
            break;
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
        break;
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
    if (status_leds_settings->enable_status_leds)
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
                                 0.0f,   // hue min
                                 0.0f,   // hue max
                                 0.0f,   // color change speed
                                 0.0f,   // brightness min
                                 1.0f,   // brightness max
                                 500.0f, // brightness change speed
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
