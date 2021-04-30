/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "uart_hal.h"
#include "gpio_driver.h"
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_usart.h"
#include "port_defs.h"
#include "board_common.h"

#if defined(USART1)
#define UART_COUNT_1 1
#else
#define UART_COUNT_1 0
#endif
#if defined(USART2)
#define UART_COUNT_2 1
#else
#define UART_COUNT_2 0
#endif
#if defined(USART3)
#define UART_COUNT_3 1
#else
#define UART_COUNT_3 0
#endif
#if defined(UART4)
#define UART_COUNT_4 1
#else
#define UART_COUNT_4 0
#endif
#if defined(UART5)
#define UART_COUNT_5 1
#else
#define UART_COUNT_5 0
#endif
#if defined(USART6)
#define UART_COUNT_6 1
#else
#define UART_COUNT_6 0
#endif
#if defined(UART7)
#define UART_COUNT_7 1
#else
#define UART_COUNT_7 0
#endif
#if defined(UART8)
#define UART_COUNT_8 1
#else
#define UART_COUNT_8 0
#endif

#define UART_COUNT (UART_COUNT_1+UART_COUNT_2+UART_COUNT_3+UART_COUNT_4+UART_COUNT_5+UART_COUNT_6+UART_COUNT_7+UART_COUNT_8)

static USART_TypeDef * const phy_block[1+UART_COUNT] = {
    (void *)0,
#ifdef USART1
    USART1,
#endif
#ifdef USART2
    USART2,
#endif
#ifdef USART3
    USART3,
#endif
#ifdef UART4
    UART4,
#endif
#ifdef UART5
    UART5,
#endif
#ifdef USART6
    USART6,
#endif
#ifdef UART7
    UART7,
#endif
#ifdef UART8
    UART8,
#endif
};

#ifdef CONFIG_UART_STM32_RX_INTERRUPT
static IRQn_Type const phy_irqn[1+UART_COUNT] = {
    (IRQn_Type)0,
#ifdef USART1
    USART1_IRQn,
#endif
#ifdef USART2
    USART2_IRQn,
#endif
#ifdef USART3
    USART3_IRQn,
#endif
#ifdef UART4
    UART4_IRQn,
#endif
#ifdef UART5
    UART5_IRQn,
#endif
#ifdef USART6
    USART6_IRQn,
#endif
#ifdef UART7
    UART7_IRQn,
#endif
#ifdef UART8
    UART8_IRQn,
#endif
};
#endif

static uint8_t loghdl2phyix[UART_COUNT] = { 0 };

typedef struct {
    uint8_t phy_hdl;
    union {
        struct {
            uint16_t rx, tx, cts, rts;
        } pin;
        uint16_t pins[4];
    };
    union {
        struct {
            uint16_t rx, tx, cts, rts;
        } af;
        uint16_t afs[4];
    };
} uart_phy_pin_cfg_t;

#define PHYIX_U1 1
#define PHYIX_U2 2
#define PHYIX_U3 3
#define PHYIX_U4 4
#define PHYIX_U5 5
#define PHYIX_U6 6
#define PHYIX_U7 7
#define PHYIX_U8 8

