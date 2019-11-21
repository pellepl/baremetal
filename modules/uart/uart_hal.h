#ifndef _UART_HAL_H_
#define _UART_HAL_H_

#include "uart_driver.h"

int uart_hal_init(uint32_t hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin);
int uart_hal_tx(uint32_t hdl, char x);
int uart_hal_rx(uint32_t hdl);
int uart_hal_rxpoll(uint32_t hdl);
int uart_hal_deinit(uint32_t hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin);

#endif // _UART_HAL_H_
