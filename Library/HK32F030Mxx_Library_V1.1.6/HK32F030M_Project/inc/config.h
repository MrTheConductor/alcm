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
 *
 */

/**
 * @file config.h
 * @brief System configuration file for the ALCM project.
 *
 * This file contains the system configuration settings for the ALCM project.
 * It is intended to be provide a common location for system-wide settings that
 * can be easily modified.
 */
#ifndef CONFIG_H
#define CONFIG_H

//------------------------------------------------------------------------------
// Battery voltage curves 
//------------------------------------------------------------------------------
// These are the voltage curves for the Sony VTC6 and P42A cells used in
// various boards. The values are in millivolts and represent the voltage
// at each percentage point from 0% to 100%.
//
// If you are using a different battery, you will need to add your own
// voltage curve here.
#define VTC6_CELL_VOLTAGES {4200, 4064, 4015, 3895, 3821, 3745, 3655, 3559, 3459, 3292, 3000};
#define P42A_CELL_VOLTAGES {4200, 4064, 4015, 3895, 3821, 3745, 3655, 3559, 3459, 3292, 3000};

//------------------------------------------------------------------------------
// Target configuration 
//------------------------------------------------------------------------------
// The target board can be defined here, or in the command line.  If nothing is
// defined, it will default to TARGET_MULTI which includes support for all
// boards, but no app support.
#if !defined(TARGET_XRV) && !defined(TARGET_PINTV) && !defined(TARGET_GTV) && !defined(TARGET_DEV) && !defined(TARGET_MULTI)
#define TARGET_MULTI
#endif

// Stock PintV and XRV use 15S2P Sony VTC6 cells
#if defined(TARGET_XRV) || defined(TARGET_PINTV)
#define BATTERY_CELL_VOLTAGES VTC6_CELL_VOLTAGES
#define BATTERY_CELL_COUNT 15
// Stock GTV uses 20S2P P42A cells
#elif defined(TARGET_GTV)
#define BATTERY_CELL_VOLTAGES P42A_CELL_VOLTAGES 
#define BATTERY_CELL_COUNT 20
// Fake battery for dev board
#elif defined(TARGET_DEV)
#define BATTERY_CELL_VOLTAGES VTC6_CELL_VOLTAGES
#define BATTERY_CELL_COUNT 15
#endif
// If you are using a different battery, you will need to define
// BATTERY_CELL_VOLTAGES and BATTERY_CELL_COUNT manually.

// ------------------------------------------------------------------------------
// Refloat support
// ------------------------------------------------------------------------------
// Refloat (formerly Float package) is a VESC package that adds allows some
// app based control of the LCM.
// #define ENABLE_REFLOAT // Enable Refloat support

//------------------------------------------------------------------------------
// System timeout configuration 
//------------------------------------------------------------------------------
#define IDLE_ACTIVE_TIMEOUT (4 * 1000)        // Time before transitioning to default idle
#define IDLE_DEFAULT_TIMEOUT (2 * 60 * 1000)  // Time before transitioning to dozing idle
#define IDLE_DOZING_TIMEOUT (8 * 60 * 1000)   // Time before transitioning to shutting down
#define IDLE_SHUTTING_DOWN_TIMEOUT (1 * 1000) // Time allowed to abort shutdown

//------------------------------------------------------------------------------
// Ride mode configuration 
//------------------------------------------------------------------------------
#define STOPPED_RPM_THRESHOLD (20)              // RPM threshold for stopped
#define SLOW_RPM_THRESHOLD (2000)               // RPM threshold for slow riding speed (3-4 MPH)
#define DUTY_CYCLE_DANGER_THRESHOLD (90.0f)     // Duty cycle threshold for danger zone
#define DUTY_CYCLE_WARNING_THRESHOLD (80.0f)    // Duty cycle threshold for warning zone

//------------------------------------------------------------------------------
// Button configuration 
//------------------------------------------------------------------------------
#define SINGLE_CLICK_MIN 10  // Shortest time for a single click (ms)
#define SINGLE_CLICK_MAX 180 // Longest time for a single click (ms)
#define REPEAT_WINDOW 200    // Time window for repeat clicks (ms)
#define HOLD_MAX 500         // Time for a hold event (ms)

//------------------------------------------------------------------------------
// Event queue configuration 
//------------------------------------------------------------------------------
#define EVENT_QUEUE_SIZE 8U   // Maximum number of events in the queue
#define MAX_SUBSCRIPTIONS 32U // Maximum number of event subscribers
#define MAX_TIMERS 8U         // Maximum number of system timers

// Headlights configuration
#define HEADLIGHTS_ENABLE_DOZING            // Enable dozing animation for headlights
#define HEADLIGHTS_ENABLE_SHUTTING_DOWN     // Enable shutting down animation for headlights 
#define SLOW_BREATH_PERIOD 6000U            // How fast to "breathe" headlights when dozing (ms)
#define FAST_BREATH_PERIOD 500U             // How fast to flash headlights (ms)
#define FADE_PERIOD 500U                    // How long to fade out headlights on disable (ms) 
#define RPM_HYSTERISIS 40.0f                // How many ERPMs (+/-) to allow before changing direction
#define HEADLIGHTS_IDLE_BRIGHTNESS 0.20f    // Brightness of headlights when idle (0.0 to 1.0) 

