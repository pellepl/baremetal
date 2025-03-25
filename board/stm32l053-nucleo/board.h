/* Copyright (c) 2025 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

// stm32l0disco stm32l053

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"
#include "stm32l053xx.h"

#define PORTA(x) (0 + (x))
#define PORTB(x) (16 + (x))
#define PORTC(x) (32 + (x))

#define BOARD_PIN_MAX (16 * 3)

#define BOARD_BUTTON_COUNT (1)
#define BOARD_BUTTON_GPIO_PIN ((const uint16_t[BOARD_BUTTON_COUNT]){PORTC(13)})
#define BOARD_BUTTON_GPIO_ACTIVE ((const uint8_t[BOARD_BUTTON_COUNT]){0})

#define BOARD_LED_COUNT (1)
#define BOARD_LED_GPIO_PIN ((const uint16_t[BOARD_LED_COUNT]){PORTA(5)})
#define BOARD_LED_GPIO_ACTIVE ((const uint8_t[BOARD_LED_COUNT]){1})

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT (1)
#define BOARD_UART_GPIO_PINS                                                                                                    \
    {                                                                                                                           \
        (board_uart_pindef_t){.rx_pin = PORTB(7), .tx_pin = PORTB(6), .cts_pin = BOARD_PIN_UNDEF, .rts_pin = BOARD_PIN_UNDEF}, \
    }

#endif // _BOARD_H_
