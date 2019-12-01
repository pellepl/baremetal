/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "nrf.h"
#include "crystal_test.h"
#include "minio.h"
#include "cpu.h"

#define LFCLK_CYCLES    (32767)
#define SYSTICK_VALUE   (1<<20)

static int calibrated = 0;
static int32_t constant_measure_error = 0;

static uint32_t hfclk_cycles(lfclk_oscillator_t osc, uint32_t lfclk_cycles) {
    __DSB();  
    SysTick->LOAD = SYSTICK_VALUE - 1;
    SysTick->VAL = 0UL;

    // wait for selected low-frequency oscillator to start up
    NRF_CLOCK->LFCLKSRC = osc;
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);

    // setup and reset rtc
    NRF_RTC0->PRESCALER = 0;
    NRF_RTC0->TASKS_CLEAR = 1;
    while (NRF_RTC0->COUNTER != 0);
    __DSB();  

    uint32_t hfclk_cycles = 0;

    // start measuring
    cpu_interrupt_disable();
    NRF_RTC0->TASKS_START = 1;
    SysTick->CTRL = (1<<0) | (1<<2); // enable, use core clock
    while (NRF_RTC0->COUNTER < lfclk_cycles) {
        if (SysTick->CTRL & (1<<16)) {
            // systick overflow
            hfclk_cycles += SYSTICK_VALUE;
        }
    }
    uint32_t systick_stop_val = SysTick->VAL;
    uint32_t rest = (SYSTICK_VALUE - 1) - systick_stop_val;
    hfclk_cycles += rest;
    SysTick->CTRL = 0; // disable sysclk
    NRF_RTC0->TASKS_STOP = 1; // disable rtc0
    NRF_CLOCK->TASKS_LFCLKSTOP = 1; // disable lfclk
    __DSB();  
    cpu_interrupt_enable();

    return hfclk_cycles;
}

int crystal_test(lfclk_oscillator_t osc) {
    if (!calibrated) {
        printf("Calibrating...\n");
        // wait for the external oscillator to start up
        NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
        NRF_CLOCK->TASKS_HFCLKSTART = 1;
        while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

        // warm up, tune in, or some other hw thing
        (void)hfclk_cycles(LFCLK_OSC_SYNTH, 100);
        // calculate constant error
        uint32_t synthetisized_lfclk = hfclk_cycles(LFCLK_OSC_SYNTH, LFCLK_CYCLES);
        constant_measure_error = cpu_core_clock_freq() - synthetisized_lfclk;
        printf("Calibration done [constant measure error:%d]\n", constant_measure_error);
        calibrated = 1;
    }

    // warm up, tune in, or some other hw thing
    (void)hfclk_cycles(osc, 100);

    uint32_t hfclk = hfclk_cycles(osc, LFCLK_CYCLES);
    uint32_t hfclk_freq = cpu_core_clock_freq();
    int32_t delta = hfclk_freq - hfclk;
    delta -= constant_measure_error;
    int32_t hfclk_mhz = hfclk_freq / 1000000UL;
    int32_t ppm = (delta + (delta > 0 ? hfclk_mhz : -hfclk_mhz)/2) / hfclk_mhz;
    printf("Acq:%d (%+d constant error)  Exp:%d  Delta:%d\nDeviation %+dppm\n", 
        hfclk, constant_measure_error, hfclk_freq, delta, ppm);

    return 0;
}