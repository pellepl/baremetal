/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "uart_driver.h"
#include "uart_hal.h"
#include "board.h"

static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;

int uart_init(uint32_t hdl, const uart_config_t *config) {
    if (!config) return -1;
    if (hdl >= BOARD_UART_COUNT) return -1;
    const board_uart_pindef_t pindef = uart_pindefs[hdl];
    return uart_hal_init(hdl, config,
        pindef.rx_pin, pindef.tx_pin, pindef.rts_pin, pindef.cts_pin);
}
int uart_putchar(uint32_t hdl, char x) {
    if (hdl >= BOARD_UART_COUNT) return -1;
    return uart_hal_tx(hdl, x);
}
int uart_getchar(uint32_t hdl) {
    if (hdl >= BOARD_UART_COUNT) return -1;
    return uart_hal_rx(hdl);
}
int uart_pollchar(uint32_t hdl) {
    if (hdl >= BOARD_UART_COUNT) return -1;
    return uart_hal_rxpoll(hdl);
}
int uart_deinit(uint32_t hdl) {
    if (hdl >= BOARD_UART_COUNT) return -1;
    const board_uart_pindef_t pindef = uart_pindefs[hdl];
    return uart_hal_deinit(hdl,
        pindef.rx_pin, pindef.tx_pin, pindef.rts_pin, pindef.cts_pin);
}
