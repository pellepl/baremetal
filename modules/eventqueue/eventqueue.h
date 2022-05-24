/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _EVENTQUEUE_H
#define _EVENTQUEUE_H

#include "bmtypes.h"

#ifndef STR
#  define STR(x) _STR(x)
#  define _STR(x) #x
#endif

#ifdef EVENTQ_CUSTOM_INC
#  include STR(EVENTQ_CUSTOM_INC)
#endif

#ifndef EVENTQ_DBG
#define EVENTQ_DBG(...)
#endif

#ifndef eventq_type_t
#define eventq_type_t uint32_t
#endif

#ifndef EVENTQ_CRITICAL_REGION_ENTER
#define EVENTQ_CRITICAL_REGION_ENTER()
#endif

#ifndef EVENTQ_CRITICAL_REGION_EXIT
#define EVENTQ_CRITICAL_REGION_EXIT()
#endif

#ifndef EVENTQ_EVENT_POOL_SIZE
#define EVENTQ_EVENT_POOL_SIZE (32)
#endif

typedef void (*eventq_handle_fn_t)(eventq_type_t type, void *arg);

typedef struct eventq_evt_s {
    eventq_type_t type;
    void *arg;
    eventq_handle_fn_t fn;
    volatile struct eventq_evt_s *_next;
} eventq_evt_t;

/**
 * Initiates the event queue.
 * @param generic_handle_fn event handle function, for events without specific
 *                           handle function.
 */
void eventq_init(eventq_handle_fn_t generic_handle_fn);

/**
 * Schedules a new event for execution.
 * @param type      type of event, passed to handle function
 * @param arg       event argument, passed to handle function
 * @param handle_fn event handle function, if NULL the generic function handler
 *                  giving in initialization is called.
 * @return zero if there are no free events, non-zero if ok
 */
int eventq_add(eventq_type_t type, void *arg, eventq_handle_fn_t handle_fn);

/**
 * Executes one scheduled event.
 * @return zero if there were no events to execute, non-zero if an event was executed
 */
int eventq_run(void);

#endif // _EVENTQUEUE_H