static const uart_phy_pin_cfg_t defs[] = {
#ifdef USART1
    {.phy_hdl = PHYIX_U1,
     .pin.rx = PA(10), .pin.tx = PA(9),  .pin.cts = PA(11), .pin.rts = PA(12),
     .af.rx  = 7,      .af.tx  = 7,      .af.cts  = 7,      .af.rts  = 7,
    },
    {.phy_hdl = PHYIX_U1,
     .pin.rx = PB(7),  .pin.tx = PB(5),  .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx  = 7,      .af.tx  = 7,      .af.cts  = 0,      .af.rts  = 0,
    },
#endif
#ifdef USART2
    {.phy_hdl = PHYIX_U2,
     .pin.rx = PA(3), .pin.tx = PA(2),  .pin.cts = PA(0), .pin.rts = PA(1),
     .af.rx  = 7,      .af.tx  = 7,      .af.cts  = 7,      .af.rts  = 7,
    },
    {.phy_hdl = PHYIX_U2,
     .pin.rx = PD(6), .pin.tx = PD(5),  .pin.cts = PD(3), .pin.rts = PD(4),
     .af.rx  = 7,      .af.tx  = 7,      .af.cts  = 7,      .af.rts  = 7,
    },
#endif
#ifdef USART3
    {.phy_hdl = PHYIX_U3,
     .pin.rx = PB(11), .pin.tx = PB(10),  .pin.cts = PB(13), .pin.rts = PB(14),
     .af.rx  = 7,      .af.tx  = 7,      .af.cts  = 7,      .af.rts  = 7,
    },
    {.phy_hdl = PHYIX_U3,
     .pin.rx = PC(11), .pin.tx = PC(10),  .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx  = 7,      .af.tx  = 7,      .af.cts  = 0,      .af.rts  = 0,
    },
#endif
#ifdef UART4
    {.phy_hdl = PHYIX_U4,
     .pin.rx = PA(1), .pin.tx = PA(0),  .pin.cts = PB(0), .pin.rts = PA(15),
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 8,      .af.rts  = 8,
    },
    {.phy_hdl = PHYIX_U4,
     .pin.rx = PC(11), .pin.tx = PC(10),  .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 0,      .af.rts  = 0,
    },
#endif
#ifdef UART5
    {.phy_hdl = PHYIX_U5,
     .pin.rx = PD(2), .pin.tx = PC(12),  .pin.cts = PC(9), .pin.rts = PC(8),
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 7,      .af.rts  = 7,
    },
#endif
#ifdef USART6
    {.phy_hdl = PHYIX_U6,
     .pin.rx = PC(7), .pin.tx = PC(6),  .pin.cts = PG(13), .pin.rts = PG(8),
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 8,      .af.rts  = 8,
    },
    {.phy_hdl = PHYIX_U6,
     .pin.rx = PG(9), .pin.tx = PG(14),  .pin.cts = PG(15), .pin.rts = PG(12),
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 8,      .af.rts  = 8,
    },
#endif
#ifdef UART7
    {.phy_hdl = PHYIX_U7,
     .pin.rx = PE(7), .pin.tx = PE(8),  .pin.cts = PE(10), .pin.rts =PE(9),
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 8,      .af.rts  = 8,
    },
    {.phy_hdl = PHYIX_U7,
     .pin.rx = PF(6), .pin.tx = PF(7),  .pin.cts = PF(9), .pin.rts =PF(8),
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 8,      .af.rts  = 8,
    },
#endif
#ifdef UART8
    {.phy_hdl = PHYIX_U8,
     .pin.rx = PE(0), .pin.tx = PE(1),  .pin.cts = PD(14), .pin.rts =PD(15),
     .af.rx  = 8,      .af.tx  = 8,      .af.cts  = 8,      .af.rts  = 8,
    },
#endif
};

static int pins_to_phy_hdl(uart_phy_pin_cfg_t *cfg) {
    uint32_t ix;
    uint32_t pix;
    uint32_t user_pin_defs = 0;
    cfg->phy_hdl = (uint8_t)-1;
    for (pix = 0; pix < 4; pix++) {
        if (cfg->pins[pix] != BOARD_PIN_UNDEF) user_pin_defs++;
    }
    if (user_pin_defs == 0) {
        return ERR_UART_CONFIG;
    }
    uint8_t cur_phy_hdl = (uint8_t)-1;
    uint32_t pin_hits = 0;
    for (ix = 0; ix < sizeof(defs)/sizeof(defs[0]); ix++) {
        const uart_phy_pin_cfg_t *ref = &defs[ix];
        if (cur_phy_hdl != ref->phy_hdl) {
            cur_phy_hdl = ref->phy_hdl;
            pin_hits = 0;
        }
        for (pix = 0; pix < 4; pix++) {
            if (cfg->pins[pix] != BOARD_PIN_UNDEF &&
                ref->pins[pix] == cfg->pins[pix]) {
                pin_hits++;
                cfg->afs[pix] = ref->afs[pix];
            }
        }
        if (pin_hits == user_pin_defs) {
            cfg->phy_hdl = cur_phy_hdl;
            break;
        }
    }
    if (cfg->phy_hdl == (uint8_t)-1) {
        return ERR_UART_CONFIG;
    }
    return 0;
}

extern void gpio_hal_stm32f7_af(uint16_t pin, uint8_t af);

int uart_hal_init(unsigned int hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    if (loghdl2phyix[hdl] != 0) return ERR_UART_BUSY;

    uart_phy_pin_cfg_t phy_cfg = {
        .pin.rx = rx_pin,
        .pin.tx = tx_pin,
        .pin.rts = rts_pin,
        .pin.cts = cts_pin
    };

    int res = pins_to_phy_hdl(&phy_cfg);

    if (res) return res;

    if (config->flowcontrol == UART_FLOWCONTROL_NONE) {
        rts_pin = cts_pin = BOARD_PIN_UNDEF;
    }

    switch (phy_cfg.phy_hdl) {
#       ifdef USART1
        case PHYIX_U1:
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
            break;
#       endif
#       ifdef USART2
        case PHYIX_U2:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
            break;
#       endif
#       ifdef USART3
        case PHYIX_U3:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
            break;
#       endif
#       ifdef UART4
        case PHYIX_U4:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART4);
            break;
#       endif
#       ifdef UART5
        case PHYIX_U5:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5);
            break;
