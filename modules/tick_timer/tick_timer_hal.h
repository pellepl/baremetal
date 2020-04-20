#ifndef _TICK_TIMER_HAL_H_
#define _TICK_TIMER_HAL_H_

#include "tick_timer.h"

void tick_timer_hal_init(tick_timer_t *tim);
// return current timer value
uint32_t tick_timer_hal_get_current(tick_timer_t *tim);
// set timer period
void tick_timer_hal_set_period(tick_timer_t *tim, uint32_t ticks);
// call this from timer overflow irq
void tick_timer_hal_cb_overflow(tick_timer_t *tim);

#endif // _TICK_TIMER_HAL_H_