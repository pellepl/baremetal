#include "uart_hal.h"
#include "gpio_driver.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_usart.h"
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

#define UART_COUNT (UART_COUNT_1+UART_COUNT_2+UART_COUNT_3+UART_COUNT_4+UART_COUNT_5)

static USART_TypeDef * const phy_hdl[1+UART_COUNT] = {
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
};

static uint8_t log_hdl[UART_COUNT] = { 0 };

typedef enum {
    REMAP_NONE = 0,
    REMAP_FULL,
    REMAP_PART
} remap_t;

typedef struct {
    uint8_t phy_hdl;
    union {
        struct {
            uint16_t rx, tx, cts, rts;
        } pin;
        uint16_t pins[4];
    };
    remap_t rmp;
} uart_phy_pin_cfg_t;

#define HDL_U1 1
#define HDL_U2 2
#define HDL_U3 3
#define HDL_U4 4
#define HDL_U5 5

static const uart_phy_pin_cfg_t defs[] = {
#ifdef USART1
    {.phy_hdl = HDL_U1,
     .pin.rx = PA(10), .pin.tx = PA(9),  .pin.cts = PB(14), .pin.rts = PA(12),
     .rmp = REMAP_NONE
    },
    {.phy_hdl = HDL_U1,
     .pin.rx = PB(7),  .pin.tx = PC(6),  .pin.cts = PB(14), .pin.rts = PA(12),
     .rmp = REMAP_FULL
    },
#endif
#ifdef USART2
    {.phy_hdl = HDL_U2,
     .pin.rx = PA(3),  .pin.tx = PA(2),  .pin.cts = PA(0),  .pin.rts = PA(1),
     .rmp = REMAP_NONE
    },
    {.phy_hdl = HDL_U2,
     .pin.rx = PD(6),  .pin.tx = PD(5),  .pin.cts = PD(3),  .pin.rts = PD(4),
     .rmp = REMAP_FULL
    },
#endif
#ifdef USART3
    {.phy_hdl = HDL_U3,
     .pin.rx = PB(11), .pin.tx = PB(10), .pin.cts = PB(13), .pin.rts = PB(14),
     .rmp = REMAP_NONE
    },
    {.phy_hdl = HDL_U3,
     .pin.rx = PD(9),  .pin.tx = PD(8),  .pin.cts = PD(11), .pin.rts = PD(12),
     .rmp = REMAP_FULL
    },
    {.phy_hdl = HDL_U3,
     .pin.rx = PC(11), .pin.tx = PC(19), .pin.cts = PB(13), .pin.rts = PB(14),
     .rmp = REMAP_PART
    },
#endif
#ifdef UART4
    {.phy_hdl = HDL_U4,
     .pin.rx = PC(11), .pin.tx = PC(10), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .rmp = REMAP_NONE
    },
#endif
#ifdef UART5
    {.phy_hdl = HDL_U5,
     .pin.rx = PD(2),  .pin.tx = PC(12), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .rmp = REMAP_NONE
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
    for (ix = 0; ix < sizeof(defs)/sizeof(defs[0]); ix++) {
        uart_phy_pin_cfg_t ref = defs[ix];
        uint32_t pin_hits = 0;
        for (pix = 0; pix < 4; pix++) {
            if (cfg->pins[pix] != BOARD_PIN_UNDEF &&
                ref.pins[pix] == cfg->pins[pix]) {
                pin_hits++;
            }
        }
        if (pin_hits == user_pin_defs) {
            cfg->phy_hdl = ref.phy_hdl;
            cfg->rmp = ref.rmp;
            break;
        }
    }
    if (cfg->phy_hdl == (uint8_t)-1) {
        return ERR_UART_CONFIG;
    }
    return 0;
}

int uart_hal_init(uint32_t hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    if (log_hdl[hdl] != 0) return ERR_UART_BUSY;

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
        #ifdef USART1
        case HDL_U1:
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
            switch (phy_cfg.rmp) {
                case REMAP_FULL: LL_GPIO_AF_EnableRemap_USART1(); break;
                default: LL_GPIO_AF_DisableRemap_USART1(); break;
            }
            break;
        #endif
        #ifdef USART2
        case HDL_U2:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
            switch (phy_cfg.rmp) {
                case REMAP_FULL: LL_GPIO_AF_EnableRemap_USART2(); break;
                default: LL_GPIO_AF_DisableRemap_USART2(); break;
            }
            break;
        #endif
        #ifdef USART3
        case HDL_U3:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
            switch (phy_cfg.rmp) {
                case REMAP_PART: LL_GPIO_AF_RemapPartial_USART3(); break;
                case REMAP_FULL: LL_GPIO_AF_EnableRemap_USART3(); break;
                default: LL_GPIO_AF_DisableRemap_USART3(); break;
            }
            break;
        #endif
        #ifdef UART4
        case HDL_U4:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART4);
            break;
        #endif
        #ifdef UART5
        case HDL_U5:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5);
            break;
        #endif
    }
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    if (phy_cfg.pin.rx != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.rx, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    }
    if (res == 0 && phy_cfg.pin.tx != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.tx, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
    }
    if (res == 0 && phy_cfg.pin.cts != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.cts, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    }
    if (res == 0 && phy_cfg.pin.rts != BOARD_PIN_UNDEF) {
         res = gpio_config(phy_cfg.pin.rts, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_UP);
    }

    USART_TypeDef *u = phy_hdl[phy_cfg.phy_hdl];
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
            .OverSampling = LL_USART_OVERSAMPLING_16
        };
        res = LL_USART_Init(u, &usart_config);

        if (res == 0) {
            LL_USART_Enable(u);
        }
    }

    if (res == 0) {
        log_hdl[hdl] = phy_cfg.phy_hdl;
    }

    return res;
}

int uart_hal_tx(uint32_t hdl, char x) {
    if (log_hdl[hdl]) {
        USART_TypeDef *u = phy_hdl[log_hdl[hdl]];
        while (LL_USART_IsActiveFlag_TXE(u) == 0);
        LL_USART_TransmitData8(u, (uint8_t)x);
        return 0;
    } else {
        return ERR_UART_NOINIT;
    }
}

int uart_hal_rx(uint32_t hdl) {
    if (log_hdl[hdl]) {
        USART_TypeDef *u = phy_hdl[log_hdl[hdl]];
        while (LL_USART_IsActiveFlag_RXNE(u) == 0);
        return LL_USART_ReceiveData8(u);
    } else {
        return ERR_UART_NOINIT;
    }
}

int uart_hal_rxpoll(uint32_t hdl) {
    if (log_hdl[hdl]) {
        USART_TypeDef *u = phy_hdl[log_hdl[hdl]];
        if (LL_USART_IsActiveFlag_RXNE(u) == 0) {
            return LL_USART_ReceiveData8(u);
        } else {
            return -1;
        }
    } else {
        return ERR_UART_NOINIT;
    }
}

int uart_hal_deinit(uint32_t hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    // TODO PETER
    log_hdl[hdl] = 0;
    return 0;
}