/* Copyright (c) 2023 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include <stdio.h>
#include "uart_hal.h"
#include "board_common.h"

int uart_hal_init(unsigned int hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    return 0;
}

int uart_hal_tx(unsigned int hdl, char x) {
    printf("%c", x);
    return 0;
}

int uart_hal_rx(unsigned int hdl) {
    return -1;
}

int uart_hal_rxpoll(unsigned int hdl) {
    return -1;
}

int uart_hal_deinit(unsigned int hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    return 0;
}

