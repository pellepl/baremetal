#ifndef _TIMER_H_
#define _TIMER_H_

#include "tick_timer.h"

struct timer;

typedef void (*timer_callback_fn_t)(struct timer *timer);

typedef enum
{
    TIMER_ONESHOT = 0,
    TIMER_REPETITIVE
} timer_type_t;

typedef struct timer
{
    timer_type_t type;
    timer_callback_fn_t cb;
    tick_t t_delta;
    tick_t t_trigger;
    int started;
    void *user;
    struct timer *_next;
} timer_t;

#define TIMER_MS_TO_TICKS(x) (((x) * 65536UL) / 1000UL)

void timer_start(timer_t *timer, timer_callback_fn_t fn, tick_t delta_tick, timer_type_t type);
void timer_stop(timer_t *timer);
tick_t timer_now(void);
void timer_init(void);

#endif