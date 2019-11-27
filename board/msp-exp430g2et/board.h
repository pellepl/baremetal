// BluePill STM32F103 board

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

#define BOARD_PIN_MAX                   (8*2)

#define BOARD_BUTTON_COUNT              (1)
#define BOARD_BUTTON_GPIO_PIN           ((const uint16_t[BOARD_BUTTON_COUNT]){3})
#define BOARD_BUTTON_GPIO_ACTIVE        ((const uint8_t[BOARD_BUTTON_COUNT]){0})

#define BOARD_LED_COUNT                 (2)
#define BOARD_LED_GPIO_PIN              ((const uint16_t[BOARD_LED_COUNT]){0,6})
#define BOARD_LED_GPIO_ACTIVE           ((const uint8_t[BOARD_LED_COUNT]){0,0})

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT                (1)
#define BOARD_UART_GPIO_PINS          \
    { \
        (board_uart_pindef_t){.rx_pin=1,.tx_pin=2,.cts_pin=BOARD_PIN_UNDEF,.rts_pin=BOARD_PIN_UNDEF}, \
    }

#endif // _BOARD_H_
