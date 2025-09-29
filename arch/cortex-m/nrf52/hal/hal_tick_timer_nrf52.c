// tick_timer_hal_nrf52.c
#include "nrf.h"
#include "tick_timer_hal.h"

#ifndef CONFIG_TICK_TIMER_NRF52_PRESCALER
#error "Define CONFIG_TICK_TIMER_NRF52_PRESCALER (0..9). Tick = 16MHz / 2^PRESCALER"
#endif

#if !defined(CONFIG_TICK_TIMER_NRF52_HW_TIMER) || (CONFIG_TICK_TIMER_NRF52_HW_TIMER < 0) || (CONFIG_TICK_TIMER_NRF52_HW_TIMER > 4)
#error "Define CONFIG_TICK_TIMER_NRF52_HW_TIMER as 0..4 (TIMER0..TIMER4)"
#endif

#define CAT(a, b) a##b
#define XCAT(a, b) CAT(a, b)

#define NRF_TIMERx XCAT(NRF_TIMER, CONFIG_TICK_TIMER_NRF52_HW_TIMER)
#define TIMERx_IRQn XCAT(TIMER, XCAT(CONFIG_TICK_TIMER_NRF52_HW_TIMER, _IRQn))
#define TIMERx_IRQHandler XCAT(TIMER, XCAT(CONFIG_TICK_TIMER_NRF52_HW_TIMER, _IRQHandler))

static tick_timer_t *__tick_timer;
static volatile uint16_t __tick_timer_period;

void tick_timer_hal_init(tick_timer_t *tim)
{
    __tick_timer = tim;
    tim->hal_max_ticks = 0xffff;

    NRF_TIMERx->TASKS_STOP = 1;
    NRF_TIMERx->TASKS_CLEAR = 1;

    NRF_TIMERx->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMERx->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
    NRF_TIMERx->PRESCALER = CONFIG_TICK_TIMER_NRF52_PRESCALER;
    NRF_TIMERx->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;

    NRF_TIMERx->CC[0] = 0xffff;
    __tick_timer_period = 0xffff;

    NRF_TIMERx->EVENTS_COMPARE[0] = 0;
    NRF_TIMERx->INTENCLR = 0xffffffff;
    NRF_TIMERx->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    NVIC_ClearPendingIRQ(TIMERx_IRQn);
    NVIC_EnableIRQ(TIMERx_IRQn);

    NRF_TIMERx->TASKS_START = 1;
}

void tick_timer_hal_deinit(tick_timer_t *tim)
{
    NVIC_DisableIRQ(TIMERx_IRQn);
    NVIC_ClearPendingIRQ(TIMERx_IRQn);
    NRF_TIMERx->TASKS_STOP = 1;
    NRF_TIMERx->TASKS_CLEAR = 1;
    NRF_TIMERx->INTENCLR = 0xffffffff;
}

uint32_t tick_timer_hal_get_current(tick_timer_t *tim)
{
    (void)tim;
    NRF_TIMERx->TASKS_CAPTURE[1] = 1;
    uint32_t now = NRF_TIMERx->CC[1] & 0xffff;
    return now;
}

void tick_timer_hal_set_period(tick_timer_t *tim, uint32_t ticks)
{
    (void)tim;
    NRF_TIMERx->TASKS_STOP = 1;
    NRF_TIMERx->EVENTS_COMPARE[0] = 0;
    NRF_TIMERx->CC[0] = (uint16_t)ticks;
    __tick_timer_period = (uint16_t)ticks;
    NRF_TIMERx->TASKS_CLEAR = 1;
    NRF_TIMERx->TASKS_START = 1;
}

void TIMERx_IRQHandler(void);
void TIMERx_IRQHandler(void)
{
    if (NRF_TIMERx->EVENTS_COMPARE[0])
    {
        NRF_TIMERx->EVENTS_COMPARE[0] = 0;
        tick_timer_hal_cb_overflow(__tick_timer);
    }
}
