#ifndef __HK32F030M_FLASH_H__
#define __HK32F030M_FLASH_H__

#include "hk32f030m.h"

#ifdef __cplusplus
extern "C" {
#endif

// FLASH Status
typedef enum {
    FLASH_BUSY = 1,
    FLASH_ERROR_WRP,
    FLASH_ERROR_PROGRAM,
    FLASH_COMPLETE,
    FLASH_TIMEOUT
} FLASH_Status;

// FLASH Keys
#define FLASH_KEY1  ((uint32_t)0x45670123)
#define FLASH_KEY2  ((uint32_t)0xCDEF89AB)

// Function declarations (stubs)
void FLASH_Unlock(void);
void FLASH_Lock(void);
FLASH_Status FLASH_ErasePage(uint32_t Page_Address);
FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data);
FLASH_Status FLASH_ProgramHalfWord(uint32_t Address, uint16_t Data);
uint32_t FLASH_GetFlagStatus(uint32_t FLASH_FLAG);
void FLASH_ClearFlag(uint32_t FLASH_FLAG);

#ifdef __cplusplus
}
#endif

#endif /* __HK32F030M_FLASH_H__ */
