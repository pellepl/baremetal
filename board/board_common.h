/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _BOARD_COMMON_H_
#define _BOARD_COMMON_H_

#include "bmtypes.h"
#include "cpu.h"

typedef struct  {
    uint16_t rx_pin;
    uint16_t tx_pin;
    uint16_t cts_pin;
    uint16_t rts_pin;
} board_uart_pindef_t;

#define BOARD_PIN_UNDEF         ((uint16_t)-1)

#ifndef DEFAULT_UART_BAUDRATE
#define DEFAULT_UART_BAUDRATE   UART_BAUDRATE_115200
#endif

void board_init(void);

#endif // _BOARD_COMMON_H_
