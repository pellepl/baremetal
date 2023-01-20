/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "board.h"
#include "gpio_driver.h"

int main(void) {
    uint32_t ix;
    (void)ix;
    cpu_init();
    board_init();
    gpio_init();
#   if BOARD_LED_COUNT > 0
    for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
        gpio_config(BOARD_LED_GPIO_PIN[ix], GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }
#   endif
    while(1) {
#   if BOARD_LED_COUNT > 0
        for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
            gpio_set(BOARD_LED_GPIO_PIN[ix], BOARD_LED_GPIO_ACTIVE[ix]);
        }
#   endif
        cpu_halt(500);
#   if BOARD_LED_COUNT > 0
        for (ix = 0; ix < BOARD_LED_COUNT; ix++) {
            gpio_set(BOARD_LED_GPIO_PIN[ix], !BOARD_LED_GPIO_ACTIVE[ix]);
        }
#   endif
        cpu_halt(500);
    }
}
