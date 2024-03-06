#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "cpu.h"

#define EVENTQ_CRITICAL_REGION_ENTER() cpu_interrupt_disable()
#define EVENTQ_CRITICAL_REGION_EXIT() cpu_interrupt_enable()
#define EVENTQ_EVENT_POOL_SIZE (32)

#endif // _EVENTS_H_

