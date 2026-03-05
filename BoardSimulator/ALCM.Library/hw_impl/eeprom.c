/*
 * BoardSimulator - EEPROM emulation using file storage
 * Implements eeprom.h interface with RAM + file persistence
 */

#include "eeprom.h"
#include "hk32f030m_flash.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Simulated EEPROM size (match real hardware)
#define EEPROM_SIZE 1024
#define EEPROM_FILE "alcm_eeprom.bin"

// RAM copy of EEPROM
static uint8_t eeprom_data[EEPROM_SIZE];
static bool eeprom_initialized = false;

void eeprom_init(void) {
    if (eeprom_initialized) {
        return;
    }
    
    // Try to load from file
    FILE *f = fopen(EEPROM_FILE, "rb");
    if (f != NULL) {
        fread(eeprom_data, 1, EEPROM_SIZE, f);
        fclose(f);
    } else {
        // Initialize to 0xFF (erased flash state)
        memset(eeprom_data, 0xFF, EEPROM_SIZE);
    }
    
    eeprom_initialized = true;
}

void eeprom_read(uint16_t addr, uint8_t *data, uint16_t len) {
    if (!eeprom_initialized) {
        eeprom_init();
    }
    
    // Bounds check
    if (addr + len > EEPROM_SIZE) {
        return;
    }
    
    memcpy(data, &eeprom_data[addr], len);
}

void eeprom_write(uint16_t addr, uint8_t *data, uint16_t len) {
    if (!eeprom_initialized) {
        eeprom_init();
    }
    
    // Bounds check
    if (addr + len > EEPROM_SIZE) {
        return;
    }
    
    memcpy(&eeprom_data[addr], data, len);
    
    // Persist to file
    FILE *f = fopen(EEPROM_FILE, "wb");
    if (f != NULL) {
        fwrite(eeprom_data, 1, EEPROM_SIZE, f);
        fclose(f);
    }
}
