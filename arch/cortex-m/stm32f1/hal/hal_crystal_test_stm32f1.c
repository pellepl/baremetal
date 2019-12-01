/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "stm32f1xx.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_rcc.h"
#include "stm32f1xx_ll_rtc.h"
#include "stm32f1xx_ll_pwr.h"
#include "crystal_test.h"
#include "minio.h"
#include "cpu.h"

#define LFCLK_CYCLES    (32768)
#define SYSTICK_VALUE   (1<<20)

static int calibrated = 0;
static int32_t constant_measure_error = 0;

static uint32_t hfclk_cycles(lfclk_oscillator_t osc, uint32_t lfclk_cycles) {
    // while (LL_RTC_IsActiveFlag_RS(RTC) != 0);

    SysTick->CTRL = 0; // systick disable
    __DSB();  
    SysTick->LOAD = SYSTICK_VALUE - 1;
    SysTick->VAL = 0UL;

    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();

    // wait for selected low-frequency oscillator to start up
    if (osc == LFCLK_OSC_RC) {
        LL_RCC_LSE_Disable();
        LL_RCC_LSI_Enable();
        while (LL_RCC_LSI_IsReady() == 0);
    } else if (osc == LFCLK_OSC_XTAL) {
        LL_RCC_LSE_Enable();
        LL_RCC_LSI_Disable();
        while (LL_RCC_LSE_IsReady() == 0);
    } else {
        LL_RCC_LSE_Disable();
        LL_RCC_LSI_Disable();
        lfclk_cycles = ((HSE_VALUE/128)*lfclk_cycles) / 32768;
    }

    // setup and reset rtc
    if (osc == LFCLK_OSC_RC) {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
    } else if (osc == LFCLK_OSC_XTAL) {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    } else {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_HSE_DIV128);
    }

    uint32_t hfclk_cycles = 0;

    LL_RCC_EnableRTC();
    __DSB();  
    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);

    LL_RTC_DisableWriteProtection(RTC);


    cpu_interrupt_disable();
    LL_RTC_ClearFlag_RS(RTC);
    LL_RTC_SetAsynchPrescaler(RTC, 0xfffff);
    LL_RTC_TIME_Set(RTC, 0);
    
    LL_RTC_EnableWriteProtection(RTC);

    while (LL_RTC_IsActiveFlag_RTOF(RTC) == 0);

    // start measuring
    SysTick->CTRL = (1<<0) | (1<<2); // enable, use core clock
    __DSB();  
    while (LL_RTC_GetDivider(RTC) > 0xfffff - lfclk_cycles) {
        if (SysTick->CTRL & (1<<16)) {
            // systick overflow
            hfclk_cycles += SYSTICK_VALUE;
            //printf("%d %08x\n", LL_RTC_TIME_Get(RTC), LL_RTC_GetDivider(RTC));
         }
    }
    uint32_t systick_stop_val = SysTick->VAL;
    uint32_t rest = (SYSTICK_VALUE - 1) - systick_stop_val;
    hfclk_cycles += rest;
    SysTick->CTRL = 0; // disable sysclk
    LL_RCC_DisableRTC();
    __DSB();  

    cpu_interrupt_enable();
    return hfclk_cycles;
}

int crystal_test(lfclk_oscillator_t osc) {
    if (LL_RCC_HSE_IsReady() == 0) {
        printf("External high frequency crystal not synced\n");
        return -1;
    }

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR | LL_APB1_GRP1_PERIPH_BKP);

    LL_PWR_EnableBkUpAccess();

    if (!calibrated) {
        printf("Calibrating...\n");

        // calculate constant error
        uint32_t synthetisized_lfclk = hfclk_cycles(LFCLK_OSC_SYNTH, LFCLK_CYCLES);
        constant_measure_error = cpu_core_clock_freq() - synthetisized_lfclk;
        printf("Calibration done [constant measure error:%d]\n", constant_measure_error);
        calibrated = 1;
    }

    uint32_t hfclk = hfclk_cycles(osc, LFCLK_CYCLES);
    
    LL_PWR_DisableBkUpAccess();

    uint32_t hfclk_freq = cpu_core_clock_freq();
    int32_t delta = hfclk_freq - hfclk;
    delta -= constant_measure_error;
    int32_t hfclk_mhz = hfclk_freq / 1000000UL;
    int32_t ppm = (delta + (delta > 0 ? hfclk_mhz : -hfclk_mhz)/2) / hfclk_mhz;
    printf("Acq:%d (%+d constant error)  Exp:%d  Delta:%d\nDeviation %+dppm\n", 
        hfclk, constant_measure_error, hfclk_freq, delta, ppm);


    return 0;
}