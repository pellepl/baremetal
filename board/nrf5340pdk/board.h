/* Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

// Nordic NRF5340-PDK board

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

#define BOARD_PIN_MAX                   (48)

#define BOARD_BUTTON_COUNT              (4)
#define BOARD_BUTTON_GPIO_PIN           ((const uint16_t[BOARD_BUTTON_COUNT]){8,9,23,24})
#define BOARD_BUTTON_GPIO_ACTIVE        ((const uint8_t[BOARD_BUTTON_COUNT]){0,0,0,0})

#define BOARD_LED_COUNT                 (4)
#define BOARD_LED_GPIO_PIN              ((const uint16_t[BOARD_LED_COUNT]){28,29,30,31})
#define BOARD_LED_GPIO_ACTIVE           ((const uint8_t[BOARD_LED_COUNT]){0,0,0,0})

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT                (1)
#define BOARD_UART_GPIO_PINS          \
    { \
        (board_uart_pindef_t){.rx_pin=19,.tx_pin=20,.cts_pin=21,.rts_pin=22}, \
    }

#endif // _BOARD_H_
