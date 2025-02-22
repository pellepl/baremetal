#include <stddef.h>
#include "cpu.h"
#include "event.h"
#include "minio.h"

static struct
{
    volatile event_t *head;
    volatile event_t *tail;
    event_func_t generic_event_handler;
} me;

bool event_execute_one(void)
{
    bool had_ev = false;
    event_func_t fn = NULL;
    void *arg = NULL;
    uint32_t type = 0;

    cpu_interrupt_disable();
    volatile event_t *e = me.head;
    if (e != NULL)
    {
        had_ev = true;
        me.head = e->_next;
        if (me.head == NULL)
        {
            me.tail = NULL;
        }
        e->_next = NULL;
        e->_posted = false;
        fn = e->fn;
        type = e->type;
        arg = e->arg;
    }
    cpu_interrupt_enable();

    if (had_ev)
    {
        if (fn)
        {
            fn(type, arg);
        }
        else if (me.generic_event_handler)
        {
            me.generic_event_handler(type, arg);
        }
    }
    return had_ev;
}

void event_add(event_t *e, uint32_t type, void *arg)
{
    cpu_interrupt_disable();
    if (!e->_posted)
    {
        e->_posted = true;
        e->_next = NULL;
        e->type = type;
        e->arg = arg;
        if (me.head == NULL && me.tail == NULL)
        {
            me.head = me.tail = e;
        }
        else
        {
            me.tail->_next = e;
            me.tail = e;
        }
    }
    cpu_interrupt_enable();
}

void event_init(event_func_t generic_event_handler)
{
    me.head = me.tail = NULL;
    me.generic_event_handler = generic_event_handler;
}
