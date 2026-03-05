#include "hk32f030m_flash.h"

// FLASH stub implementations - no-ops for simulation

void FLASH_Unlock(void) {
    // No-op
}

void FLASH_Lock(void) {
    // No-op
}

FLASH_Status FLASH_ErasePage(uint32_t Page_Address) {
    return FLASH_COMPLETE;
}

FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data) {
    return FLASH_COMPLETE;
}

FLASH_Status FLASH_ProgramHalfWord(uint32_t Address, uint16_t Data) {
    return FLASH_COMPLETE;
}

uint32_t FLASH_GetFlagStatus(uint32_t FLASH_FLAG) {
    return 0;
}

void FLASH_ClearFlag(uint32_t FLASH_FLAG) {
    // No-op
}
