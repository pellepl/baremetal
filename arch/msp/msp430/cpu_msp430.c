#include "msp430.h"
#include "cpu.h"

void cpu_init(void) {
    WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
}

void cpu_reset(void) {
 // TODO PETER
 // start watchdog and timeout?
}

void cpu_halt(uint32_t milliseconds) {
    volatile uint32_t t = 0x1 * milliseconds;
    while (--t);
}

void cpu_interrupt_enable(void) {
    __nop();
    __bis_SR_register(GIE);
    __nop();
}

void cpu_interrupt_disable(void) {
    __bic_SR_register(GIE);
}

uint32_t cpu_core_clock_freq(void) {
    return 10000000ul; // TODO PETER
}
