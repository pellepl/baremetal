#include "eventqueue.h"

static struct {
    eventq_handle_fn_t common_handle_fn;
    eventq_evt_t pool[EVENTQ_EVENT_POOL_SIZE];
    volatile eventq_evt_t *scheduled_first;
    volatile eventq_evt_t *scheduled_last;
    volatile eventq_evt_t *free;
} _ev_q;

void eventq_init(eventq_handle_fn_t common_handle_fn) {
    _ev_q.common_handle_fn = common_handle_fn;
    _ev_q.scheduled_first = _ev_q.scheduled_last = 0;
    _ev_q.free = 0;
    for (int i = 0; i < EVENTQ_EVENT_POOL_SIZE; i++) {
        _ev_q.pool[i]._next = _ev_q.free;
        _ev_q.free = &_ev_q.pool[i];
    }
}

int eventq_run(void) {
    eventq_type_t type;
    void *arg;
    eventq_handle_fn_t fn = 0;
    eventq_evt_t *ev = 0;
    EVENTQ_CRITICAL_REGION_ENTER();
    {
        // fetch first event, get data, make it free, and put it in free queue
        ev = _ev_q.scheduled_first;
        if (ev) {
            // get first event data
            type = ev->type;
            arg = ev->arg;
            fn = ev->fn;

            // schedule next event
            _ev_q.scheduled_first = ev->_next;
            if (_ev_q.scheduled_first == 0) {
                // no more scheduled events
                _ev_q.scheduled_last = 0;
            }

            // put current ev in free list
            if (_ev_q.free) {
                ev->_next = _ev_q.free;
            } else {
                // free list was empty, this is the first free
                ev->_next = 0;
            }
            _ev_q.free = ev;
        }
    }
    EVENTQ_CRITICAL_REGION_EXIT();
    if (ev) {
        if (fn) { 
            fn(type, arg);
        } else if (_ev_q.common_handle_fn) {
            _ev_q.common_handle_fn(type, arg);
        }
    }
    return ev != 0;
}

int eventq_add(eventq_type_t type, void *arg, eventq_handle_fn_t handle_fn) {
    int scheduled = 0;
    EVENTQ_CRITICAL_REGION_ENTER();
    if (_ev_q.free) {
        // fetch free event
        eventq_evt_t *ev = _ev_q.free;
        _ev_q.free = ev->_next;

        // put it last in scheduled list
        ev->_next = 0;
        if (_ev_q.scheduled_last) {
            _ev_q.scheduled_last->_next = ev;
            _ev_q.scheduled_last = ev;
        } else {
            // scheduled list was empty
            _ev_q.scheduled_first = _ev_q.scheduled_last = ev;
        }

        ev->type = type;
        ev->arg = arg;
        ev->fn = handle_fn;
        scheduled = 1;
    }
    EVENTQ_CRITICAL_REGION_EXIT();
    return scheduled;
}
