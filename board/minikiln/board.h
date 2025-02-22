/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

// based on BluePill STM32F103 board

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

// #define MAX6675
#define MAX31855

#define PORTA(x) (0 * 16 + x)
#define PORTB(x) (1 * 16 + x)
#define PORTC(x) (2 * 16 + x)

#define BOARD_PIN_MAX (16 * 3)

#define BOARD_BUTTON_COUNT (0)
#define BOARD_BUTTON_GPIO_PIN ((const uint16_t[BOARD_BUTTON_COUNT]){})
#define BOARD_BUTTON_GPIO_ACTIVE ((const uint8_t[BOARD_BUTTON_COUNT]){})

#define BOARD_LED_COUNT (0)
#define BOARD_LED_GPIO_PIN ((const uint16_t[BOARD_LED_COUNT]){})
#define BOARD_LED_GPIO_ACTIVE ((const uint8_t[BOARD_LED_COUNT]){})

#define THERMOCOUPLE_SAMPLE_PERIOD_MS 100
#define THERMOCOUPLE_MAX_NBR_CONSEQ_ERRS 10

#define BOARD_PIN_ELEMENT_1 PORTB(12)

#define BOARD_PIN_THERMOC_CLK PORTB(13)
#define BOARD_PIN_THERMOC_CS PORTB(14)
#define BOARD_PIN_THERMOC_MISO PORTB(15)

#define BOARD_PIN_ROTARY_S0 PORTA(0)
#define BOARD_PIN_ROTARY_S1 PORTA(1)
#define BOARD_PIN_ROTARY_PRESS PORTA(4)

#define BOARD_PIN_DISP_I2C_SCL PORTB(6)
#define BOARD_PIN_DISP_I2C_SDA PORTB(7)

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT (1)
#define BOARD_UART_GPIO_PINS                                                                                                   \
    {                                                                                                                          \
        (board_uart_pindef_t){.rx_pin = PORTA(3), .tx_pin = PORTA(2), .cts_pin = BOARD_PIN_UNDEF, .rts_pin = BOARD_PIN_UNDEF}, \
    }

#endif // _BOARD_H_
