/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

// U-blox NINA B3 EVK board

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

#define BOARD_PIN_MAX                   (32+16)

#define BOARD_BUTTON_COUNT              (1)
#define BOARD_BUTTON_GPIO_PIN           ((const uint16_t[BOARD_BUTTON_COUNT]){2})
#define BOARD_BUTTON_GPIO_ACTIVE        ((const uint8_t[BOARD_BUTTON_COUNT]){0})

#define BOARD_LED_COUNT                 (3)
#define BOARD_LED_GPIO_PIN              ((const uint16_t[BOARD_LED_COUNT]){13,25,32+0})
#define BOARD_LED_GPIO_ACTIVE           ((const uint8_t[BOARD_LED_COUNT]){0,0,0})

#define BOARD_UART_COUNT                (1)
#define BOARD_UART_GPIO_PINS          \
    { \
        (board_uart_pindef_t){.rx_pin=29,.tx_pin=32+13,.cts_pin=32+12,.rts_pin=31}, \
    }

#endif // _BOARD_H_
