/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _UART_DRIVER_H_
#define _UART_DRIVER_H_

#include "bmtypes.h"

#ifndef UART_STD
#define UART_STD            (0)
#endif

#ifndef ERR_UART_BASE
#define ERR_UART_BASE       (100)
#endif

#define ERR_UART_NOINIT     -(ERR_UART_BASE + 0)
#define ERR_UART_BUSY       -(ERR_UART_BASE + 1)
#define ERR_UART_CONFIG     -(ERR_UART_BASE + 2)

typedef struct {
    enum uart_config_baudrate {
        UART_BAUDRATE_600 = 0,
        UART_BAUDRATE_1200,
        UART_BAUDRATE_2400,
        UART_BAUDRATE_4800,
        UART_BAUDRATE_9600,
        UART_BAUDRATE_57600,
        UART_BAUDRATE_115200,
        UART_BAUDRATE_460800,
        UART_BAUDRATE_921600,
        UART_BAUDRATE_1000000,
    } baudrate;
    enum uart_config_flowcontrol {
        UART_FLOWCONTROL_NONE = 0,
        UART_FLOWCONTROL_RTSCTS,
    } flowcontrol;
    enum uart_config_stopbits {
        UART_STOPBITS_1 = 0,
        UART_STOPBITS_2,
    } stopbits;
    enum uart_config_parity {
        UART_PARITY_NONE = 0,
        UART_PARITY_ODD,
        UART_PARITY_EVEN,
    } parity;
} uart_config_t;

/** Initializes given uart block with given config */
int uart_init(unsigned int hdl, const uart_config_t *config);
/** Sends a character over given uart block - blocking */
int uart_putchar(unsigned int hdl, char x);
/** Receives a character over given uart block - blocking */
int uart_getchar(unsigned int hdl);
/** Receives a character over given uart block if there is one available, else returns -1 */
int uart_pollchar(unsigned int hdl);
/** Frees and decouples any resources used by given uart block */
int uart_deinit(unsigned int hdl);

#endif // _UART_DRIVER_H_
