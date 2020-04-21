#ifndef _TICK_TIMER_HAL_H_
#define _TICK_TIMER_HAL_H_

#include "tick_timer.h"

/**
 * Called during initialization. Must set the
 * tick_timer_t->hal_max_ticks value.
 * @param tim the tick timer struct
 */
void tick_timer_hal_init(tick_timer_t *tim);

/**
 * Returns current raw timer value.
 * @param tim the tick timer struct
 * @return the timer value
 */
uint32_t tick_timer_hal_get_current(tick_timer_t *tim);

/**
 * Sets the timer period.
 * @param tim the tick timer struct
 * @param ticks 
 */
void tick_timer_hal_set_period(tick_timer_t *tim, uint32_t ticks);

/**
 * This is to be called from the timers overflow IRQ.
 * Considered to be in IRQ context.
 * @param tim the tick timer struct
 */
void tick_timer_hal_cb_overflow(tick_timer_t *tim);

#endif // _TICK_TIMER_HAL_H_
