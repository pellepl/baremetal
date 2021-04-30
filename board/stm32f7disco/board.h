// STM32F7 Discovery kit for STM32F746

#ifndef _BOARD_H_
#define _BOARD_H_

#include "board_common.h"

#define BOARD_PORT_TO_PIN(x)            (((x)-'A')*16)
#define BOARD_PIN(port, pin)            (BOARD_PORT_TO_PIN(port) + (pin))

#define BOARD_PIN_MAX                   (BOARD_PIN('L', 0)-1)

#define BOARD_BUTTON_COUNT              (1)
#define BOARD_BUTTON_GPIO_PIN           ((const uint16_t[BOARD_BUTTON_COUNT]){BOARD_PIN('I', 11)})
#define BOARD_BUTTON_GPIO_ACTIVE        ((const uint8_t[BOARD_BUTTON_COUNT]){0})

#define BOARD_LED_COUNT                 (1)
#define BOARD_LED_GPIO_PIN              ((const uint16_t[BOARD_LED_COUNT]){BOARD_PIN('I', 1)})
#define BOARD_LED_GPIO_ACTIVE           ((const uint8_t[BOARD_LED_COUNT]){0})

// used like this
// static const board_uart_pindef_t uart_pindefs[BOARD_UART_COUNT] = BOARD_UART_GPIO_PINS;
#define BOARD_UART_COUNT                (1)
// tx:CN4/D1 rx:CN4/D0
#define BOARD_UART_GPIO_PINS          \
    { \
        (board_uart_pindef_t){.rx_pin=BOARD_PIN('C', 7),.tx_pin=BOARD_PIN('C', 6),.cts_pin=-1,.rts_pin=-1}, \
    }

#define LCD_W   480
#define LCD_H   272

#endif // _BOARD_H_
