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

int gpio_hal_set(uint16_t pin, uint8_t state) {
    NRF_GPIO_Type *port = port_for_pin(pin);
    pin &= (P0_PIN_NUM-1);
    if (state) {
        port->OUTSET = (1<<pin);
    } else {
        port->OUTCLR = (1<<pin);
    }
    return 0;
}

int gpio_hal_read(uint16_t pin) {
    NRF_GPIO_Type *port = port_for_pin(pin);
    pin &= (P0_PIN_NUM-1);
    return ((port->IN >> pin) & 1) != 0;
}

int gpio_hal_deinit(void) {
    return 0;
}
