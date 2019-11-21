#ifndef _CPU_H_
#define _CPU_H_

#include "types.h"

void cpu_init(void);
void cpu_reset(void);
void cpu_halt(uint32_t milliseconds);
void cpu_interrupt_enable(void);
void cpu_interrupt_disable(void);
uint32_t cpu_core_clock_freq(void);

#endif // _CPU_H_
