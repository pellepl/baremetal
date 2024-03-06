#include "bmtypes.h"
#include _CORTEX_CORE_HEADER
#include "cpu.h"

__attribute__((weak)) void cpu_init(void) {}

__attribute__((weak, noreturn)) void cpu_reset(void) {
    NVIC_SystemReset();
    while(1);
}

__attribute__((weak)) void cpu_halt(uint32_t milliseconds) {
    uint32_t ticks_per_ms = cpu_core_clock_freq() / 1000;
    ticks_per_ms -= 4; // remove 1+3 ticks for loop: subs, bne
    SysTick->LOAD = ticks_per_ms - 1;
    SysTick->VAL = 0UL;
    SysTick->CTRL = (1<<0) | (1<<2); // enable, use core clock
    while (milliseconds--) {
        while ((SysTick->CTRL & (1<<16)) == 0); // wait till overflow
    }
    SysTick->CTRL = 0; // disable
}

__attribute__((weak)) void cpu_halt_us(uint32_t microseconds)
{
    if (microseconds == 0)
        return;
    const uint32_t ticks_per_us = cpu_core_clock_freq() / 1000000;
    if (microseconds / ticks_per_us > (1 << 23)) // SysTick is 24 bits  by definition
    {
        cpu_halt(microseconds / 1000);
    }
    else
    {
        SysTick->LOAD = microseconds * ticks_per_us - 1;
        SysTick->VAL = 0UL;
        SysTick->CTRL = (1 << 0) | (1 << 2); // enable, use core clock
        while ((SysTick->CTRL & (1 << 16)) == 0)
            ;              // wait till overflow
        SysTick->CTRL = 0; // disable
    }
}

__attribute__((weak)) void cpu_interrupt_enable(void) {
    __enable_irq();
}

__attribute__((weak)) void cpu_interrupt_disable(void) {
    __disable_irq();
}

__attribute__((weak)) uint32_t cpu_core_clock_freq(void) {
    SystemCoreClockUpdate();
    return SystemCoreClock;
}
