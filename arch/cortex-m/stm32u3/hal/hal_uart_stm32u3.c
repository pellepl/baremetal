/* Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "uart_hal.h"
#include "gpio_driver.h"
#include "stm32u3xx_ll_bus.h"
#include "stm32u3xx_ll_gpio.h"
#include "stm32u3xx_ll_usart.h"
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
#if defined(LPUART1)
#define UART_COUNT_LPU1 1
#else
#define UART_COUNT_LPU1 0
#endif

#define UART_COUNT (UART_COUNT_1 + UART_COUNT_2 + UART_COUNT_3 + UART_COUNT_4 + UART_COUNT_5 + UART_COUNT_LPU1)

#define PHYIX_U1 1
#define PHYIX_U2 2
#define PHYIX_U3 3
#define PHYIX_U4 4
#define PHYIX_U5 5
#define PHYIX_LPU1 6
#define PHYIX_COUNT 7

static USART_TypeDef *const phy_block[PHYIX_COUNT] = {
    (void *)0,
#ifdef USART1
    [PHYIX_U1] = USART1,
#endif
#ifdef USART2
    [PHYIX_U2] = USART2,
#endif
#ifdef USART3
    [PHYIX_U3] = USART3,
#endif
#ifdef UART4
    [PHYIX_U4] = UART4,
#endif
#ifdef UART5
    [PHYIX_U5] = UART5,
#endif
#ifdef LPUART1
    [PHYIX_LPU1] = LPUART1,
#endif
};

#ifdef CONFIG_UART_STM32_RX_INTERRUPT
static IRQn_Type const phy_irqn[PHYIX_COUNT] = {
    (IRQn_Type)0,
#if defined(USART1) && defined(USART1_IRQn)
    [PHYIX_U1] = USART1_IRQn,
#endif
#if defined(USART2) && defined(USART2_IRQn)
    [PHYIX_U2] = USART2_IRQn,
#endif
#if defined(USART3) && defined(USART3_IRQn)
    [PHYIX_U3] = USART3_IRQn,
#endif
#if defined(UART4) && defined(UART4_IRQn)
    [PHYIX_U4] = UART4_IRQn,
#endif
#if defined(UART5) && defined(UART5_IRQn)
    [PHYIX_U5] = UART5_IRQn,
#endif
#if defined(LPUART1) && defined(LPUART1_IRQn)
    [PHYIX_LPU1] = LPUART1_IRQn,
#endif
};
#endif

static uint8_t loghdl2phyix[UART_COUNT] = {0};

static void stm32u3_uart_clear_rx_errors(USART_TypeDef *u) {
    if (LL_USART_IsActiveFlag_ORE(u)) {
        LL_USART_ClearFlag_ORE(u);
    }
    if (LL_USART_IsActiveFlag_FE(u)) {
        LL_USART_ClearFlag_FE(u);
    }
    if (LL_USART_IsActiveFlag_NE(u)) {
        LL_USART_ClearFlag_NE(u);
    }
    if (LL_USART_IsActiveFlag_PE(u)) {
        LL_USART_ClearFlag_PE(u);
    }
}

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

static const uart_phy_pin_cfg_t defs[] = {
#ifdef USART1
    {.phy_hdl = PHYIX_U1,
     .pin.rx = PA(10), .pin.tx = PA(9), .pin.cts = PA(11), .pin.rts = PA(12),
     .af.rx = 7, .af.tx = 7, .af.cts = 7, .af.rts = 7},
    {.phy_hdl = PHYIX_U1,
     .pin.rx = PB(7), .pin.tx = PB(6), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx = 7, .af.tx = 7, .af.cts = 0, .af.rts = 0},
#endif
#ifdef USART2
    {.phy_hdl = PHYIX_U2,
     .pin.rx = PA(3), .pin.tx = PA(2), .pin.cts = PA(0), .pin.rts = PA(1),
     .af.rx = 7, .af.tx = 7, .af.cts = 7, .af.rts = 7},
    {.phy_hdl = PHYIX_U2,
     .pin.rx = PD(6), .pin.tx = PD(5), .pin.cts = PD(3), .pin.rts = PD(4),
     .af.rx = 7, .af.tx = 7, .af.cts = 7, .af.rts = 7},
#endif
#ifdef USART3
    {.phy_hdl = PHYIX_U3,
     .pin.rx = PB(11), .pin.tx = PB(10), .pin.cts = PB(13), .pin.rts = PB(14),
     .af.rx = 7, .af.tx = 7, .af.cts = 7, .af.rts = 7},
    {.phy_hdl = PHYIX_U3,
     .pin.rx = PC(11), .pin.tx = PC(10), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx = 7, .af.tx = 7, .af.cts = 0, .af.rts = 0},
    {.phy_hdl = PHYIX_U3,
     .pin.rx = PD(9), .pin.tx = PD(8), .pin.cts = PD(11), .pin.rts = PD(12),
     .af.rx = 7, .af.tx = 7, .af.cts = 7, .af.rts = 7},
#endif
#ifdef UART4
    {.phy_hdl = PHYIX_U4,
     .pin.rx = PA(1), .pin.tx = PA(0), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx = 8, .af.tx = 8, .af.cts = 0, .af.rts = 0},
#endif
#ifdef UART5
    {.phy_hdl = PHYIX_U5,
     .pin.rx = PD(2), .pin.tx = PC(12), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx = 8, .af.tx = 8, .af.cts = 0, .af.rts = 0},
#endif
#ifdef LPUART1
    {.phy_hdl = PHYIX_LPU1,
     .pin.rx = PA(3), .pin.tx = PA(2), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx = 8, .af.tx = 8, .af.cts = 0, .af.rts = 0},
    {.phy_hdl = PHYIX_LPU1,
     .pin.rx = PB(11), .pin.tx = PB(10), .pin.cts = BOARD_PIN_UNDEF, .pin.rts = BOARD_PIN_UNDEF,
     .af.rx = 8, .af.tx = 8, .af.cts = 0, .af.rts = 0},
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
    if (user_pin_defs == 0) return ERR_UART_CONFIG;

    for (ix = 0; ix < sizeof(defs) / sizeof(defs[0]); ix++) {
        const uart_phy_pin_cfg_t *ref = &defs[ix];
        uint32_t pin_hits = 0;
        for (pix = 0; pix < 4; pix++) {
            if (cfg->pins[pix] != BOARD_PIN_UNDEF && ref->pins[pix] == cfg->pins[pix]) {
                pin_hits++;
                cfg->afs[pix] = ref->afs[pix];
            }
        }
        if (pin_hits == user_pin_defs) {
            cfg->phy_hdl = ref->phy_hdl;
            break;
        }
    }
    if (cfg->phy_hdl == (uint8_t)-1) return ERR_UART_CONFIG;
    return 0;
}

extern void gpio_hal_stm32u3_af(uint16_t pin, uint8_t af);

static void stm32u3_uart_clock_enable(uint8_t phy_hdl) {
    switch (phy_hdl) {
#ifdef USART1
    case PHYIX_U1: LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1); break;
#endif
#ifdef USART2
    case PHYIX_U2: LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2); break;
#endif
#ifdef USART3
    case PHYIX_U3: LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3); break;
#endif
#ifdef UART4
    case PHYIX_U4: LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART4); break;
#endif
#ifdef UART5
    case PHYIX_U5: LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5); break;
#endif
#ifdef LPUART1
    case PHYIX_LPU1: LL_APB3_GRP1_EnableClock(LL_APB3_GRP1_PERIPH_LPUART1); break;
#endif
    default: break;
    }
}

static void stm32u3_uart_clock_disable(uint8_t phy_hdl) {
    switch (phy_hdl) {
#ifdef USART1
    case PHYIX_U1: LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_USART1); break;
#endif
#ifdef USART2
    case PHYIX_U2: LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART2); break;
#endif
#ifdef USART3
    case PHYIX_U3: LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART3); break;
#endif
#ifdef UART4
    case PHYIX_U4: LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART4); break;
#endif
#ifdef UART5
    case PHYIX_U5: LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_UART5); break;
#endif
#ifdef LPUART1
    case PHYIX_LPU1: LL_APB3_GRP1_DisableClock(LL_APB3_GRP1_PERIPH_LPUART1); break;
#endif
    default: break;
    }
}

int uart_hal_init(unsigned int hdl, const uart_config_t *config, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    if (hdl >= UART_COUNT) return ERR_UART_CONFIG;
    if (loghdl2phyix[hdl] != 0) return ERR_UART_BUSY;

    uart_phy_pin_cfg_t phy_cfg = {
        .pin.rx = rx_pin,
        .pin.tx = tx_pin,
        .pin.rts = rts_pin,
        .pin.cts = cts_pin
    };

    if (config->flowcontrol == UART_FLOWCONTROL_NONE) {
        phy_cfg.pin.rts = BOARD_PIN_UNDEF;
        phy_cfg.pin.cts = BOARD_PIN_UNDEF;
    }

    int res = pins_to_phy_hdl(&phy_cfg);
    if (res) return res;

    stm32u3_uart_clock_enable(phy_cfg.phy_hdl);

    if (phy_cfg.pin.rx != BOARD_PIN_UNDEF) {
        res = gpio_config(phy_cfg.pin.rx, GPIO_DIRECTION_FUNCTION_IN, CONFIG_UART_GPIO_RX_PULL_UP ? GPIO_PULL_UP : GPIO_PULL_NONE);
        gpio_hal_stm32u3_af(phy_cfg.pin.rx, phy_cfg.af.rx);
    }
    if (res == 0 && phy_cfg.pin.tx != BOARD_PIN_UNDEF) {
        res = gpio_config(phy_cfg.pin.tx, GPIO_DIRECTION_FUNCTION_OUT, CONFIG_UART_GPIO_TX_PULL_NONE ? GPIO_PULL_NONE : GPIO_PULL_UP);
        gpio_hal_stm32u3_af(phy_cfg.pin.tx, phy_cfg.af.tx);
    }
    if (res == 0 && phy_cfg.pin.cts != BOARD_PIN_UNDEF) {
        res = gpio_config(phy_cfg.pin.cts, GPIO_DIRECTION_FUNCTION_IN, CONFIG_UART_GPIO_CTS_PULL_UP ? GPIO_PULL_UP: GPIO_PULL_NONE);
        gpio_hal_stm32u3_af(phy_cfg.pin.cts, phy_cfg.af.cts);
    }
    if (res == 0 && phy_cfg.pin.rts != BOARD_PIN_UNDEF) {
        res = gpio_config(phy_cfg.pin.rts, GPIO_DIRECTION_FUNCTION_OUT, CONFIG_UART_GPIO_RTS_PULL_NONE ? GPIO_PULL_NONE: GPIO_PULL_UP);
        gpio_hal_stm32u3_af(phy_cfg.pin.rts, phy_cfg.af.rts);
    }

    USART_TypeDef *u = phy_block[phy_cfg.phy_hdl];
    if (res == 0) {
        uint32_t transfer_direction;
        if (phy_cfg.pin.rx == BOARD_PIN_UNDEF) {
            transfer_direction = phy_cfg.pin.tx == BOARD_PIN_UNDEF ? LL_USART_DIRECTION_NONE : LL_USART_DIRECTION_TX;
        } else {
            transfer_direction = phy_cfg.pin.tx == BOARD_PIN_UNDEF ? LL_USART_DIRECTION_RX : LL_USART_DIRECTION_TX_RX;
        }

        uint32_t parity;
        switch (config->parity) {
        case UART_PARITY_EVEN: parity = LL_USART_PARITY_EVEN; break;
        case UART_PARITY_ODD: parity = LL_USART_PARITY_ODD; break;
        default: parity = LL_USART_PARITY_NONE; break;
        }
        uint32_t stopbits = config->stopbits == UART_STOPBITS_2 ? LL_USART_STOPBITS_2 : LL_USART_STOPBITS_1;

        uint32_t hw_flowctrl = LL_USART_HWCONTROL_NONE;
        if (config->flowcontrol == UART_FLOWCONTROL_RTSCTS) {
            if (phy_cfg.pin.rts == BOARD_PIN_UNDEF) {
                hw_flowctrl = phy_cfg.pin.cts == BOARD_PIN_UNDEF ? LL_USART_HWCONTROL_NONE : LL_USART_HWCONTROL_CTS;
            } else {
                hw_flowctrl = phy_cfg.pin.cts == BOARD_PIN_UNDEF ? LL_USART_HWCONTROL_RTS : LL_USART_HWCONTROL_RTS_CTS;
            }
        }

        uint32_t baudrate;
        switch (config->baudrate) {
        case UART_BAUDRATE_600: baudrate = 600; break;
        case UART_BAUDRATE_1200: baudrate = 1200; break;
        case UART_BAUDRATE_2400: baudrate = 2400; break;
        case UART_BAUDRATE_4800: baudrate = 4800; break;
        case UART_BAUDRATE_9600: baudrate = 9600; break;
        case UART_BAUDRATE_57600: baudrate = 57600; break;
        case UART_BAUDRATE_460800: baudrate = 460800; break;
        case UART_BAUDRATE_921600: baudrate = 921600; break;
        case UART_BAUDRATE_1000000: baudrate = 1000000; break;
        default: baudrate = 115200; break;
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
#ifdef CONFIG_UART_STM32_RX_INTERRUPT
            LL_USART_EnableIT_RXNE(u);
            LL_USART_DisableIT_IDLE(u);
            LL_USART_DisableIT_PE(u);
            LL_USART_DisableIT_ERROR(u);
            LL_USART_DisableIT_TC(u);
            LL_USART_DisableIT_TXE(u);
            LL_USART_DisableIT_EOB(u);
            NVIC_ClearPendingIRQ(phy_irqn[phy_cfg.phy_hdl]);
            NVIC_EnableIRQ(phy_irqn[phy_cfg.phy_hdl]);
#endif
            LL_USART_Enable(u);
            while (!LL_USART_IsActiveFlag_TEACK(u) || !LL_USART_IsActiveFlag_REACK(u));
        }
    }

    if (res == 0) {
        loghdl2phyix[hdl] = phy_cfg.phy_hdl;
    }
    return res;
}

int uart_hal_tx(unsigned int hdl, char x) {
    if (hdl < UART_COUNT && loghdl2phyix[hdl]) {
        USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];
        while (LL_USART_IsActiveFlag_TXE(u) == 0);
        LL_USART_TransmitData8(u, (uint8_t)x);
        return 0;
    }
    return ERR_UART_NOINIT;
}

int uart_hal_rx(unsigned int hdl) {
    if (hdl < UART_COUNT && loghdl2phyix[hdl]) {
        USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];
        while (LL_USART_IsActiveFlag_RXNE(u) == 0) {
            stm32u3_uart_clear_rx_errors(u);
        }
        return LL_USART_ReceiveData8(u);
    }
    return ERR_UART_NOINIT;
}

int uart_hal_rxpoll(unsigned int hdl) {
    if (hdl < UART_COUNT && loghdl2phyix[hdl]) {
        USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];
        stm32u3_uart_clear_rx_errors(u);
        return LL_USART_IsActiveFlag_RXNE(u) == 0 ? -1 : LL_USART_ReceiveData8(u);
    }
    return ERR_UART_NOINIT;
}

int uart_hal_deinit(unsigned int hdl, uint16_t rx_pin, uint16_t tx_pin, uint16_t rts_pin, uint16_t cts_pin) {
    if (hdl >= UART_COUNT || loghdl2phyix[hdl] == 0) return ERR_UART_NOINIT;

    USART_TypeDef *u = phy_block[loghdl2phyix[hdl]];
    LL_USART_Disable(u);

#ifdef CONFIG_UART_STM32_RX_INTERRUPT
    NVIC_DisableIRQ(phy_irqn[loghdl2phyix[hdl]]);
    NVIC_ClearPendingIRQ(phy_irqn[loghdl2phyix[hdl]]);
#endif

    stm32u3_uart_clock_disable(loghdl2phyix[hdl]);

    if (rx_pin != BOARD_PIN_UNDEF) (void)gpio_config(rx_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    if (tx_pin != BOARD_PIN_UNDEF) (void)gpio_config(tx_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    if (rts_pin != BOARD_PIN_UNDEF) (void)gpio_config(rts_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);
    if (cts_pin != BOARD_PIN_UNDEF) (void)gpio_config(cts_pin, GPIO_DIRECTION_FUNCTION_IN, GPIO_PULL_NONE);

    loghdl2phyix[hdl] = 0;
    return 0;
}

#ifdef CONFIG_UART_STM32_RX_INTERRUPT
void uart_irq_rxchar_stm32u3(unsigned int hdl, char x);
__attribute__((weak)) void uart_irq_rxchar_stm32u3(unsigned int hdl, char x) {
    (void)hdl;
    (void)x;
}

static void stm32u3_uart_irq(int phy_hdl_ix) {
    USART_TypeDef *u = phy_block[phy_hdl_ix];
    NVIC_ClearPendingIRQ(phy_irqn[phy_hdl_ix]);
    uint8_t hdl;
    for (hdl = 0; hdl < UART_COUNT; hdl++) {
        if (loghdl2phyix[hdl] == phy_hdl_ix) break;
    }
    if (LL_USART_IsActiveFlag_RXNE(u)) {
        uart_irq_rxchar_stm32u3(hdl, LL_USART_ReceiveData8(u));
    } else {
        stm32u3_uart_clear_rx_errors(u);

        if (LL_USART_IsActiveFlag_IDLE(u)) {
            LL_USART_ClearFlag_IDLE(u);
        }

        if (LL_USART_IsActiveFlag_EOB(u)) {
            LL_USART_ClearFlag_EOB(u);
        }
    }
}

#if defined(USART1) && defined(USART1_IRQn)
void USART1_IRQHandler(void) { stm32u3_uart_irq(PHYIX_U1); }
#endif
#if defined(USART2) && defined(USART2_IRQn)
void USART2_IRQHandler(void) { stm32u3_uart_irq(PHYIX_U2); }
#endif
#if defined(USART3) && defined(USART3_IRQn)
void USART3_IRQHandler(void) { stm32u3_uart_irq(PHYIX_U3); }
#endif
#if defined(UART4) && defined(UART4_IRQn)
void UART4_IRQHandler(void) { stm32u3_uart_irq(PHYIX_U4); }
#endif
#if defined(UART5) && defined(UART5_IRQn)
void UART5_IRQHandler(void) { stm32u3_uart_irq(PHYIX_U5); }
#endif
#if defined(LPUART1) && defined(LPUART1_IRQn)
void LPUART1_IRQHandler(void) { stm32u3_uart_irq(PHYIX_LPU1); }
#endif
#endif
