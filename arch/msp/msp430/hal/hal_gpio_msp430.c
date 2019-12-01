/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "gpio_hal.h"
#include "msp430.h"

int gpio_hal_init(void) {
    return 0;
}

volatile uint8_t *pxin[]   = {&P1IN,  &P2IN,  &P3IN};
volatile uint8_t *pxout[]  = {&P1OUT, &P2OUT, &P3OUT};
volatile uint8_t *pxdir[]  = {&P1DIR, &P2DIR, &P3DIR};
volatile uint8_t *pxren[]  = {&P1REN, &P2REN, &P3REN};
volatile uint8_t *pxsel[]  = {&P1SEL, &P2SEL, &P3SEL};
volatile uint8_t *pxsel2[] = {&P1SEL2,&P3SEL2,&P3SEL2};

int gpio_hal_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull) {
    uint8_t port = pin>>3;
    uint8_t pinmask = (1<<(pin&7));
    if (dir == GPIO_DIRECTION_FUNCTION_IN || dir == GPIO_DIRECTION_FUNCTION_OUT) {
        *pxsel[port] |= pinmask;
        if (pull != GPIO_PULL_NONE) {
            *pxsel2[port] |= pinmask;
        } else {
            *pxsel2[port] &= ~pinmask;
        }
    } else {
        *pxsel[port] &= ~pinmask;
        *pxsel2[port] &= ~pinmask;
    }
    if (dir == GPIO_DIRECTION_OUTPUT || dir == GPIO_DIRECTION_FUNCTION_OUT) {
        *pxdir[port] |= pinmask; 
    } else {
        *pxdir[port] &= ~pinmask; 
        if (dir == GPIO_DIRECTION_INPUT) {
            if (pull == GPIO_PULL_NONE) {
                *pxren[port] &= ~pinmask;
            } else {
                *pxren[port] |= pinmask;
                if (pull == GPIO_PULL_UP) {
                    *pxout[port] |= pinmask;
                } else {
                    *pxout[port] &= ~pinmask;
                }
            }
        }
    }

    return 0;
}

int gpio_hal_set(uint16_t pin, uint8_t state) {
    uint8_t port = pin>>3;
    uint8_t pinmask = (1<<(pin&7));
    if (state) {
        *pxout[port] |= pinmask;
    } else {
        *pxout[port] &= ~pinmask;
    }
    return 0;
}

int gpio_hal_read(uint16_t pin) {
    uint8_t port = pin>>3;
    uint8_t pinmask = (1<<(pin&7));
    return (*pxin[port] & pinmask) != 0;
}

int gpio_hal_deinit(void) {
    return 0;
}
