#include "tick_timer_hal.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_tim.h"

static tick_timer_t *__tick_timer;
static volatile uint16_t __tick_timer_period;
#ifndef CONFIG_TICK_TIMER_STM32_PRESCALER
#error please define stm32 timer prescaler CONFIG_TICK_TIMER_STM32_PRESCALER (0-65535)
#endif
#if CONFIG_TICK_TIMER_STM32_HW_TIM < 2 || CONFIG_TICK_TIMER_STM32_HW_TIM > 4
#error please define stm32 timer CONFIG_TICK_TIMER_STM32_HW_TIM (2,3,4) 
#endif

#define CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define TIMx CAT(TIM, CONFIG_TICK_TIMER_STM32_HW_TIM)

void tick_timer_hal_init(tick_timer_t *tim) {
    __tick_timer = tim;
    tim->hal_max_ticks = 0xffff;
    LL_APB1_GRP1_EnableClock(CAT(LL_APB1_GRP1_PERIPH_TIM, CONFIG_TICK_TIMER_STM32_HW_TIM));

    LL_TIM_SetClockDivision(TIMx, LL_TIM_CLOCKDIVISION_DIV1);
    LL_TIM_SetCounterMode(TIMx, LL_TIM_COUNTERMODE_DOWN);
    LL_TIM_SetPrescaler(TIMx, CONFIG_TICK_TIMER_STM32_PRESCALER);
    LL_TIM_SetClockSource(TIMx, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_EnableIT_UPDATE(TIMx);
    LL_TIM_EnableUpdateEvent(TIMx);
    LL_TIM_SetUpdateSource(TIMx, LL_TIM_UPDATESOURCE_COUNTER);
    LL_TIM_SetOnePulseMode(TIMx, LL_TIM_ONEPULSEMODE_REPETITIVE);

    NVIC_EnableIRQ(CAT(TIM, CAT(CONFIG_TICK_TIMER_STM32_HW_TIM, _IRQn)));
    LL_TIM_SetCounter(TIMx, 0xffff);
    __tick_timer_period = 0xffff;
    LL_TIM_EnableCounter(TIMx);
}

uint32_t tick_timer_hal_get_current(tick_timer_t *tim) {
    return (uint32_t)((__tick_timer_period -  LL_TIM_GetCounter(TIMx)) & 0xffff);
}

void tick_timer_hal_set_period(tick_timer_t *tim, uint32_t ticks) {
    LL_TIM_SetCounter(TIMx, ticks);
    __tick_timer_period = ticks;
}

void CAT(TIM, CAT(CONFIG_TICK_TIMER_STM32_HW_TIM, _IRQHandler))(void);
void CAT(TIM, CAT(CONFIG_TICK_TIMER_STM32_HW_TIM, _IRQHandler))(void) {
    LL_TIM_ClearFlag_UPDATE(TIMx);
    tick_timer_hal_cb_overflow(__tick_timer);
}
