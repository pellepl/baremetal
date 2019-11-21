#ifndef _GPIO_DRIVER_H_
#define _GPIO_DRIVER_H_

#include "types.h"
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

#define GPIO_PIN_UNDEF        (BOARD_PIN_UNDEF)

/** Initializes gpio block */
int gpio_init(void);
/** Configures given pin */
int gpio_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull);
/** Sets given pin high or low */
int gpio_set(uint16_t pin, uint8_t state);
/** Returns state of given pin as high or low */
int gpio_read(uint16_t pin);
/** Frees and decouples any resources used by gpio block */
int gpio_deinit(void);

#endif // _GPIO_DRIVER_H_
