#include "gpio_hal.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"

int gpio_hal_init(void) {
    LL_APB2_GRP1_EnableClock(
        LL_APB2_GRP1_PERIPH_GPIOA |
        LL_APB2_GRP1_PERIPH_GPIOB |
        LL_APB2_GRP1_PERIPH_GPIOC |
        LL_APB2_GRP1_PERIPH_GPIOD |
#if defined(GPIOE)
        LL_APB2_GRP1_PERIPH_GPIOE |
#endif
#if defined(GPIOF)
        LL_APB2_GRP1_PERIPH_GPIOF |
#endif
#if defined(GPIOG)
        LL_APB2_GRP1_PERIPH_GPIOG |
#endif
        0);
    return 0;
}

static GPIO_TypeDef * const ports[] = {
    GPIOA, GPIOB, GPIOC, GPIOD,
#if defined(GPIOE)
    GPIOE,
#endif
#if defined(GPIOF)
    GPIOF,
#endif
#if defined(GPIOG)
    GPIOG
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

static GPIO_TypeDef *port_for_pin(uint16_t pin) {
    return ports[pin>>4];
}

int gpio_hal_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull) {
    GPIO_TypeDef *port = port_for_pin(pin);
    uint32_t hw_pin = pins[pin&0xf];
    uint32_t mode = (uint32_t[5]){LL_GPIO_MODE_INPUT, LL_GPIO_MODE_OUTPUT, LL_GPIO_MODE_ANALOG, LL_GPIO_MODE_INPUT, LL_GPIO_MODE_ALTERNATE}[dir];
    uint32_t output = 
        (dir == GPIO_DIRECTION_OUTPUT || dir == GPIO_DIRECTION_FUNCTION_OUT) ?
          (pull == GPIO_PULL_NONE ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL) :
          LL_GPIO_OUTPUT_OPENDRAIN;
    LL_GPIO_InitTypeDef cfg = {
        .Pin = hw_pin,
        .Mode = mode,
        .Speed = mode == LL_GPIO_MODE_ALTERNATE ? LL_GPIO_MODE_OUTPUT_50MHz : LL_GPIO_MODE_OUTPUT_10MHz,
        .OutputType = output,
        .Pull = pull == GPIO_PULL_DOWN ? LL_GPIO_PULL_DOWN : LL_GPIO_PULL_UP
    };
    (void)LL_GPIO_Init(port, &cfg);
    return 0;
}

int gpio_hal_set(uint16_t pin, uint8_t state) {
    GPIO_TypeDef *port = port_for_pin(pin);
    uint32_t hw_pin = pins[pin&0xf];
    if (state) {
        LL_GPIO_SetOutputPin(port, hw_pin);
    } else {
        LL_GPIO_ResetOutputPin(port, hw_pin);
    }
    return 0;
}

int gpio_hal_read(uint16_t pin) {
    GPIO_TypeDef *port = port_for_pin(pin);
    uint32_t hw_pin = pins[pin&0xf];
    return LL_GPIO_IsInputPinSet(port, hw_pin);
}

int gpio_hal_deinit(void) {
    LL_APB2_GRP1_DisableClock(
        LL_APB2_GRP1_PERIPH_GPIOA |
        LL_APB2_GRP1_PERIPH_GPIOB |
        LL_APB2_GRP1_PERIPH_GPIOC |
        LL_APB2_GRP1_PERIPH_GPIOD |
#if defined(GPIOE)
        LL_APB2_GRP1_PERIPH_GPIOE |
#endif
#if defined(GPIOF)
        LL_APB2_GRP1_PERIPH_GPIOF |
#endif
#if defined(GPIOG)
        LL_APB2_GRP1_PERIPH_GPIOG |
#endif
        0);
    return 0;
}
