/* Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

/*
 * CONFIG_TICK_TIMER_STM32_HW_TIM selects an exclusively owned timer:
 * L053: 2, 6, 21, 22. L073: also 3 and 7.
 * CONFIG_TICK_TIMER_STM32_PRESCALER is the PSC register value (divider - 1).
 * TIM6 owns TIM6_DAC_IRQHandler; applications using DAC IRQs must multiplex it.
 */

#include "tick_timer_hal.h"
#include "stm32l0xx_ll_bus.h"
#include "stm32l0xx_ll_rcc.h"
#include "stm32l0xx_ll_tim.h"

#ifndef CONFIG_TICK_TIMER_STM32_PRESCALER
#error Define CONFIG_TICK_TIMER_STM32_PRESCALER (0-65535)
#elif CONFIG_TICK_TIMER_STM32_PRESCALER < 0 || CONFIG_TICK_TIMER_STM32_PRESCALER > 65535
#error CONFIG_TICK_TIMER_STM32_PRESCALER must be in the range 0-65535
#endif

#if (CONFIG_TICK_TIMER_STM32_HW_TIM == 2 && defined(TIM2)) || \
    (CONFIG_TICK_TIMER_STM32_HW_TIM == 3 && defined(TIM3)) || \
    (CONFIG_TICK_TIMER_STM32_HW_TIM == 6 && defined(TIM6)) || \
    (CONFIG_TICK_TIMER_STM32_HW_TIM == 7 && defined(TIM7)) || \
    (CONFIG_TICK_TIMER_STM32_HW_TIM == 21 && defined(TIM21)) || \
    (CONFIG_TICK_TIMER_STM32_HW_TIM == 22 && defined(TIM22))
/* Timer is available on the selected device. */
#else
#error Select an available timer with CONFIG_TICK_TIMER_STM32_HW_TIM (2,3,6,7,21,22)
#endif

#define CAT_IMPL(a, b) a ## b
#define CAT(a, b) CAT_IMPL(a, b)
#define TIMx CAT(TIM, CONFIG_TICK_TIMER_STM32_HW_TIM)

#if CONFIG_TICK_TIMER_STM32_HW_TIM == 21 || CONFIG_TICK_TIMER_STM32_HW_TIM == 22
#define TIMx_PERIPH CAT(LL_APB2_GRP1_PERIPH_TIM, CONFIG_TICK_TIMER_STM32_HW_TIM)
#define TIMx_CLOCK_ENABLE LL_APB2_GRP1_EnableClock
#define TIMx_CLOCK_DISABLE LL_APB2_GRP1_DisableClock
#define TIMx_FORCE_RESET LL_APB2_GRP1_ForceReset
#define TIMx_RELEASE_RESET LL_APB2_GRP1_ReleaseReset
#else
#define TIMx_PERIPH CAT(LL_APB1_GRP1_PERIPH_TIM, CONFIG_TICK_TIMER_STM32_HW_TIM)
#define TIMx_CLOCK_ENABLE LL_APB1_GRP1_EnableClock
#define TIMx_CLOCK_DISABLE LL_APB1_GRP1_DisableClock
#define TIMx_FORCE_RESET LL_APB1_GRP1_ForceReset
#define TIMx_RELEASE_RESET LL_APB1_GRP1_ReleaseReset
#endif

#if CONFIG_TICK_TIMER_STM32_HW_TIM == 6
#define TIMx_IRQn TIM6_DAC_IRQn
#define TIMx_IRQHandler TIM6_DAC_IRQHandler
#else
#define TIMx_IRQn CAT(TIM, CAT(CONFIG_TICK_TIMER_STM32_HW_TIM, _IRQn))
#define TIMx_IRQHandler CAT(TIM, CAT(CONFIG_TICK_TIMER_STM32_HW_TIM, _IRQHandler))
#endif

static tick_timer_t *tick_timer;

