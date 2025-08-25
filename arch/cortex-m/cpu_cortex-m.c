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
#endif

__attribute__((weak)) void cpu_interrupt_enable(void)
{
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
