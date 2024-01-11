/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#if CONFIG_GPIO==1

#include "gpio_driver.h"
#include "cli.h"
#include "minio.h"
#include "board.h"
#include "cpu.h"

static int cli_gpio_set(int argc, const char **argv) {
    if (argc != 2) {
        printf("[<pin>, 0|1]\n");
        return -1;
    }
    int pin = strtol(argv[0], 0, 0);
    int set = strtol(argv[1], 0, 0);
    return gpio_set(pin, set);
}
CLI_FUNCTION(cli_gpio_set, "gpio_set", "");

static int cli_gpio_read(int argc, const char **argv) {
    if (argc != 1) {
        printf("[<pin>]\n");
        return -1;
    }
    int pin = strtol(argv[0], 0, 0);
    int val = gpio_read(pin);
    if (val >= 0) {
        printf("%d\n", val ? 1 : 0);
    } else {
        return val;
    }
    return 0;
}
CLI_FUNCTION(cli_gpio_read, "gpio_read", "");

static int cli_gpio_config(int argc, const char **argv) {
    if (argc < 2 || argc > 3) {
        printf("[<pin>, out|in (, up|down|none)]\n");
        return -1;
    }
    int pin = strtol(argv[0], 0, 0);
    gpio_direction_t dir = argv[1][0] == 'o' ? GPIO_DIRECTION_OUTPUT : GPIO_DIRECTION_INPUT;
    gpio_pull_t pull;
    if (argc == 2) {
        pull = GPIO_PULL_NONE;
    } else {
        pull = argv[2][0] == 'u' ? GPIO_PULL_UP : (argv[2][0] == 'd' ? GPIO_PULL_DOWN : GPIO_PULL_NONE);
    }
    return gpio_config(pin, dir, pull);
}
CLI_FUNCTION(cli_gpio_config, "gpio_config", "");

static int cli_gpio_freq(int argc, const char **argv) {
    if (argc != 3) {
        printf("[<pin>, <freq>, <time_ms>]\n");
        return -1;
    }
    int res;
    int pin = strtol(argv[0], 0, 0);
    int freq = strtol(argv[1], 0, 0);
    int time = strtol(argv[2], 0, 0);
    int period_half = 1000 / freq / 2;
    if (period_half <= 0) return -1;
    int accuracy = (sizeof(void*) == 2) ? 0x100 : 0x10000;
    int err = (((accuracy * 1000) / freq) - (accuracy * (2*period_half))) / 1000;
    int err_accum = 0;
    int state = 1;
    res = gpio_config(pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    for (int i = 0; !res && i < time / period_half; i++) {
        gpio_set(pin, state);
        err_accum += err;
        if (err_accum >= accuracy) {
            cpu_halt(period_half + 1);
            err_accum -= accuracy;
        } else {
            cpu_halt(period_half);
        }
        state = state ? 0 : 1;
    }
    return res;
}
CLI_FUNCTION(cli_gpio_freq, "gpio_freq", "");

static int cli_gpio_list(int argc, const char **argv) {
    const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;

    int i;
    printf("pin count: %d\n", BOARD_PIN_MAX);
    printf("led count: %d\n", BOARD_LED_COUNT);
    for (i = 0; i < BOARD_LED_COUNT; i++) {
        printf("  pin:%2d\tactive:%d\n", BOARD_LED_GPIO_PIN[i], BOARD_LED_GPIO_ACTIVE[i]);
    }
    printf("button count: %d\n", BOARD_BUTTON_COUNT);
    for (i = 0; i < BOARD_BUTTON_COUNT; i++) {
        printf("  pin:%2d\tactive:%d\n", BOARD_BUTTON_GPIO_PIN[i], BOARD_BUTTON_GPIO_ACTIVE[i]);
    }
    printf("uart count: %d\n", BOARD_UART_COUNT);
    for (i = 0; i < BOARD_UART_COUNT; i++) {
        printf("  uart%d rx:%d tx:%d rts:%d cts:%d\n", i,
          uart_pindefs[i].rx_pin, uart_pindefs[i].tx_pin, uart_pindefs[i].rts_pin, uart_pindefs[i].cts_pin);
    }
    return 0;
}
CLI_FUNCTION(cli_gpio_list, "gpio_list", "");

#endif
