/* Copyright (c) 2025 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "stm32f0xx.h"
#include "cpu.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"

#if !defined(NO_STM32_HSE_PLL) && !defined(NO_STM32F0_HSE_PLL)
void cpu_init(void)
{
#if HSE_VALUE == 8000000
/*
 * The system Clock is configured as follow :
 *    System Clock source            = MSI
 *    SYSCLK(Hz)                     = 2097000
 *    HCLK(Hz)                       = 2097000
 *    AHB Prescaler                  = 1
 *    APB1 Prescaler                 = 1
 *    APB2 Prescaler                 = 1
 *    Flash Latency(WS)              = 0
 *    Main regulator output voltage  = Scale3 mode
 */
#ifndef STM32L0_FLASH_LATENCY
#define STM32L0_FLASH_LATENCY LL_FLASH_LATENCY_1
#endif
#ifndef STM32L0_MSI_RANGE
#define STM32L0_MSI_RANGE LL_RCC_MSIRANGE_5
#endif
#ifndef STM32L0_RCC_AHB_DIV
#define STM32L0_RCC_AHB_DIV    LL_RCC_SYSCLK_DIV_1
#endif
#ifndef STM32L0_RCC_APB1_DIV
#define STM32L0_RCC_APB1_DIV    LL_RCC_APB1_DIV_1
#endif
#ifndef STM32L0_RCC_APB2_DIV
#define STM32L0_RCC_APB2_DIV    LL_RCC_APB2_DIV_1
#endif

#else
#ifndef STM32L0_FLASH_LATENCY
#error Define STM32L0_FLASH_LATENCY
#endif
#ifndef STM32L0_MSI_RANGE
#error Define STM32L0_MSI_RANGE
#endif
#ifndef STM32L0_RCC_AHB_DIV
#error Define STM32L0_RCC_AHB_DIV
#endif
#ifndef STM32L0_RCC_APB1_DIV
#error Define STM32L0_RCC_APB1_DIV
#endif
#ifndef STM32L0_RCC_APB2_DIV
#error Define STM32L0_RCC_APB2_DIV
#endif

#endif
    // MSI configuration and activation
    LL_RCC_PLL_Disable();
    // Set new latency
    LL_FLASH_SetLatency(STM32L0_FLASH_LATENCY);

    LL_RCC_MSI_Enable();
    while (LL_RCC_MSI_IsReady() != 1)
    {
    };
    LL_RCC_MSI_SetRange(STM32L0_MSI_RANGE);
    LL_RCC_MSI_SetCalibTrimming(0x0);

    // Sysclk activation
    LL_RCC_SetAHBPrescaler(STM32L0_RCC_AHB_DIV);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
    {
    };

    // Set APB1 & APB2 prescaler
    LL_RCC_SetAPB1Prescaler(STM32L0_RCC_APB1_DIV);
    LL_RCC_SetAPB2Prescaler(STM32L0_RCC_APB2_DIV);

    // Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function)
    LL_SetSystemCoreClock(2097000);

    // Enable Power Control clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    // The voltage scaling allows optimizing the power consumption when the device is
    // clocked below the maximum system frequency, to update the voltage scaling value
    // regarding system frequency refer to product datasheet.
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE3);
}
#endif // NO_STM32_HSE_PLL