#       endif
#       ifdef USART6
        case PHYIX_U6:
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6);
            break;
#       endif
#       ifdef UART7
        case PHYIX_U7:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART7);
            break;
#       endif
#       ifdef UART8
        case PHYIX_U8:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART8);
            break;
#       endif
    }
    if (phy_cfg.pin.rx != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.rx, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
         gpio_hal_stm32f7_af(phy_cfg.pin.rx, phy_cfg.af.rx);
    }
    if (res == 0 && phy_cfg.pin.tx != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.tx, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
         gpio_hal_stm32f7_af(phy_cfg.pin.tx, phy_cfg.af.tx);
    }
    if (res == 0 && phy_cfg.pin.cts != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.cts, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
         gpio_hal_stm32f7_af(phy_cfg.pin.cts, phy_cfg.af.cts);
    }
    if (res == 0 && phy_cfg.pin.rts != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.rts, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
         gpio_hal_stm32f7_af(phy_cfg.pin.rts, phy_cfg.af.rts);
    }

    USART_TypeDef *u = phy_block[phy_cfg.phy_hdl];
    if (res == 0) {
        uint32_t transfer_direction;
        if (phy_cfg.pin.rx == BOARD_PIN_UNDEF) {
            if (phy_cfg.pin.tx == BOARD_PIN_UNDEF) {
                transfer_direction = LL_USART_DIRECTION_NONE;
            } else {
                transfer_direction = LL_USART_DIRECTION_TX;
            }
        } else {
            if (phy_cfg.pin.tx == BOARD_PIN_UNDEF) {
                transfer_direction = LL_USART_DIRECTION_RX;
            } else {
                transfer_direction = LL_USART_DIRECTION_TX_RX;
            }
        }
        uint32_t parity, stopbits;
        switch (config->parity) {
            case UART_PARITY_EVEN:  parity = LL_USART_PARITY_EVEN; break;
            case UART_PARITY_ODD:   parity = LL_USART_PARITY_ODD; break;
            default:                parity = LL_USART_PARITY_NONE; break;
        }
        stopbits = config->stopbits == UART_STOPBITS_2 ? LL_USART_STOPBITS_2 : LL_USART_STOPBITS_1;

        uint32_t hw_flowctrl = LL_USART_HWCONTROL_NONE;
        if (config->flowcontrol == UART_FLOWCONTROL_RTSCTS) {
            if (phy_cfg.pin.rts == BOARD_PIN_UNDEF) {
                if (phy_cfg.pin.cts == BOARD_PIN_UNDEF) {
                    hw_flowctrl = LL_USART_HWCONTROL_NONE;
                } else {
                    hw_flowctrl = LL_USART_HWCONTROL_CTS;
                }
            } else {
                if (phy_cfg.pin.cts == BOARD_PIN_UNDEF) {
                    hw_flowctrl = LL_USART_HWCONTROL_RTS;
                } else {
                    hw_flowctrl = LL_USART_HWCONTROL_RTS_CTS;
                }
            }
        }

        uint32_t baudrate;
        switch (config->baudrate) {
            case UART_BAUDRATE_9600:    baudrate = 9600; break;
            case UART_BAUDRATE_57600:   baudrate = 57600; break;
            case UART_BAUDRATE_460800:  baudrate = 460800; break;
            case UART_BAUDRATE_921600:  baudrate = 921600; break;
            case UART_BAUDRATE_1000000: baudrate = 1000000; break;
            default:                    baudrate = 115200; break;
        }

        LL_USART_InitTypeDef usart_config = {
            .BaudRate = baudrate,
            .DataWidth = LL_USART_DATAWIDTH_8B,
            .StopBits = stopbits,
            .Parity = parity,
            .TransferDirection = transfer_direction,
            .HardwareFlowControl = hw_flowctrl,
            .OverSampling = LL_USART_OVERSAMPLING_8
        };
        res = LL_USART_Init(u, &usart_config);

        if (res == 0) {
#           ifdef CONFIG_UART_STM32_RX_INTERRUPT
            LL_USART_EnableIT_RXNE(u);
            NVIC_ClearPendingIRQ(phy_irqn[phy_cfg.phy_hdl]);
            NVIC_EnableIRQ(phy_irqn[phy_cfg.phy_hdl]);
#           endif
            LL_USART_Enable(u);
        }
    }

    if (res == 0) {
        loghdl2phyix[hdl] = phy_cfg.phy_hdl;
    }

    return res;
}

