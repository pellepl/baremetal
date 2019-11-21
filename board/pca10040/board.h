// Nordic NRF52 PCA10040 board

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

#define BOARD_PIN_MAX                   (32)

#define BOARD_BUTTON_COUNT              (4)
#define BOARD_BUTTON_GPIO_PIN           ((const uint16_t[BOARD_BUTTON_COUNT]){13,14,15,16})
#define BOARD_BUTTON_GPIO_ACTIVE        ((const uint8_t[BOARD_BUTTON_COUNT]){0,0,0,0})

#define BOARD_LED_COUNT                 (4)
#define BOARD_LED_GPIO_PIN              ((const uint16_t[BOARD_LED_COUNT]){17,18,19,20})
#define BOARD_LED_GPIO_ACTIVE           ((const uint8_t[BOARD_LED_COUNT]){0,0,0,0})

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT                (1)
#define BOARD_UART_GPIO_PINS          \
    { \
        (board_uart_pindef_t){.rx_pin=8,.tx_pin=6,.cts_pin=7,.rts_pin=5}, \
    }

#endif // _BOARD_H_
