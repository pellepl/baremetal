#include "gpio_driver.h"
#include "gpio_hal.h"
#include "board.h"

int gpio_init(void) {
    return gpio_hal_init();
}
int gpio_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull) {
    if (pin >= BOARD_PIN_MAX) return -1;
    return gpio_hal_config(pin, dir, pull);
}
int gpio_set(uint16_t pin, uint8_t state) {
    if (pin >= BOARD_PIN_MAX) return -1;
    return gpio_hal_set(pin, state);
}
int gpio_read(uint16_t pin) {
    if (pin >= BOARD_PIN_MAX) return -1;
    return gpio_hal_read(pin);
}
int gpio_deinit(void) {
    return gpio_hal_deinit();
}
