#include "msp430.h"
#include "cpu.h"

uint32_t _clockspeed = 1100000ul; // https://www.ti.com/lit/ug/slau144j/slau144j.pdf 5.2 p 275

void cpu_init(void) {
    WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
}

void cpu_reset(void) {
    FCTL1 = 0; // https://www.ti.com/lit/ug/slau144j/slau144j.pdf 7.4.1 p 324
}

void cpu_halt(uint32_t milliseconds) {
    // ~ severily roughly
    volatile uint32_t cycles = 3 * milliseconds * (cpu_core_clock_freq() >> 16);
    while (cycles--);
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
    return _clockspeed;
}

// needed by crt0.o
void *memmove (void *dst, const void *src, unsigned int num);
void *memmove (void *dst, const void *src, unsigned int num) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (num--) *d++ = *s++;
    } else {
      const uint8_t *ls = s + (num-1);
      uint8_t *ld = d + (num-1);
      while (num--) *ld-- = *ls--;
    }
    return dst;
}
