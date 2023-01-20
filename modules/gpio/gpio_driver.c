/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "gpio_driver.h"
#include "gpio_hal.h"
#include "board.h"

int gpio_init(void) {
    return gpio_hal_init();
}
int gpio_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull) {
#if defined(BOARD_PIN_MAX) && BOARD_PIN_MAX > 0
    if (pin >= BOARD_PIN_MAX) return -1;
    return gpio_hal_config(pin, dir, pull);
#else
    return 0;
#endif
}
int gpio_set(uint16_t pin, uint8_t state) {
#if defined(BOARD_PIN_MAX) && BOARD_PIN_MAX > 0
    if (pin >= BOARD_PIN_MAX) return -1;
    return gpio_hal_set(pin, state);
#else
    return -1;
#endif
}
int gpio_read(uint16_t pin) {
#if defined(BOARD_PIN_MAX) && BOARD_PIN_MAX > 0
    if (pin >= BOARD_PIN_MAX) return -1;
    return gpio_hal_read(pin);
#else
    return -1;
#endif
}
int gpio_deinit(void) {
    return gpio_hal_deinit();
}
