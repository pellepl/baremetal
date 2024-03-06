/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

// Pluto board

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

#define BOARD_PIN_MAX (32)

#define BOARD_BUTTON_COUNT (3)
#define BOARD_BUTTON_GPIO_PIN ((const uint16_t[BOARD_BUTTON_COUNT]){19, 11, 25})
#define BOARD_BUTTON_GPIO_ACTIVE ((const uint8_t[BOARD_BUTTON_COUNT]){0, 0, 0})

#define BOARD_LED_COUNT (0)
#define BOARD_LED_GPIO_PIN ((const uint16_t[BOARD_LED_COUNT]){})
#define BOARD_LED_GPIO_ACTIVE ((const uint8_t[BOARD_LED_COUNT]){})

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT (1)
#define BOARD_UART_GPIO_PINS                                                           \
    {                                                                                  \
        (board_uart_pindef_t){.rx_pin = 6, .tx_pin = 8, .cts_pin = -1, .rts_pin = -1}, \
    }

#endif // _BOARD_H_
