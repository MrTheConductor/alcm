#ifndef __SYSTEM_HK32F030M_H__
#define __SYSTEM_HK32F030M_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t SystemCoreClock;

void SystemInit(void);
void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_HK32F030M_H__ */
