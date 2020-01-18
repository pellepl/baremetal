/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "msp430.h"
#include "uart_hal.h"
#include "gpio_driver.h"
#include "board_common.h"

int uart_hal_init(unsigned int hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    gpio_config(rx_pin, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
    gpio_config(tx_pin, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
    UCA0CTL0 = 0 |
        (config->parity == UART_PARITY_NONE ? 0 : UCPEN) |
        (config->parity == UART_PARITY_EVEN ? 0 : UCPAR) |
        (config->stopbits == UART_STOPBITS_1 ? 0 : UCSPB);
    
    UCA0CTL1 |= UCSSEL_2; // SMCLK
    // TODO PETER
    UCA0BR0 = 0x08; // 1MHz 115200
    UCA0BR1 = 0x00; // 1MHz 115200
    UCA0MCTL = UCBRS2 + UCBRS0; // Modulation UCBRSx = 5
    UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine
    return 0;
}

int uart_hal_tx(unsigned int hdl, char x) {
    while ((IFG2 & UCA0TXIFG) == 0);
    UCA0TXBUF = x;
    return 0;
}

int uart_hal_rx(unsigned int hdl) {
    while ((IFG2 & UCA0RXIFG) == 0);
    return UCA0RXBUF;
}

int uart_hal_rxpoll(unsigned int hdl) {
    if ((IFG2 & UCA0RXIFG) == 0)
        return -1;
    else
        return UCA0RXBUF;
}

int uart_hal_deinit(unsigned int hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    UCA0CTL1 |= UCSWRST; // place USCI in reset
    return 0;
}

