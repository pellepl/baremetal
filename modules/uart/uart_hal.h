/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _UART_HAL_H_
#define _UART_HAL_H_

#include "uart_driver.h"

int uart_hal_init(unsigned int hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin);
int uart_hal_tx(unsigned int hdl, char x);
int uart_hal_rx(unsigned int hdl);
int uart_hal_rxpoll(unsigned int hdl);
int uart_hal_deinit(unsigned int hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin);

#endif // _UART_HAL_H_
