/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include <unistd.h>
#include <stdlib.h>
#include "cpu.h"

void cpu_init(void) {
}

void cpu_reset(void) {
    exit(0);
}

void cpu_halt(uint32_t milliseconds) {
    usleep(milliseconds*1000ULL);
}

void cpu_interrupt_enable(void) {
}

void cpu_interrupt_disable(void) {
}

uint32_t cpu_core_clock_freq(void) {
    return 0xffffffff;
}

