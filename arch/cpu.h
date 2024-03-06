/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _CPU_H_
#define _CPU_H_

#include "bmtypes.h"

void cpu_init(void);
void cpu_reset(void);
void cpu_halt(uint32_t milliseconds);
void cpu_halt_us(uint32_t microseconds);
void cpu_interrupt_enable(void);
void cpu_interrupt_disable(void);
uint32_t cpu_core_clock_freq(void);

#endif // _CPU_H_
