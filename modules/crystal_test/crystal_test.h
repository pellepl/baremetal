/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _CRYSTAL_TEST_H_
#define _CRYSTAL_TEST_H_

typedef enum {
    LFCLK_OSC_RC = 0,
    LFCLK_OSC_XTAL = 1,
    LFCLK_OSC_SYNTH = 2,
} lfclk_oscillator_t;

int crystal_test(lfclk_oscillator_t osc);

#endif // _CRYSTAL_TEST_H_