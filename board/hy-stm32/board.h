/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

// BluePill STM32F103 board

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

#define BOARD_PIN_MAX                   (16*5)

#define BOARD_BUTTON_COUNT              (4)
#define BOARD_BUTTON_GPIO_PIN           ((const uint16_t[BOARD_BUTTON_COUNT]){4*16+5,4*16+4,4*16+3,4*16+2})
#define BOARD_BUTTON_GPIO_ACTIVE        ((const uint8_t[BOARD_BUTTON_COUNT]){0,0,0,0})

#define BOARD_LED_COUNT                 (4)
#define BOARD_LED_GPIO_PIN              ((const uint16_t[BOARD_LED_COUNT]){2*16+6,2*16+7,3*16+13,3*16+6})
#define BOARD_LED_GPIO_ACTIVE           ((const uint8_t[BOARD_LED_COUNT]){1,1,1,1})

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT                (2)
#define BOARD_UART_GPIO_PINS          \
    { \
        (board_uart_pindef_t){.rx_pin=0*16+3,.tx_pin=0*16+2,.cts_pin=BOARD_PIN_UNDEF,.rts_pin=BOARD_PIN_UNDEF}, \
        (board_uart_pindef_t){.rx_pin=0*16+10,.tx_pin=0*16+9,.cts_pin=BOARD_PIN_UNDEF,.rts_pin=BOARD_PIN_UNDEF}, \
    }

#endif // _BOARD_H_
