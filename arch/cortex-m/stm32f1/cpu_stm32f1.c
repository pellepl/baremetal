/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "stm32f1xx.h"
#include "cpu.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_system.h"

#if !defined(NO_STM32_HSE_PLL) && !defined(NO_STM32F1_HSE_PLL)
void cpu_init(void) {
#if HSE_VALUE==8000000
// for going from 8MHz crystal to 72MHz PLL
#ifndef STM32F1_RCC_SYSCLK_DIV
#define STM32F1_RCC_SYSCLK_DIV  LL_RCC_SYSCLK_DIV_1
#endif
#ifndef STM32F1_RCC_APB1_DIV
#define STM32F1_RCC_APB1_DIV    LL_RCC_APB1_DIV_2
#endif
#ifndef STM32F1_RCC_APB2_DIV
#define STM32F1_RCC_APB2_DIV    LL_RCC_APB2_DIV_1
#endif
#ifndef STM32F1_RCC_ADC_CLKSRC_DIV
#define STM32F1_RCC_ADC_CLKSRC_DIV  LL_RCC_ADC_CLKSRC_PCLK2_DIV_8
#endif
#ifndef STM32F1_FLASH_LATENCY
#define STM32F1_FLASH_LATENCY   LL_FLASH_LATENCY_2
#endif
#ifndef STM32F1_RCC_PLL_CLKSRC_DIV
#define STM32F1_RCC_PLL_CLKSRC_DIV  LL_RCC_PLLSOURCE_HSE_DIV_1
#endif
#ifndef STM32F1_RCC_PLL_MUL
#define STM32F1_RCC_PLL_MUL     LL_RCC_PLL_MUL_9
#endif
#else
#ifndef STM32F1_RCC_SYSCLK_DIV
#error Define STM32F1_RCC_SYSCLK_DIV for given HSE_VALUE
#endif
#ifndef STM32F1_RCC_APB1_DIV
#error Define STM32F1_RCC_APB1_DIV for given HSE_VALUE
#endif
#ifndef STM32F1_RCC_APB2_DIV
#error Define STM32F1_RCC_APB2_DIV for given HSE_VALUE
#endif
#ifndef STM32F1_RCC_ADC_CLKSRC_DIV
#error Define STM32F1_RCC_ADC_CLKSRC_DIV for given HSE_VALUE
#endif
#ifndef STM32F1_FLASH_LATENCY
#error Define STM32F1_FLASH_LATENCY for given HSE_VALUE
#endif
#ifndef STM32F1_RCC_PLL_CLKSRC_DIV
#error Define STM32F1_RCC_PLL_CLKSRC_DIV for given HSE_VALUE
#endif
#ifndef STM32F1_RCC_PLL_MUL
#error Define STM32F1_RCC_PLL_MUL for given HSE_VALUE
#endif
#endif

    // enable HSE and wait for it
    LL_RCC_HSE_Enable();
    while (LL_RCC_HSE_IsReady() == 0);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);

    // set up prescalers for AHB, APB1-2 and ADC
    LL_RCC_SetAHBPrescaler(STM32F1_RCC_SYSCLK_DIV);
    LL_RCC_SetAPB1Prescaler(STM32F1_RCC_APB1_DIV);          // max 36Mhz
    LL_RCC_SetAPB2Prescaler(STM32F1_RCC_APB2_DIV);          // max 72Mhz
    LL_RCC_SetADCClockSource(STM32F1_RCC_ADC_CLKSRC_DIV);   // max 14Mhz

    // 0 -24MHz : 0 latency
    // 24-48MHz : 1 latency
    // 48-72MHz : 2 latency
    LL_FLASH_SetLatency(STM32F1_FLASH_LATENCY);

    // set PLL = (HSE / <divisor>) * <multiplier>
    LL_RCC_PLL_ConfigDomain_SYS(STM32F1_RCC_PLL_CLKSRC_DIV, STM32F1_RCC_PLL_MUL);

    // enable PLL and wait for lock
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() == 0);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
}
#endif // NO_STM32_HSE_LOCK