int uart_hal_tx(unsigned int hdl, char x) {
    if (loghdl2phyix[hdl]) {
        USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];
        while (LL_USART_IsActiveFlag_TXE(u) == 0);
        LL_USART_TransmitData8(u, (uint8_t)x);
        return 0;
    } else {
        return ERR_UART_NOINIT;
    }
}

int uart_hal_rx(unsigned int hdl) {
    if (loghdl2phyix[hdl]) {
        USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];
        LL_USART_ClearFlag_ORE(u);
        LL_USART_ClearFlag_FE(u);
        while (LL_USART_IsActiveFlag_RXNE(u) == 0);
        return LL_USART_ReceiveData8(u);
    } else {
        return ERR_UART_NOINIT;
    }
}

int uart_hal_rxpoll(unsigned int hdl) {
    if (loghdl2phyix[hdl]) {
        USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];
        LL_USART_ClearFlag_ORE(u);
        LL_USART_ClearFlag_FE(u);
        if (LL_USART_IsActiveFlag_RXNE(u) == 0) {
            return -1;
        } else {
            return LL_USART_ReceiveData8(u);
        }
    } else {
        return ERR_UART_NOINIT;
    }
}

int uart_hal_deinit(unsigned int hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    if (loghdl2phyix[hdl] == 0) return ERR_UART_NOINIT;

    USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];

    LL_USART_Disable(u);

#   ifdef CONFIG_UART_STM32_RX_INTERRUPT
    NVIC_DisableIRQ(phy_irqn[loghdl2phyix[hdl]]);
    NVIC_ClearPendingIRQ(phy_irqn[loghdl2phyix[hdl]]);
#   endif

    switch (loghdl2phyix[hdl]) {
#       ifdef USART1
        case PHYIX_U1:
            LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_USART1);
            break;
#       endif
#       ifdef USART2
        case PHYIX_U2:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART2);
            break;
#       endif
#       ifdef USART3
        case PHYIX_U3:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART3);
            break;
#       endif
#       ifdef UART4
        case PHYIX_U4:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART4);
            break;
#       endif
#       ifdef UART5
        case PHYIX_U5:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART5);
            break;
#       endif
#       ifdef USART6
        case PHYIX_U6:
            LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_USART6);
            break;
#       endif
#       ifdef UART7
        case PHYIX_U7:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART7);
            break;
#       endif
#       ifdef UART8
        case PHYIX_U8:
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART8);
            break;
#       endif

    }
    if (rx_pin != BOARD_PIN_UNDEF) (void)gpio_config(rx_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    if (tx_pin != BOARD_PIN_UNDEF) (void)gpio_config(tx_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    if (rts_pin != BOARD_PIN_UNDEF) (void)gpio_config(rts_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    if (cts_pin != BOARD_PIN_UNDEF) (void)gpio_config(cts_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);

    loghdl2phyix[hdl] = 0;
    return 0;
}

#ifdef CONFIG_UART_STM32_RX_INTERRUPT
void uart_irq_rxchar_stm32f7(unsigned int hdl, char x);
__attribute__(( weak )) void uart_irq_rxchar_stm32f7(unsigned int hdl, char x) {}

static void stm32f7_uart_irq(int phy_hdl_ix) {
    USART_TypeDef *u = phy_block[phy_hdl_ix];
    NVIC_ClearPendingIRQ(phy_irqn[phy_hdl_ix]);
    uint8_t hdl;
    for (hdl = 0; hdl < UART_COUNT; hdl++) {
        if (loghdl2phyix[hdl] == phy_hdl_ix) {
            break;
        }
    }
    if (LL_USART_IsActiveFlag_RXNE(u)) {
         // cleared by reading byte
        uart_irq_rxchar_stm32f7(hdl, LL_USART_ReceiveData8(u));
    }
}

#if defined(USART1)
void USART1_IRQHandler(void);
void USART1_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U1);
}
#endif
#if defined(USART2)
void USART2_IRQHandler(void);
void USART2_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U2);
}
#endif
#if defined(USART3)
void USART3_IRQHandler(void);
void USART3_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U3);
}
#endif
#if defined(UART4)
void UART4_IRQHandler(void);
void UART4_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U4);
}
#endif
#if defined(UART5)
void UART5_IRQHandler(void);
void UART5_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U5);
}
#endif
#if defined(USART6)
void USART6_IRQHandler(void);
void USART6_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U6);
}
#endif
#if defined(UART7)
void UART7_IRQHandler(void);
void UART7_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U7);
}
#endif
#if defined(UART8)
void UART8_IRQHandler(void);
void UART8_IRQHandler(void) {
    stm32f7_uart_irq(PHYIX_U8);
}
#endif

#endif
