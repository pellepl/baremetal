#include "bmtypes.h"
#include _CORTEX_CORE_HEADER
#include "cpu.h"

__attribute__((weak)) void cpu_init(void) {}

__attribute__((weak, noreturn)) void cpu_reset(void)
{
    NVIC_SystemReset();
    while (1)
        ;
}

#if CONFIG_STM32F1_HALT_USING_SYSCLK

__attribute__((weak)) void cpu_halt(uint32_t milliseconds) {
    uint32_t ticks_per_ms = cpu_core_clock_freq() / 1000;
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

#else

static inline void delay_cycles(uint32_t cycles)
{
    uint32_t end = DWT->CYCCNT + cycles;
    while ((int32_t)(DWT->CYCCNT - end) < 0);
}

__attribute__((weak)) void cpu_halt(uint32_t ms)
{
    static int cyccnt_init = 0;
    if (!cyccnt_init)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)
        {
            DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        }
        cyccnt_init = 1;
    }
    const uint32_t cpm = cpu_core_clock_freq() / 1000u;
    while (ms--) {
        delay_cycles(cpm);
    }
}

__attribute__((weak)) void cpu_halt_us(uint32_t us)
{
    static int cyccnt_init = 0;
    if (!cyccnt_init)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)
            DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        cyccnt_init = 1;
    }

    const uint32_t cpu_hz = cpu_core_clock_freq();
    const uint32_t cycles_per_us = cpu_hz / 1000000u;
    if (!cycles_per_us) return;

    uint64_t total = (uint64_t)us * (uint64_t)cycles_per_us;
    // Limit each wait to <= 0x7FFFFFFF cycles so wrap logic stays valid
    while (total) {
        uint32_t chunk = (total > 0x7FFFFFFFULL) ? 0x7FFFFFFFu : (uint32_t)total;
        delay_cycles(chunk);
        total -= chunk;
    }
}
#endif

__attribute__((weak)) void cpu_interrupt_enable(void) {
    __enable_irq();
}

__attribute__((weak)) void cpu_interrupt_disable(void)
{
    __disable_irq();
}

__attribute__((weak)) uint32_t cpu_core_clock_freq(void)
{
    SystemCoreClockUpdate();
    return SystemCoreClock;
}