//------------------------------------------------------------------------------
// Status LED configuration 
//------------------------------------------------------------------------------
// The status LEDs are used to provide visual feedback for various events
// and to indicate battery levels.
//
// They can be disabled to save code space or if there's no status LEDs on the
// board like an XR.
#define ENABLE_STATUS_LEDS                        // Enable the status LEDs
#define STATUS_LEDS_FADE_TO_BLACK_TIMEOUT (1000U) // Time to fade to black when shutting down
#define LOW_BATTERY_THRESHOLD (15.0f)             // Threshold for yellow/always on indicator
#define CRITICAL_BATTERY_THRESHOLD (5.0f)         // Threshold for red flashing indicator
#define STATUS_LEDS_SCAN_SPEED (2000U)            // Speed of the scan animation (ms)

//------------------------------------------------------------------------------
// Animation configuration 
//------------------------------------------------------------------------------
// The animations are used to provide visual feedback for various events
// and to look cool.
//
// Individual animations can be enabled or disable to save code space. 
#undef ENABLE_FIRE_ANIMATION            // Fire effect animation 
#define ENABLE_KNIGHT_RIDER_ANIMATION   // Red "knight rider" animation
#undef ENABLE_EXPANDING_PULSE_ANIMATION // Expanding pulse animation 
#undef ENABLE_PULSE_ANIMATION           // Expanding pulse animation 
#undef ENABLE_THE_FUZZ_ANIMATION        // The Fuzz animation 

//------------------------------------------------------------------------------
// Buzzer configuration 
//------------------------------------------------------------------------------
// The buzzer can be used to provide audio feedback for various events.
// It can be used to indicate warnings, dangers, or other events.
//
// It can also be turned off completely to save code space and silence. 
#define ENABLE_BUZZER           // Enable the buzzer module
#undef BUZZER_ENABLE_WARNING    // Enable beeper at warning threshold
#define BUZZER_ENABLE_DANGER    // Enable beeper at danger threshold

//------------------------------------------------------------------------------
// Battery configuration 
//------------------------------------------------------------------------------
// There's a couple ways to get the battery level:
// 1. Read the input voltage and calculate a value based on
//    the number and type of cells installed.
// 2. Read the battery level directly from the VESC based
//    on their calculations.
//
// The first method is more accurate, but it requires
// programming the number of cells and battery curve into
// the firmware. The second method is less accurate, but
// requires no modification from board to board.
// #define ENABLE_VOLTAGE_MONITORING // Enable battery voltage monitoring

//------------------------------------------------------------------------------
// IMU configuration
//------------------------------------------------------------------------------
// We can use the VESC IMU to get the board orientation which allows us to
// dim the headlights when the board is pointed straight up or down.
// This is useful for preventing the headlights from blinding the rider when
// picking the board up.
//
// We can also use the IMU to detect when the board is on its side, to trigger
// dozing idle mode.
//
// This can be undefined to save code space if IMU features are not wanted.
#define ENABLE_VESC_IMU         // Enable VESC IMU commnunication
#define ENABLE_ROLL_EVENTS      // Enable roll events
#define ENABLE_PITCH_EVENTS     // Enable pitch events

//------------------------------------------------------------------------------
// Debug configuration
//------------------------------------------------------------------------------
// These should be generally undefined in production builds.
#undef UART_DEBUG
#undef MANUAL_HSI_TRIMMING

// Update configuration for Refloat
#if defined(ENABLE_REFLOAT)
#undef ENABLE_VESC_IMU              // Refloat has its own IMU reporting
#undef ENABLE_ROLL_EVENTS           // Refloat does not report IMU roll
#define ENABLE_PITCH_EVENTS         // Refloat does report IMU pitch 
#define ENABLE_VOLTAGE_MONITORING   // Refloat only reports voltage, not battery percentage 
#endif // ENABLE_REFLOAT

// Need manual HSI trimming for dev board
#if defined(TARGET_DEV) && !defined(MANUAL_HSI_TRIMMING)
#define MANUAL_HSI_TRIMMING
#endif

// XR board has no status LEDs
#if defined(TARGET_XRV) && defined(ENABLE_STATUS_LEDS)
#undef ENABLE_STATUS_LEDS
#undef ENABLE_ROLL_EVENTS
#endif

// Double-check configurations and provide errors if something is wrong
#if defined(ENABLE_VOLTAGE_MONITORING) && (!defined(BATTERY_CELL_VOLTAGES) || !defined(BATTERY_CELL_COUNT))
#error "ENABLE_VOLTAGE_MONITORING requires BATTERY_CELL_VOLTAGES and BATTERY_CELL_COUNT to be defined."
#endif

#endif