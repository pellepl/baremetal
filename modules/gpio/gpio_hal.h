/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _GPIO_HAL_H_
#define _GPIO_HAL_H_

#include "gpio_driver.h"

int gpio_hal_init(void);
int gpio_hal_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull);
int gpio_hal_disconnect(uint16_t pin, gpio_pull_t pull);
int gpio_hal_set(uint16_t pin, uint8_t state);
int gpio_hal_read(uint16_t pin);
int gpio_hal_deinit(void);
int gpio_hal_irq_callback(uint16_t pin, gpio_irq_cb_t callback);

#endif // _GPIO_HAL_H_
