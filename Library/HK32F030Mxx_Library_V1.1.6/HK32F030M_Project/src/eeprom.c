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
#include "hk32f030m.h"
#include "eeprom.h"

#define EEPROM_ADDR (0x0c000000ul)

/**
 * @brief      Writes data to the EEPROM.
 *
 * @param[in]  addr  The address in the EEPROM to write to.
 * @param[in]  data  The data to write.
 * @param[in]  len   The number of bytes to write.
 */
void eeprom_write(uint16_t addr, uint8_t *data, uint16_t len)
{
    FLASH_Unlock();

    while (len--)
    {
        FLASH_Status status = EEPROM_EraseByte(EEPROM_ADDR + addr);
        if (status != FLASH_COMPLETE)
        {
            break;
        }
        status = EEPROM_ProgramByte(EEPROM_ADDR + addr, *data++);
        if (status != FLASH_COMPLETE)
        {
            break;
        }
        addr++;
    }

    FLASH_Lock();
}

/**
 * @brief      Reads data from the EEPROM.
 *
 * @param[in]  addr  The address in the EEPROM to read from.
 * @param[out] data  The data to read into.
 * @param[in]  len   The number of bytes to read.
 */
void eeprom_read(uint16_t addr, uint8_t *data, uint16_t len)
{
    while (len--)
    {
        *data++ = *(uint8_t *)(EEPROM_ADDR + addr);
        addr++;
    }
}