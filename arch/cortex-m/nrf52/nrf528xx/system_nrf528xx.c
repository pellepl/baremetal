#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wredundant-decls" // can someone explain why we need this?

#define SystemCoreClock SystemCoreClock_10
#define SystemCoreClockUpdate SystemCoreClockUpdate_10
#define SystemInit SystemInit_10
#include "system_nrf52810.c"
#undef SystemCoreClock
#undef SystemCoreClockUpdate
#undef SystemInit
#undef __SYSTEM_CLOCK_64M

#define SystemCoreClock SystemCoreClock_11
#define SystemCoreClockUpdate SystemCoreClockUpdate_11
#define SystemInit SystemInit_11
#include "system_nrf52811.c"
#undef SystemCoreClock
#undef SystemCoreClockUpdate
#undef SystemInit
#undef __SYSTEM_CLOCK_64M

#define SystemCoreClock SystemCoreClock_32
#define SystemCoreClockUpdate SystemCoreClockUpdate_32
#define SystemInit SystemInit_32
#include "system_nrf52.c"
#undef SystemCoreClock
#undef SystemCoreClockUpdate
#undef SystemInit
#undef __SYSTEM_CLOCK_64M

#define SystemCoreClock SystemCoreClock_33
#define SystemCoreClockUpdate SystemCoreClockUpdate_33
#define SystemInit SystemInit_33
#include "system_nrf52833.c"
#undef SystemCoreClock
#undef SystemCoreClockUpdate
#undef SystemInit
#undef __SYSTEM_CLOCK_64M

#define SystemCoreClock SystemCoreClock_40
#define SystemCoreClockUpdate SystemCoreClockUpdate_40
#define SystemInit SystemInit_40
#include "system_nrf52840.c"
#undef SystemCoreClock
#undef SystemCoreClockUpdate
#undef SystemInit
#undef __SYSTEM_CLOCK_64M

#pragma GCC diagnostic pop

#define __SYSTEM_CLOCK_64M      (64000000UL)

uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK_64M;

void SystemCoreClockUpdate(void);
void SystemInit(void);

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = __SYSTEM_CLOCK_64M;
}

volatile uint32_t g_chip;

void SystemInit(void) {
    g_chip = *((uint32_t *)0x10000100); // FICR->INFO.PART
    switch (g_chip) {
        case 0x52810: SystemInit_10(); break;
        case 0x52811: SystemInit_11(); break;
        case 0x52832: SystemInit_32(); break;
        case 0x52833: SystemInit_33(); break;
        case 0x52840: SystemInit_40(); break;
    }
    SystemCoreClockUpdate();
}
