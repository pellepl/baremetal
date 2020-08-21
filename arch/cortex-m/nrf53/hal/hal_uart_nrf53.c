/* Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "uart_hal.h"
#include "nrf.h"
#include "hal_peripherals_nrf53.h"
#include "gpio_driver.h"


// TODO PETER needs 53-specific love

// UART seems deprecated in 53 in favour for UARTE, but it is still same HW block. 
// Avoid enforcing the DMA in favour for KISS.
typedef struct {                                /*!< (@ 0x40002000) UART0 Structure                                            */
  __OM  uint32_t  TASKS_STARTRX;                /*!< (@ 0x00000000) Start UART receiver                                        */
  __OM  uint32_t  TASKS_STOPRX;                 /*!< (@ 0x00000004) Stop UART receiver                                         */
  __OM  uint32_t  TASKS_STARTTX;                /*!< (@ 0x00000008) Start UART transmitter                                     */
  __OM  uint32_t  TASKS_STOPTX;                 /*!< (@ 0x0000000C) Stop UART transmitter                                      */
  __IM  uint32_t  RESERVED[3];
  __OM  uint32_t  TASKS_SUSPEND;                /*!< (@ 0x0000001C) Suspend UART                                               */
  __IM  uint32_t  RESERVED1[56];
  __IOM uint32_t  EVENTS_CTS;                   /*!< (@ 0x00000100) CTS is activated (set low). Clear To Send.                 */
  __IOM uint32_t  EVENTS_NCTS;                  /*!< (@ 0x00000104) CTS is deactivated (set high). Not Clear To Send.          */
  __IOM uint32_t  EVENTS_RXDRDY;                /*!< (@ 0x00000108) Data received in RXD                                       */
  __IM  uint32_t  RESERVED2[4];
  __IOM uint32_t  EVENTS_TXDRDY;                /*!< (@ 0x0000011C) Data sent from TXD                                         */
  __IM  uint32_t  RESERVED3;
  __IOM uint32_t  EVENTS_ERROR;                 /*!< (@ 0x00000124) Error detected                                             */
  __IM  uint32_t  RESERVED4[7];
  __IOM uint32_t  EVENTS_RXTO;                  /*!< (@ 0x00000144) Receiver timeout                                           */
  __IM  uint32_t  RESERVED5[46];
  __IOM uint32_t  SHORTS;                       /*!< (@ 0x00000200) Shortcut register                                          */
  __IM  uint32_t  RESERVED6[64];
  __IOM uint32_t  INTENSET;                     /*!< (@ 0x00000304) Enable interrupt                                           */
  __IOM uint32_t  INTENCLR;                     /*!< (@ 0x00000308) Disable interrupt                                          */
  __IM  uint32_t  RESERVED7[93];
  __IOM uint32_t  ERRORSRC;                     /*!< (@ 0x00000480) Error source                                               */
  __IM  uint32_t  RESERVED8[31];
  __IOM uint32_t  ENABLE;                       /*!< (@ 0x00000500) Enable UART                                                */
  __IM  uint32_t  RESERVED9;
  __IOM uint32_t  PSELRTS;                      /*!< (@ 0x00000508) Pin select for RTS                                         */
  __IOM uint32_t  PSELTXD;                      /*!< (@ 0x0000050C) Pin select for TXD                                         */
  __IOM uint32_t  PSELCTS;                      /*!< (@ 0x00000510) Pin select for CTS                                         */
  __IOM uint32_t  PSELRXD;                      /*!< (@ 0x00000514) Pin select for RXD                                         */
  __IM  uint32_t  RXD;                          /*!< (@ 0x00000518) RXD register                                               */
  __OM  uint32_t  TXD;                          /*!< (@ 0x0000051C) TXD register                                               */
  __IM  uint32_t  RESERVED10;
  __IOM uint32_t  BAUDRATE;                     /*!< (@ 0x00000524) Baud rate                                                  */
  __IM  uint32_t  RESERVED11[17];
  __IOM uint32_t  CONFIG;                       /*!< (@ 0x0000056C) Configuration of parity and hardware flow control          */
} NRF_UART_Type;                                /*!< Size = 1392 (0x570)                                                       */


#if defined(FIXTHISFOR53PLZPETER)
static NRF_UART_Type *hw[2] = {
    (NRF_UART_Type *)NRF_UARTE0_NS,
    (NRF_UART_Type *)NRF_UARTE1_NS
};
#else
static NRF_UART_Type *hw[1] = {
    (NRF_UART_Type *)NRF_UARTE0,
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
        case UART_BAUDRATE_9600:    u->BAUDRATE = 0x00275000; break;
        case UART_BAUDRATE_57600:   u->BAUDRATE = 0x00EBF000; break;
        case UART_BAUDRATE_115200:  u->BAUDRATE = 0x01D7E000; break;
        case UART_BAUDRATE_460800:  u->BAUDRATE = 0x075F7000; break;
        case UART_BAUDRATE_921600:  u->BAUDRATE = 0x0EBED000; break;
        case UART_BAUDRATE_1000000: u->BAUDRATE = 0x10000000; break;
    }

    u->CONFIG = 0
        | ((config->parity == UART_PARITY_NONE ? UARTE_CONFIG_PARITY_Excluded : UARTE_CONFIG_PARITY_Included) << UARTE_CONFIG_PARITY_Pos)
        | (config->flowcontrol == UART_FLOWCONTROL_NONE ? UARTE_CONFIG_HWFC_Disabled : UARTE_CONFIG_HWFC_Enabled);

#if defined(UART_PSEL_RXD_CONNECT_Pos)
    u->PSEL.RXD = rx_pin;
    u->PSEL.TXD = tx_pin;
    u->PSEL.CTS = cts_pin;
    u->PSEL.RTS = rts_pin;
#else
    u->PSELRXD = rx_pin;
    u->PSELTXD = tx_pin;
    u->PSELCTS = cts_pin;
    u->PSELRTS = rts_pin;
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

    u->ENABLE = UARTE_ENABLE_ENABLE_Enabled;

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
    u->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
#if defined(UART_PSEL_RXD_CONNECT_Pos)
    u->PSELRXD = (uint32_t)-1;
    u->PSELTXD = (uint32_t)-1;
    u->PSELCTS = (uint32_t)-1;
    u->PSELRTS = (uint32_t)-1;
#else
    u->PSELRXD = (uint32_t)-1;
    u->PSELTXD = (uint32_t)-1;
    u->PSELCTS = (uint32_t)-1;;
    u->PSELRTS = (uint32_t)-1;
#endif
    return 0;
}
