#include "tick_timer.h"
#include "tick_timer_hal.h"

static inline void tick_timer_set_period(tick_timer_t *t, uint32_t ticks) {
    tick_timer_hal_set_period(t, ticks);
    t->timer_period = ticks;
}

static inline void tick_timer_decide_next_period(tick_timer_t *t) {
    if (t->next_wakeup_tick - t->cur_tick > t->hal_max_ticks) {
        tick_timer_set_period(t, t->hal_max_ticks);
    } else {
        tick_timer_set_period(t, t->next_wakeup_tick - t->cur_tick);
    }
}

static inline void tick_timer_await_splice(tick_timer_t *t) {
#   if TICK_TIMER_LEAST_VALUE_BEFORE_OP > 0
    while (TICK_TIMER_CALC_CURRENT_TICK(tick_timer_hal_get_current(t)) >= t->timer_period - TICK_TIMER_LEAST_VALUE_BEFORE_OP) {
        // something is very soon to happen, do not meddle with state now
        TICK_TIMER_AWAIT_SPLICE_INSTR;
    }
#   endif
}

void tick_timer_set_alarm(tick_timer_t *t, tick_t ticks) {
    tick_timer_await_splice(t);
    if (t->next_wakeup_tick == 0 || ticks < t->next_wakeup_tick) {
        t->cur_tick += TICK_TIMER_CALC_CURRENT_TICK(tick_timer_hal_get_current(t));
        t->next_wakeup_tick = ticks;
        t->wakeup_ticks_left = t->next_wakeup_tick - t->cur_tick;
        tick_timer_decide_next_period(t);
    } else {
        TICK_TIMER_ASSERT(0);
    }
}

void tick_timer_abort_alarm(tick_timer_t *t) {
    tick_timer_await_splice(t);
    if (t->next_wakeup_tick) {
        t->cur_tick += TICK_TIMER_CALC_CURRENT_TICK(tick_timer_hal_get_current(t));
        tick_timer_set_period(t, t->hal_max_ticks);
        t->next_wakeup_tick = 0;
    }
}

tick_t tick_timer_get_current(tick_timer_t *t) {
    return TICK_TIMER_CALC_CURRENT_TICK(tick_timer_hal_get_current(t)) + t->cur_tick;
}

tick_t tick_timer_get_alarm(tick_timer_t *t) {
    return t->next_wakeup_tick;
}

void tick_timer_hal_cb_overflow(tick_timer_t *t) {
    t->cur_tick += t->timer_period;
    if (t->next_wakeup_tick == 0) {
        tick_timer_set_period(t, t->hal_max_ticks);
    } else {
        t->wakeup_ticks_left -= t->timer_period;
        if (t->wakeup_ticks_left == 0) {
            tick_timer_set_period(t, t->hal_max_ticks);
            t->next_wakeup_tick = 0;
            tick_timer_on_alarm(t);
        } else {
            tick_timer_decide_next_period(t);
        }
    }
}

__attribute__(( weak )) void tick_timer_on_alarm(tick_timer_t *tim) {
}

void tick_timer_init(tick_timer_t *tim) {
    tim->cur_tick = tim->next_wakeup_tick = tim->wakeup_ticks_left = 0;
    tick_timer_hal_init(tim);
    tim->timer_period = tim->hal_max_ticks;
}
