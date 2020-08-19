/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "gpio_hal.h"
#include "nrf.h"
#include "nrf5340_application_peripherals.h"
#define P0_PIN_NUM  32

static NRF_GPIO_Type *port_for_pin(uint16_t pin) {
    return pin < P0_PIN_NUM ? NRF_P0_S : NRF_P1_S;
}

int gpio_hal_init(void) {
    return 0;
}

int gpio_hal_config(uint16_t pin, gpio_direction_t dir, gpio_pull_t pull) {
    NRF_GPIO_Type *port = port_for_pin(pin);
    pin &= (P0_PIN_NUM-1);
    port->PIN_CNF[pin] = 0
        | ((dir == GPIO_DIRECTION_OUTPUT ? GPIO_PIN_CNF_DIR_Output : GPIO_PIN_CNF_DIR_Input)          << GPIO_PIN_CNF_DIR_Pos)
        | ((dir == GPIO_DIRECTION_INPUT ? GPIO_PIN_CNF_INPUT_Connect : GPIO_PIN_CNF_INPUT_Disconnect) << GPIO_PIN_CNF_INPUT_Pos)
        | ((pull == GPIO_PULL_NONE ? GPIO_PIN_CNF_PULL_Disabled : 
              (pull == GPIO_PULL_DOWN ? GPIO_PIN_CNF_PULL_Pulldown : GPIO_PIN_CNF_PULL_Pullup))       << GPIO_PIN_CNF_PULL_Pos)
        | (GPIO_PIN_CNF_DRIVE_S0S1                                                                    << GPIO_PIN_CNF_DRIVE_Pos)
        | (GPIO_PIN_CNF_SENSE_Disabled                                                                << GPIO_PIN_CNF_SENSE_Pos)
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
