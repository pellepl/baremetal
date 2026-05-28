#include "tick_timer_hal.h"
#include "stm32f3xx_ll_bus.h"
#include "stm32f3xx_ll_rcc.h"
#include "stm32f3xx_ll_tim.h"

static tick_timer_t *__tick_timer;
static volatile uint16_t __tick_timer_period;
#ifndef CONFIG_TICK_TIMER_STM32_PRESCALER
#error please define stm32 timer prescaler CONFIG_TICK_TIMER_STM32_PRESCALER (0-65535)
#endif
#if CONFIG_TICK_TIMER_STM32_HW_TIM != 2 && \
    CONFIG_TICK_TIMER_STM32_HW_TIM != 3 && \
    CONFIG_TICK_TIMER_STM32_HW_TIM != 6 && \
    CONFIG_TICK_TIMER_STM32_HW_TIM != 7
#error please define stm32 timer CONFIG_TICK_TIMER_STM32_HW_TIM (2,3,6,7)
#endif

#if CONFIG_TICK_TIMER_STM32_HW_TIM == 6 || CONFIG_TICK_TIMER_STM32_HW_TIM == 7
#define TICK_TIMER_STM32_BASIC_TIM 1
#endif

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define TIMx CAT(TIM, CONFIG_TICK_TIMER_STM32_HW_TIM)

#if CONFIG_TICK_TIMER_STM32_HW_TIM == 6
#ifdef TIM6_DAC_IRQHandler
#undef TIM6_DAC_IRQHandler
#endif
#define TIMx_IRQn TIM6_DAC_IRQn
#define TIMx_IRQHandler TIM6_DAC_IRQHandler
#elif CONFIG_TICK_TIMER_STM32_HW_TIM == 7
#ifdef TIM7_IRQHandler
#undef TIM7_IRQHandler
#endif
#define TIMx_IRQn TIM7_IRQn
#define TIMx_IRQHandler TIM7_IRQHandler
#else
#define TIMx_IRQn CAT(TIM, CAT(CONFIG_TICK_TIMER_STM32_HW_TIM, _IRQn))
#define TIMx_IRQHandler CAT(TIM, CAT(CONFIG_TICK_TIMER_STM32_HW_TIM, _IRQHandler))
#endif

static void tick_timer_hal_apply_period(uint32_t ticks) {
    if (ticks == 0) {
        ticks = 1;
    }
    __tick_timer_period = ticks;

#ifdef TICK_TIMER_STM32_BASIC_TIM
    LL_TIM_SetAutoReload(TIMx, ticks - 1U);
    LL_TIM_SetCounter(TIMx, 0);
#else
    LL_TIM_SetCounter(TIMx, ticks);
#endif
}

void tick_timer_hal_init(tick_timer_t *tim) {
    __tick_timer = tim;
    tim->hal_max_ticks = 0xffff;
    LL_APB1_GRP1_EnableClock(CAT(LL_APB1_GRP1_PERIPH_TIM, CONFIG_TICK_TIMER_STM32_HW_TIM));

#ifndef TICK_TIMER_STM32_BASIC_TIM
    LL_TIM_SetClockDivision(TIMx, LL_TIM_CLOCKDIVISION_DIV1);
    LL_TIM_SetCounterMode(TIMx, LL_TIM_COUNTERMODE_DOWN);
    LL_TIM_SetClockSource(TIMx, LL_TIM_CLOCKSOURCE_INTERNAL);
#endif
    LL_TIM_SetPrescaler(TIMx, CONFIG_TICK_TIMER_STM32_PRESCALER);
    LL_TIM_EnableUpdateEvent(TIMx);
    LL_TIM_SetUpdateSource(TIMx, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_SetOnePulseMode(TIMx, LL_TIM_ONEPULSEMODE_REPETITIVE);
    tick_timer_hal_apply_period(0xffff);
#ifdef TICK_TIMER_STM32_BASIC_TIM
    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_ClearFlag_UPDATE(TIMx);
#endif
    LL_TIM_EnableIT_UPDATE(TIMx);

    NVIC_EnableIRQ(TIMx_IRQn);
    LL_TIM_EnableCounter(TIMx);
}

void tick_timer_hal_deinit(tick_timer_t *tim) {
    (void)tim;
    LL_TIM_DisableCounter(TIMx);
    NVIC_DisableIRQ(TIMx_IRQn);
    LL_APB1_GRP1_DisableClock(CAT(LL_APB1_GRP1_PERIPH_TIM, CONFIG_TICK_TIMER_STM32_HW_TIM));
}

uint32_t tick_timer_hal_get_current(tick_timer_t *tim) {
    (void)tim;
#ifdef TICK_TIMER_STM32_BASIC_TIM
    return LL_TIM_GetCounter(TIMx);
#else
    return (uint32_t)((__tick_timer_period - LL_TIM_GetCounter(TIMx)) & 0xffff);
#endif
}

uint32_t tick_timer_hal_get_frequency(tick_timer_t *tim) {
    (void)tim;
    LL_RCC_ClocksTypeDef clocks;
    LL_RCC_GetSystemClocksFreq(&clocks);

    uint32_t tim_clk = clocks.PCLK1_Frequency;
    if (LL_RCC_GetAPB1Prescaler() != LL_RCC_APB1_DIV_1) {
        tim_clk *= 2U;
    }

    return tim_clk / (CONFIG_TICK_TIMER_STM32_PRESCALER + 1U);
}

void tick_timer_hal_set_period(tick_timer_t *tim, uint32_t ticks) {
    (void)tim;
    tick_timer_hal_apply_period(ticks);
}

void TIMx_IRQHandler(void);
void TIMx_IRQHandler(void) {
    LL_TIM_ClearFlag_UPDATE(TIMx);
    tick_timer_hal_cb_overflow(__tick_timer);
}
