/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _GPIO_DRIVER_H_
#define _GPIO_DRIVER_H_

#include "bmtypes.h"
#include "board_common.h"

typedef enum {
    GPIO_DIRECTION_INPUT = 0,
    GPIO_DIRECTION_OUTPUT,
    GPIO_DIRECTION_ANALOG,
    GPIO_DIRECTION_FUNCTION_IN,
    GPIO_DIRECTION_FUNCTION_OUT,
} gpio_direction_t;

typedef enum {
    GPIO_PULL_NONE = 0,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN,
} gpio_pull_t;

typedef void (*gpio_irq_cb_t)(uint16_t pin, uint8_t state);

#define GPIO_PIN_UNDEF        (BOARD_PIN_UNDEF)

/** Initializes gpio block */
int gpio_init(void);
/** Configures given pin */
int gpio_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull);
/** Disconnect pin */
int gpio_disconnect(uint16_t pin, gpio_pull_t pull);
/** Sets given pin high or low */
int gpio_set(uint16_t pin, uint8_t state);
/** Returns state of given pin as high or low */
int gpio_read(uint16_t pin);
/** Frees and decouples any resources used by gpio block */
int gpio_deinit(void);
/** Registers to call given callback on changes, callback = 0 to disable */
int gpio_irq_callback(uint16_t pin, gpio_irq_cb_t callback);

#endif // _GPIO_DRIVER_H_
