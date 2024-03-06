/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "gpio_hal.h"
#include "nrf.h"
#if defined(NRF52805_XXAA)
#include "nrf52805_peripherals.h"
#define NRF_P1      (void*)0
#define P0_PIN_NUM  32
#elif defined(NRF52810_XXAA)
#include "nrf52810_peripherals.h"
#define NRF_P1      (void*)0
#define P0_PIN_NUM  32
#elif defined(NRF52811_XXAA)
#include "nrf52811_peripherals.h"
#define NRF_P1      (void*)0
#define P0_PIN_NUM  32
#elif defined(NRF52810_XXAA)
#include "nrf52810_peripherals.h"
#elif defined(NRF52833_XXAA)
#include "nrf52833_peripherals.h"
#elif defined(NRF52840_XXAA)
#include "nrf52840_peripherals.h"
#else
#define NRF_P1      (void*)0
#define P0_PIN_NUM  32
#endif

static NRF_GPIO_Type *port_for_pin(uint16_t pin) {
    return pin < P0_PIN_NUM ? NRF_P0 : NRF_P1;
}

int gpio_hal_init(void) {
    return 0;
}

int gpio_hal_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull) {
    NRF_GPIO_Type *port = port_for_pin(pin);
    pin &= (P0_PIN_NUM-1);
    int is_out = dir == GPIO_DIRECTION_OUTPUT || dir == GPIO_DIRECTION_FUNCTION_OUT;
    int is_func = dir == GPIO_DIRECTION_FUNCTION_IN || dir == GPIO_DIRECTION_FUNCTION_OUT;
    port->PIN_CNF[pin] = 0
        | ((is_out ? GPIO_PIN_CNF_DIR_Output : GPIO_PIN_CNF_DIR_Input)                                  << GPIO_PIN_CNF_DIR_Pos)
        | ((!is_out ? GPIO_PIN_CNF_INPUT_Connect : GPIO_PIN_CNF_INPUT_Disconnect)                       << GPIO_PIN_CNF_INPUT_Pos)
        | ((pull == GPIO_PULL_NONE ? GPIO_PIN_CNF_PULL_Disabled :
              (pull == GPIO_PULL_DOWN ? GPIO_PIN_CNF_PULL_Pulldown : GPIO_PIN_CNF_PULL_Pullup))         << GPIO_PIN_CNF_PULL_Pos)
        | ((is_func ? GPIO_PIN_CNF_DRIVE_H0H1 : GPIO_PIN_CNF_DRIVE_S0S1)                                << GPIO_PIN_CNF_DRIVE_Pos)
        | (GPIO_PIN_CNF_SENSE_Disabled                                                                  << GPIO_PIN_CNF_SENSE_Pos)
        ;
    return 0;
}

int gpio_hal_disconnect(uint16_t pin, gpio_pull_t pull)
{
    NRF_GPIO_Type *port = port_for_pin(pin);
    pin &= (P0_PIN_NUM - 1);
    port->PIN_CNF[pin] =
        (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
        (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
        ((pull == GPIO_PULL_NONE ? GPIO_PIN_CNF_PULL_Disabled : (pull == GPIO_PULL_DOWN ? GPIO_PIN_CNF_PULL_Pulldown : GPIO_PIN_CNF_PULL_Pullup)) << GPIO_PIN_CNF_PULL_Pos) |
        (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
        (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
    return 0;
}

int gpio_hal_set(uint16_t pin, uint8_t state) {
    NRF_GPIO_Type *port = port_for_pin(pin);
    pin &= (P0_PIN_NUM-1);
    if (state) {
        port->OUTSET = (uint32_t)(1UL << pin);
    } else {
        port->OUTCLR = (uint32_t)(1UL << pin);
    }
    return 0;
}

int gpio_hal_read(uint16_t pin) {
    NRF_GPIO_Type *port = port_for_pin(pin);
    pin &= (P0_PIN_NUM-1);
    return ((port->IN >> pin) & 1) != 0;
}

#if CONFIG_GPIO_IRQ == 1

static struct
{
    uint16_t pin;
    // if cb == NULL, this GPIOTE slot is free
    gpio_irq_cb_t cb;
} pin_irq_cb[8];

int gpio_hal_irq_callback(uint16_t pin, gpio_irq_cb_t callback)
{
    pin &= 0x3f;
    if (callback)
    {
        for (int i = 0; i < 7; i++)
        {
            if (pin_irq_cb[i].cb == 0)
            {
                int enable_nvic_irq = (NRF_GPIOTE->INTENSET == 0);
                NRF_GPIOTE->EVENTS_IN[i] = 0;
                NRF_GPIOTE->CONFIG[i] =
                    (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
                    (pin << GPIOTE_CONFIG_PSEL_Pos) |
                    (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
                pin_irq_cb[i].pin = pin;
                pin_irq_cb[i].cb = callback;
                NRF_GPIOTE->INTENSET = (1 << i);
                if (enable_nvic_irq)
                {
                    NVIC_EnableIRQ(GPIOTE_IRQn);
                }
                return 0;
            }
        }
    }
    else
    {
        for (int i = 0; i < 7; i++)
        {
            if (pin_irq_cb[i].pin == pin && pin_irq_cb[i].cb)
            {
                NRF_GPIOTE->INTENCLR = (1 << i);
                if (NRF_GPIOTE->INTENSET == 0)
                {
                    NVIC_DisableIRQ(GPIOTE_IRQn);
                }
                NRF_GPIOTE->EVENTS_IN[i] = 0;
                NRF_GPIOTE->CONFIG[i] = 0;
                pin_irq_cb[i].pin = GPIO_PIN_UNDEF;
                pin_irq_cb[i].cb = 0;
                return 0;
            }
        }
    }
    return -1;
}

int gpio_hal_deinit(void)
{
    for (int i = 0; i < 8; i++)
    {
        NRF_GPIOTE->EVENTS_IN[i] = 0;
        NRF_GPIOTE->CONFIG[i] = 0;
        pin_irq_cb[i].cb = 0;
    }
    NRF_GPIOTE->INTENCLR = 0xffffffff;
    NVIC_DisableIRQ(GPIOTE_IRQn);
    return 0;
}

void GPIOTE_IRQHandler(void);
void GPIOTE_IRQHandler(void)
{
    for (int i = 0; i < 8; i++)
    {
        if (NRF_GPIOTE->EVENTS_IN[i])
        {
            NRF_GPIOTE->EVENTS_IN[i] = 0;
            if (pin_irq_cb[i].cb)
            {
                pin_irq_cb[i].cb(pin_irq_cb[i].pin, gpio_hal_read(pin_irq_cb[i].pin));
            }
        }
    }
}

#else

int gpio_hal_irq_callback(uint16_t pin, gpio_irq_cb_t callback)
{
    return -1;
}

int gpio_hal_deinit(void)
{
    return 0;
}

#endif
