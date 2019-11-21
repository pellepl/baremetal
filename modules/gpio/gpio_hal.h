#ifndef _GPIO_HAL_H_
#define _GPIO_HAL_H_

#include "gpio_driver.h"

int gpio_hal_init(void);
int gpio_hal_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull);
int gpio_hal_set(uint16_t pin, uint8_t state);
int gpio_hal_read(uint16_t pin);
int gpio_hal_deinit(void);

#endif // _GPIO_HAL_H_
