/* Copyright (c) 2023 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include "uart_hal.h"
#include "board_common.h"

int uart_hal_init(unsigned int hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    // disable stdout buffer
    setvbuf(stdout, NULL, _IONBF, 0);
    return 0;
}

int uart_hal_tx(unsigned int hdl, char x) {
    if (hdl == UART_STD) {
        printf("%c", x);
    }
    return 0;
}

int uart_hal_rx(unsigned int hdl) {
    uint8_t b;
    if (hdl == UART_STD) {
        int err = read(STDIN_FILENO, &b, 1);
        if (err >= 1) {
            return b;
        }
    }
    return -1;
}

int uart_hal_rxpoll(unsigned int hdl) {
    if (hdl == UART_STD) {
        struct pollfd ufds = {
            .fd = STDIN_FILENO,
            .events = POLLIN,
            .revents = 0
        };
        int err = poll(&ufds, 1, 0);
        if (err > 0 && (ufds.revents & POLLIN) != 0) {
            return uart_hal_rx(hdl);
        }
    }
    return -1;
}

int uart_hal_deinit(unsigned int hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    return 0;
}