static void apply_period(uint32_t ticks)
{
    TICK_TIMER_ASSERT(ticks > 0 && ticks <= UINT16_MAX);
    if (ticks == 0)
        ticks = 1;
    else if (ticks > UINT16_MAX)
        ticks = UINT16_MAX;

    LL_TIM_DisableCounter(TIMx);
    LL_TIM_SetAutoReload(TIMx, ticks - 1U);
    // UG loads PSC and resets its phase as well as the up-counter. With URS
    // set to COUNTER this does not create an overflow interrupt.
    LL_TIM_GenerateEvent_UPDATE(TIMx);
    LL_TIM_SetCounter(TIMx, 0);
    LL_TIM_ClearFlag_UPDATE(TIMx);
    NVIC_ClearPendingIRQ(TIMx_IRQn);
    LL_TIM_EnableCounter(TIMx);
}

void tick_timer_hal_init(tick_timer_t *tim)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    tick_timer = tim;
    tim->hal_max_ticks = UINT16_MAX;

    NVIC_DisableIRQ(TIMx_IRQn);
    TIMx_CLOCK_ENABLE(TIMx_PERIPH);
    TIMx_FORCE_RESET(TIMx_PERIPH);
    TIMx_RELEASE_RESET(TIMx_PERIPH);
    // Reset selects internal-clock up-counting on both basic and general timers.
    LL_TIM_SetPrescaler(TIMx, CONFIG_TICK_TIMER_STM32_PRESCALER);
    LL_TIM_DisableARRPreload(TIMx);
    LL_TIM_EnableUpdateEvent(TIMx);
    LL_TIM_SetUpdateSource(TIMx, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_SetOnePulseMode(TIMx, LL_TIM_ONEPULSEMODE_REPETITIVE);
    apply_period(tim->hal_max_ticks);
    LL_TIM_EnableIT_UPDATE(TIMx);
    NVIC_EnableIRQ(TIMx_IRQn);
    __set_PRIMASK(primask);
}

void tick_timer_hal_deinit(tick_timer_t *tim)
{
    (void)tim;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    LL_TIM_DisableIT_UPDATE(TIMx);
    LL_TIM_DisableCounter(TIMx);
    NVIC_DisableIRQ(TIMx_IRQn);
    LL_TIM_ClearFlag_UPDATE(TIMx);
    NVIC_ClearPendingIRQ(TIMx_IRQn);
    TIMx_CLOCK_DISABLE(TIMx_PERIPH);
    tick_timer = 0;
    __set_PRIMASK(primask);
}

uint32_t tick_timer_hal_get_current(tick_timer_t *tim)
{
    (void)tim;
    return LL_TIM_GetCounter(TIMx);
}

uint32_t tick_timer_hal_get_frequency(tick_timer_t *tim)
{
    (void)tim;
    LL_RCC_ClocksTypeDef clocks;
    LL_RCC_GetSystemClocksFreq(&clocks);
#if CONFIG_TICK_TIMER_STM32_HW_TIM == 21 || CONFIG_TICK_TIMER_STM32_HW_TIM == 22
    uint32_t frequency = clocks.PCLK2_Frequency;
    if (LL_RCC_GetAPB2Prescaler() != LL_RCC_APB2_DIV_1)
        frequency *= 2U;
#else
    uint32_t frequency = clocks.PCLK1_Frequency;
    if (LL_RCC_GetAPB1Prescaler() != LL_RCC_APB1_DIV_1)
        frequency *= 2U;
#endif
    return frequency / (CONFIG_TICK_TIMER_STM32_PRESCALER + 1U);
}

void tick_timer_hal_set_period(tick_timer_t *tim, uint32_t ticks)
{
    (void)tim;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    apply_period(ticks);
    __set_PRIMASK(primask);
}

void TIMx_IRQHandler(void);
void TIMx_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIMx) && LL_TIM_IsEnabledIT_UPDATE(TIMx)) {
        LL_TIM_ClearFlag_UPDATE(TIMx);
        tick_timer_hal_cb_overflow(tick_timer);
    }
}
