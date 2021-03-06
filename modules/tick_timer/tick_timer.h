#ifndef _TICK_TIMER_H_
#define _TICK_TIMER_H_

#include "bmtypes.h"

// do not do any operations if timer is lower than this value
#ifndef TICK_TIMER_LEAST_VALUE_BEFORE_OP
#define TICK_TIMER_LEAST_VALUE_BEFORE_OP   (16)
#endif

// while waiting for TIMER_LEAST_VALUE_BEFORE_OP, execute this instruction
#ifndef TICK_TIMER_AWAIT_SPLICE_INSTR
#define TICK_TIMER_AWAIT_SPLICE_INSTR
#endif

// tick type definition
#ifndef TICK_TIMER_TYPE
#define TICK_TIMER_TYPE uint64_t
#endif

typedef TICK_TIMER_TYPE tick_t;

#ifndef TICK_TIMER_CALC_CURRENT_TICK
#define TICK_TIMER_CALC_CURRENT_TICK(x)    (x)
#endif // TICK_TIMER_CALC_CURRENT_TICK

#ifndef TICK_TIMER_ASSERT
#define TICK_TIMER_ASSERT(x)
#endif // TICK_TIMER_ASSERT

typedef struct {
    tick_t wakeup_ticks_left;
    uint32_t timer_period;
    volatile tick_t cur_tick;
    tick_t next_wakeup_tick;
    uint32_t hal_max_ticks;
    void *user;
} tick_timer_t;

/**
 * Initiate the tick timer framework.
 * @param tim the tick timer struct
 */
void tick_timer_init(tick_timer_t *tim);

/**
 * Sets the an alarm at the given absolute tick.
 * The alarm will only be set if there is no untriggered alarm set previously,
 * or if the new alarm is earlier than the current alarm. In the latter case,
 * the later alarm will be aborted.
 * @param tick the alarm time
 */
void tick_timer_set_alarm(tick_timer_t *tim, tick_t tick);

/** 
 * Aborts current untriggered alarm. If there is no such alarm,
 * nothing happens.
 * @param tim the tick timer struct
 */
void tick_timer_abort_alarm(tick_timer_t *tim);

/** 
 * Returns current tick.
 * @param tim the tick timer struct
 * @return the current tick value
 */
tick_t tick_timer_get_current(tick_timer_t *tim);

/**
 * Returns the current untriggered alarm.
 * @param tim the tick timer struct
 * @return the current untriggered alarm value, or 0
 *         if there is no active alarm set
 */
tick_t tick_timer_get_alarm(tick_timer_t *tim);

/**
 * This is called when an alarm triggers. Override at pleasure.
 * Original implementation does nothing.
 * @param tim the tick timer struct
 */
__attribute__(( weak )) void tick_timer_on_alarm(tick_timer_t *tim);

#endif