/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "minio.h"

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
    uart_config_t cfg = {
        .baudrate = DEFAULT_UART_BAUDRATE,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE
    };
    uart_init(UART_STD, &cfg);

    int d = 0;
    while(1) {
        printf("Hello world %d!\n", d++);
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
