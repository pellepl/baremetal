/* Copyright (c) 2025 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "gpio_hal.h"
#include "stm32l0xx_ll_bus.h"
#include "stm32l0xx_ll_gpio.h"

int gpio_hal_init(void)
{
    LL_IOP_GRP1_EnableClock(
        LL_IOP_GRP1_PERIPH_GPIOA |
        LL_IOP_GRP1_PERIPH_GPIOB |
        LL_IOP_GRP1_PERIPH_GPIOC |
#if defined(GPIOD)
        LL_IOP_GRP1_PERIPH_GPIOD |
#endif
#if defined(GPIOE)
        LL_IOP_GRP1_PERIPH_GPIOE |
#endif
#if defined(GPIOF)
        LL_IOP_GRP1_PERIPH_GPIOF |
#endif
#if defined(GPIOG)
        LL_IOP_GRP1_PERIPH_GPIOG |
#endif
#if defined(GPIOH)
        LL_IOP_GRP1_PERIPH_GPIOH |
#endif
        0);
    return 0;
}

static GPIO_TypeDef *const ports[] = {
    GPIOA,
    GPIOB,
    GPIOC,
#if defined(GPIOD)
    GPIOD,
#endif
#if defined(GPIOE)
    GPIOE,
#endif
#if defined(GPIOF)
    GPIOF,
#endif
#if defined(GPIOG)
    GPIOG,
#endif
#if defined(GPIOH)
    GPIOH,
#endif
};

static const uint32_t pins[] = {
    LL_GPIO_PIN_0,
    LL_GPIO_PIN_1,
    LL_GPIO_PIN_2,
    LL_GPIO_PIN_3,
    LL_GPIO_PIN_4,
    LL_GPIO_PIN_5,
    LL_GPIO_PIN_6,
    LL_GPIO_PIN_7,
    LL_GPIO_PIN_8,
    LL_GPIO_PIN_9,
    LL_GPIO_PIN_10,
    LL_GPIO_PIN_11,
    LL_GPIO_PIN_12,
    LL_GPIO_PIN_13,
    LL_GPIO_PIN_14,
    LL_GPIO_PIN_15,
};

static GPIO_TypeDef *port_for_pin(uint16_t pin)
{
    return ports[pin >> 4];
}

int gpio_hal_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull)
{
    GPIO_TypeDef *port = port_for_pin(pin);
    uint32_t ll_pin = pins[pin & 0xf];
    uint32_t ll_mode = (uint32_t[5]){
        LL_GPIO_MODE_INPUT,
        LL_GPIO_MODE_OUTPUT,
        LL_GPIO_MODE_ANALOG,
        LL_GPIO_MODE_ALTERNATE,
        LL_GPIO_MODE_ALTERNATE,
    }[dir];
    uint32_t ll_output =
        (dir == GPIO_DIRECTION_OUTPUT || dir == GPIO_DIRECTION_FUNCTION_OUT) ? (pull == GPIO_PULL_DOWN ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL) : LL_GPIO_OUTPUT_OPENDRAIN;
    LL_GPIO_SetPinOutputType(port, ll_pin, ll_output);
    LL_GPIO_SetPinPull(port, ll_pin, pull == GPIO_PULL_DOWN ? LL_GPIO_PULL_DOWN : LL_GPIO_PULL_UP);
    LL_GPIO_SetPinSpeed(port, ll_pin, ll_mode == LL_GPIO_MODE_ALTERNATE ? LL_GPIO_SPEED_FREQ_HIGH : LL_GPIO_SPEED_MEDIUM);
    LL_GPIO_SetPinMode(port, ll_pin, ll_mode);
    return 0;
}

int gpio_hal_set(uint16_t pin, uint8_t state)
{
    GPIO_TypeDef *port = port_for_pin(pin);
    uint32_t ll_pin = pins[pin & 0xf];
    if (state)
    {
        LL_GPIO_SetOutputPin(port, ll_pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(port, ll_pin);
    }
    return 0;
}

int gpio_hal_read(uint16_t pin)
{
    GPIO_TypeDef *port = port_for_pin(pin);
    uint32_t ll_pin = pins[pin & 0xf];
    return LL_GPIO_IsInputPinSet(port, ll_pin);
}

int gpio_hal_deinit(void)
{
    LL_IOP_GRP1_DisableClock(
        LL_IOP_GRP1_PERIPH_GPIOA |
        LL_IOP_GRP1_PERIPH_GPIOB |
        LL_IOP_GRP1_PERIPH_GPIOC |
#if defined(GPIOD)
        LL_IOP_GRP1_PERIPH_GPIOD |
#endif
#if defined(GPIOE)
        LL_IOP_GRP1_PERIPH_GPIOE |
#endif
#if defined(GPIOF)
        LL_IOP_GRP1_PERIPH_GPIOF |
#endif
#if defined(GPIOG)
        LL_IOP_GRP1_PERIPH_GPIOG |
#endif
#if defined(GPIOH)
        LL_IOP_GRP1_PERIPH_GPIOH |
#endif
        0);
    return 0;
}

void gpio_hal_stm32l0_af(uint16_t pin, uint8_t af);
void gpio_hal_stm32l0_af(uint16_t pin, uint8_t af)
{
    GPIO_TypeDef *port = port_for_pin(pin);
    uint32_t ll_pin = pins[pin & 0xf];
    if ((pin & 0xf) < 8)
    {
        LL_GPIO_SetAFPin_0_7(port, ll_pin, af);
    }
    else
    {
        LL_GPIO_SetAFPin_8_15(port, ll_pin, af);
    }
}
