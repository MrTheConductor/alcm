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
#include <string.h>

#include "settings.h"
#include "crc16_ccitt.h"
#include "eeprom.h"
#include "event_queue.h"

/**
 * @brief      Magic number to identify valid settings.
 *
 * @details    This magic number is used to identify valid settings in the EEPROM.
 *             If the magic number in the settings does not match this value, the
 *             settings are considered invalid and will be reset to default values.
 */
#define MAGIC_NUMBER 0xbeef0001

/**
 * @brief      Structure to represent the settings in the EEPROM.
 */
typedef struct
{
    settings_t settings; // settings
    uint16_t crc;        // CRC
} settings_eeprom_t;

static settings_eeprom_t eeprom = {0};
static bool_t settings_loaded = false;

EVENT_HANDLER(settings, mode_changed);

/**
 * @brief      Resets the settings to their default values.
 *
 * @details    This function resets all settings to their default values.
 *             The settings are saved to the EEPROM after this function is
 *             called. The default is intended to be pretty similar to Tony's
 *             default settings.
 */
void settings_reset(void)
{
    // Clear the EEPROM structure - technically, this isn't necessary
    // because the structure will be initialized by the following instructions,
    // however, we do it anyway just to be certain nothing is uninitialized.
    memset(&eeprom, 0, sizeof(eeprom));

    eeprom.settings.magic = MAGIC_NUMBER;
    eeprom.settings.enable_beep = true;
    eeprom.settings.enable_headlights = true;
    eeprom.settings.enable_status_leds = true;
    eeprom.settings.boot_animation = ANIMATION_OPTION_FLOATWHEEL_CLASSIC;
    eeprom.settings.idle_animation = ANIMATION_OPTION_NONE;
    eeprom.settings.dozing_animation = ANIMATION_OPTION_NONE;
    eeprom.settings.shutdown_animation = ANIMATION_OPTION_NONE;
    eeprom.settings.ride_animation = ANIMATION_OPTION_NONE;
    eeprom.settings.headlight_brightness = 0.8f;
    eeprom.settings.status_brightness = 0.8f;
    eeprom.settings.personal_color = 200.0f; // Light blue

    // Save
    settings_save();
}

/**
 * @brief      Checks if the settings are within valid ranges.
 *
 * @details    This function checks if the settings are within valid ranges.
 *             If any setting is out of range, the function returns false. Typically,
 *             the CRC will catch any corrutpion of the data at rest, but
 *             this is a extra sanity check to make sure nothing crazy happens.
 *
 * @return     True if the settings are valid, false otherwise.
 */
bool_t settings_range_check(void)
{
    bool_t valid = true;

    // Check magic number
    if (eeprom.settings.magic != MAGIC_NUMBER)
    {
        valid = false;
    }
    else if (eeprom.settings.headlight_brightness < 0.0f ||
             eeprom.settings.headlight_brightness > 1.0f)
    {
        valid = false;
    }
    else if (eeprom.settings.status_brightness < 0.0f || eeprom.settings.status_brightness > 1.0f)
    {
        valid = false;
    }
    else if (eeprom.settings.boot_animation >= ANIMATION_OPTION_COUNT ||
             eeprom.settings.idle_animation >= ANIMATION_OPTION_COUNT ||
             eeprom.settings.dozing_animation >= ANIMATION_OPTION_COUNT ||
             eeprom.settings.shutdown_animation >= ANIMATION_OPTION_COUNT)
    {
        valid = false;
    }
    else if ((eeprom.settings.enable_beep != true && eeprom.settings.enable_beep != false) ||
             (eeprom.settings.enable_headlights != true &&
              eeprom.settings.enable_headlights != false) ||
             (eeprom.settings.enable_status_leds != true &&
              eeprom.settings.enable_status_leds != false))
    {
        valid = false;
    }
    else if (eeprom.settings.personal_color < 0.0f || eeprom.settings.personal_color > 360.0f)
    {
        valid = false;
    }

    return valid;
}

/**
 * @brief      Initializes the settings module by loading settings from EEPROM.
 *
 * @details    This function reads the settings from the EEPROM and checks the
 * CRC to verify the data integrity. If the CRC is invalid, indicating potential
 *             corruption, the settings are reset to default values.
 */
lcm_status_t settings_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    eeprom_read(0x0000, (uint8_t *)&eeprom, sizeof(eeprom));

    // Check CRC
    if (eeprom.crc != crc16_ccitt((uint8_t *)&eeprom.settings, sizeof(eeprom.settings)))
    {
        // Invalid CRC, something is corrupt or we need to reset
        settings_reset();
    }

    // Even if the CRC is valid, we should still check the settings
    if (!settings_range_check())
    {
        // Invalid settings, reset
        settings_reset();
    }

    // Subscribe to the mode changed event so we can save the settings
    // at shutdown
    SUBSCRIBE_EVENT(settings, EVENT_BOARD_MODE_CHANGED, mode_changed);

    // Settings are loaded
    settings_loaded = true;

    return status;
}

/**
 * @brief      Saves the settings to the EEPROM.
 *
 * @details    This function saves the current settings to the EEPROM by first
 *             updating the CRC of the settings and then comparing the stored
 *             settings to the current settings. If the stored settings are
 *             different from the current settings, the current settings are
 *             written to the EEPROM.
 */
void settings_save(void)
{
    settings_eeprom_t stored = {0};

    // Update current CRC
    eeprom.crc = crc16_ccitt((uint8_t *)&eeprom.settings, sizeof(eeprom.settings));

    // Get stored settings from EEPROM
    eeprom_read(0x0000, (uint8_t *)&stored, sizeof(stored));

    // Compare stored settings to current settings
    if (memcmp(&stored.settings, &eeprom.settings, sizeof(eeprom.settings)) != 0)
    {
        // Write to EEPROM
        eeprom_write(0x0000, (uint8_t *)&eeprom, sizeof(eeprom));
    }
}

/**
 * @brief      Gets the current settings.
 *
 * @details    This function returns a pointer to the current settings
 * structure.
 *
 * @return     Pointer to the current settings structure.
 */
settings_t *settings_get(void)
{
    settings_t *settings = &eeprom.settings;

    if (!settings_loaded)
    {
        if (LCM_SUCCESS != settings_init())
        {
            settings = NULL;
            fault(EMERGENCY_FAULT_INIT_FAIL);
        }
    }

    return settings;
}

/**
 * @file settings.c
 * @brief Event handler for mode changes in the settings module.
 *
 * This file contains the implementation of the event handler that is triggered
 * when the mode is changed in the settings module. It saves the settings
 * when the board mode is shutting down.
 *
 * @event settings
 * @event_handler mode_changed
 */
EVENT_HANDLER(settings, mode_changed)
{
    if (data->board_mode.mode == BOARD_MODE_IDLE &&
        data->board_mode.submode == BOARD_SUBMODE_IDLE_SHUTTING_DOWN)
    {
        settings_save();
    }
}