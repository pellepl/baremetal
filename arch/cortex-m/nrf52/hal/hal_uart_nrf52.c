/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "uart_hal.h"
#include "nrf.h"
#include "gpio_driver.h"

#if defined(NRF52840) || defined(NRF52833)
static NRF_UART_Type *hw[2] = {
    NRF_UART0,
    NRF_UART1
};
#else
static NRF_UART_Type *hw[1] = {
    NRF_UART0,
};
#endif

int uart_hal_init(unsigned int hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    NRF_UART_Type *u = hw[hdl];
    if (rx_pin != BOARD_PIN_UNDEF) {
        gpio_config(rx_pin, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
    }
    if (tx_pin != BOARD_PIN_UNDEF) {
        gpio_set(tx_pin, 1);
        gpio_config(tx_pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }

    switch (config->baudrate) {
        case UART_BAUDRATE_600:     u->BAUDRATE = 0x00027000; break;
        case UART_BAUDRATE_1200:    u->BAUDRATE = 0x0004F000; break;
        case UART_BAUDRATE_2400:    u->BAUDRATE = 0x0009D000; break;
        case UART_BAUDRATE_4800:    u->BAUDRATE = 0x0013B000; break;
        case UART_BAUDRATE_9600:    u->BAUDRATE = 0x00275000; break;
        case UART_BAUDRATE_57600:   u->BAUDRATE = 0x00EBF000; break;
        case UART_BAUDRATE_115200:  u->BAUDRATE = 0x01D7E000; break;
        case UART_BAUDRATE_460800:  u->BAUDRATE = 0x075F7000; break;
        case UART_BAUDRATE_921600:  u->BAUDRATE = 0x0EBED000; break;
        case UART_BAUDRATE_1000000: u->BAUDRATE = 0x10000000; break;
    }

    u->CONFIG = 0
        | ((config->parity == UART_PARITY_NONE ? UART_CONFIG_PARITY_Excluded : UART_CONFIG_PARITY_Included) << UART_CONFIG_PARITY_Pos)
        | (config->flowcontrol == UART_FLOWCONTROL_NONE ? UART_CONFIG_HWFC_Disabled : UART_CONFIG_HWFC_Enabled);

#if defined(UART_PSEL_RXD_CONNECT_Pos)
    u->PSEL.RXD = rx_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : rx_pin;
    u->PSEL.TXD = tx_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : tx_pin;
    u->PSEL.CTS = cts_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : cts_pin;
    u->PSEL.RTS = rts_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : rts_pin;
#else
    u->PSELRXD = rx_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : rx_pin;
    u->PSELTXD = tx_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : tx_pin;
    u->PSELCTS = cts_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : cts_pin;
    u->PSELRTS = rts_pin == BOARD_PIN_UNDEF ? (uint32_t)-1 : rts_pin;
#endif
    if (config->flowcontrol == UART_FLOWCONTROL_RTSCTS) {
        if (cts_pin != BOARD_PIN_UNDEF) {
            gpio_config(cts_pin, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
        }
        if (rts_pin != BOARD_PIN_UNDEF) {
            gpio_set(rts_pin, 1);
            gpio_config(rts_pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
        }
    }

    u->ENABLE = UART_ENABLE_ENABLE_Enabled;

    u->EVENTS_RXDRDY = 0;
    u->EVENTS_ERROR = 0;
    u->EVENTS_RXTO = 0;
    u->EVENTS_TXDRDY = 0;
    u->TASKS_STARTRX = 1;
    u->TASKS_STARTTX = 1;
    u->TXD = 0;

    return 0;
}

int uart_hal_tx(unsigned int hdl, char x) {
    NRF_UART_Type *u = hw[hdl];
    while (u->EVENTS_TXDRDY == 0);
    u->EVENTS_TXDRDY = 0;
    u->TXD = x;
    return 0;
}

int uart_hal_rx(unsigned int hdl) {
    NRF_UART_Type *u = hw[hdl];
    int rxrdy, error;
    do {
        rxrdy = u->EVENTS_RXDRDY;
        error = u->EVENTS_ERROR | u->EVENTS_RXTO;
    } while (!rxrdy && !error);
    if (error) {
        u->EVENTS_RXTO = 0;
        u->EVENTS_ERROR = 0;
        return -1;
    }
    u->EVENTS_RXDRDY = 0;
    char x = (char)(u->RXD & 0xff);
    return (int)x;
}

int uart_hal_rxpoll(unsigned int hdl) {
    NRF_UART_Type *u = hw[hdl];
    int rxrdy, error;
    rxrdy = u->EVENTS_RXDRDY;
    error = u->EVENTS_ERROR | u->EVENTS_RXTO;
    if (error) {
        u->EVENTS_RXTO = 0;
        u->EVENTS_ERROR = 0;
        return -1;
    }
    if (!rxrdy) {
        return -1;
    }
    u->EVENTS_RXDRDY = 0;
    char x = (char)(u->RXD & 0xff);
    return (int)x;
}

int uart_hal_deinit(unsigned int hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    NRF_UART_Type *u = hw[hdl];
    u->TASKS_STOPTX = 1;
    u->TASKS_STOPRX = 1;
    u->ENABLE = UART_ENABLE_ENABLE_Disabled;
#if defined(UART_PSEL_RXD_CONNECT_Pos)
    u->PSEL.RXD = (uint32_t)-1;
    u->PSEL.TXD = (uint32_t)-1;
    u->PSEL.CTS = (uint32_t)-1;
    u->PSEL.RTS = (uint32_t)-1;
#else
    u->PSELRXD = (uint32_t)-1;
    u->PSELTXD = (uint32_t)-1;
    u->PSELCTS = (uint32_t)-1;;
    u->PSELRTS = (uint32_t)-1;
#endif
    return 0;
}
