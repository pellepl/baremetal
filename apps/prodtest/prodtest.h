#ifndef _PRODTEST_H_
#define _PRODTEST_H_

#include "targets.h"
#include "eventqueue.h"
#include "board.h"
#include "errors.h"

// execute any lingering events
void yield_for_events(void);

// output CLI commands starting with uppercase or lowercase
void print_commands(bool uppercase, int spaces);

// output the lfclk on a gpio
int prodtest_output_lfclk(bool enable);

// set nrf52 dcdc
void prodtest_toggle_dcdc_state(uint8_t dcdc_state);

NRF_GPIO_Type *prodtest_port_for_pin(uint16_t pin);

#endif // _PRODTEST_H_


