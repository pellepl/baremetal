/* Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "stm32f3xx.h"
#include "cpu.h"
#include "stm32f3xx_ll_rcc.h"
#include "stm32f3xx_ll_system.h"
#include "stm32f3xx_ll_utils.h"

#if !defined(NO_STM32_HSE_PLL) && !defined(NO_STM32F3_CPU_INIT)
void cpu_init(void) {
#if HSE_VALUE == 8000000
#ifndef STM32F3_RCC_SYSCLK_DIV
#define STM32F3_RCC_SYSCLK_DIV LL_RCC_SYSCLK_DIV_1
#endif
#ifndef STM32F3_RCC_APB1_DIV
#define STM32F3_RCC_APB1_DIV LL_RCC_APB1_DIV_2
#endif
#ifndef STM32F3_RCC_APB2_DIV
#define STM32F3_RCC_APB2_DIV LL_RCC_APB2_DIV_1
#endif
#ifndef STM32F3_FLASH_LATENCY
#define STM32F3_FLASH_LATENCY LL_FLASH_LATENCY_2
#endif
#ifndef STM32F3_RCC_PLL_CLKSRC_DIV
#define STM32F3_RCC_PLL_CLKSRC_DIV LL_RCC_PLLSOURCE_HSE_DIV_1
#endif
#ifndef STM32F3_RCC_PLL_MUL
#define STM32F3_RCC_PLL_MUL LL_RCC_PLL_MUL_9
#endif
#else
#ifndef STM32F3_RCC_SYSCLK_DIV
#error Define STM32F3_RCC_SYSCLK_DIV for given HSE_VALUE
#endif
#ifndef STM32F3_RCC_APB1_DIV
#error Define STM32F3_RCC_APB1_DIV for given HSE_VALUE
#endif
#ifndef STM32F3_RCC_APB2_DIV
#error Define STM32F3_RCC_APB2_DIV for given HSE_VALUE
#endif
#ifndef STM32F3_FLASH_LATENCY
#error Define STM32F3_FLASH_LATENCY for given HSE_VALUE
#endif
#ifndef STM32F3_RCC_PLL_CLKSRC_DIV
#error Define STM32F3_RCC_PLL_CLKSRC_DIV for given HSE_VALUE
#endif
#ifndef STM32F3_RCC_PLL_MUL
#error Define STM32F3_RCC_PLL_MUL for given HSE_VALUE
#endif
#endif

    LL_RCC_HSE_Enable();
    while (LL_RCC_HSE_IsReady() == 0);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);

    LL_RCC_SetAHBPrescaler(STM32F3_RCC_SYSCLK_DIV);
    LL_RCC_SetAPB1Prescaler(STM32F3_RCC_APB1_DIV);
    LL_RCC_SetAPB2Prescaler(STM32F3_RCC_APB2_DIV);

    LL_FLASH_SetLatency(STM32F3_FLASH_LATENCY);
    LL_RCC_PLL_ConfigDomain_SYS(STM32F3_RCC_PLL_CLKSRC_DIV, STM32F3_RCC_PLL_MUL);
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() == 0);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

    LL_SetSystemCoreClock(72000000);
}
#endif
