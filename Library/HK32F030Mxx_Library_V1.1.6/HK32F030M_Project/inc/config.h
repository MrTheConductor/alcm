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
// System timeout configuration 
//------------------------------------------------------------------------------
#define IDLE_ACTIVE_TIMEOUT (4 * 1000)        // Time before transitioning to default idle
#define IDLE_DEFAULT_TIMEOUT (2 * 60 * 1000)  // Time before transitioning to dozing idle
#define IDLE_DOZING_TIMEOUT (8 * 60 * 1000)   // Time before transitioning to shutting down
#define IDLE_SHUTTING_DOWN_TIMEOUT (1 * 1000) // Time allowed to abort shutdown

//------------------------------------------------------------------------------
// Ride mode configuration 
//------------------------------------------------------------------------------
#define STOPPED_RPM_THRESHOLD 20           // RPM threshold for stopped
#define SLOW_RPM_THRESHOLD 2000            // RPM threshold for slow riding speed (3-4 MPH)
#define DUTY_CYCLE_DANGER_THRESHOLD 90.0f  // Duty cycle threshold for danger zone
#define DUTY_CYCLE_WARNING_THRESHOLD 80.0f // Duty cycle threshold for warning zone

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
#define HEADLIGHTS_ENABLE_DOZING 1          // Enable dozing animation for headlights
#define HEADLIGHTS_ENABLE_SHUTTING_DOWN 1   // Enable shutting down animation for headlights 
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
#define ENABLE_STATUS_LEDS 1                      // Enable the status LEDs
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
#define ENABLE_KNIGHT_RIDER_ANIMATION 1 // Red "knight rider" animation
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
#define ENABLE_BUZZER 1 // Enable the buzzer module
#undef BUZZER_ENABLE_WARNING // Enable beeper at warning threshold
#define BUZZER_ENABLE_DANGER 1 // Enable beeper at danger threshold

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
#undef ENABLE_VOLTAGE_MONITORING // Enable battery voltage monitoring

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
#define ENABLE_IMU_EVENTS 1 // Enable IMU events

//------------------------------------------------------------------------------
// Debug configuration
//------------------------------------------------------------------------------
// These should be generally undefined in production builds.
#undef UART_DEBUG
#undef MANUAL_HSI_TRIMMING

#endif