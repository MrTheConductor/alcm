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

// System timeout configuration
#define IDLE_ACTIVE_TIMEOUT (4 * 1000)        // Time before transitioning to default idle
#define IDLE_DEFAULT_TIMEOUT (2 * 60 * 1000)  // Time before transitioning to dozing idle
#define IDLE_DOZING_TIMEOUT (8 * 60 * 1000)   // Time before transitioning to shutting down
#define IDLE_SHUTTING_DOWN_TIMEOUT (1 * 1000) // Time allowed to abort shutdown

// Ride mode configuration
#define STOPPED_RPM_THRESHOLD 20           // RPM threshold for stopped
#define SLOW_RPM_THRESHOLD 2000            // RPM threshold for slow riding speed (3-4 MPH)
#define DUTY_CYCLE_DANGER_THRESHOLD 90.0f  // Duty cycle threshold for danger zone
#define DUTY_CYCLE_WARNING_THRESHOLD 80.0f // Duty cycle threshold for warning zone

// Button configuration
#define SINGLE_CLICK_MIN 10  // Shortest time for a single click (ms)
#define SINGLE_CLICK_MAX 180 // Longest time for a single click (ms)
#define REPEAT_WINDOW 200    // Time window for repeat clicks (ms)
#define HOLD_MAX 500         // Time for a hold event (ms)

// Event queue configuration
#define EVENT_QUEUE_SIZE 8U   // Maximum number of events in the queue
#define MAX_SUBSCRIPTIONS 32U // Maximum number of event subscribers
#define MAX_TIMERS 8U         // Maximum number of system timers

// Status LEDs configuration
#define STATUS_LEDS_FADE_TO_BLACK_TIMEOUT (1000U) // Time to fade to black when shutting down
#define LOW_BATTERY_THRESHOLD (15.0f)             // Threshold for yellow/always on indicator
#define CRITICAL_BATTERY_THRESHOLD (5.0f)         // Threshold for red flashing indicator

// Buzzer configuration
#define ENABLE_BUZZER 1 // Enable the buzzer module
#undef BUZZER_ENABLE_WARNING // Enable beeper at warning level
#define BUZZER_ENABLE_DANGER 1 // Enable beeper at danger level

// DEBUG
#undef UART_DEBUG
#undef MANUAL_HSI_TRIMMING 

#endif