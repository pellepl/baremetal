#ifndef _PRODTEST_H_
#define _PRODTEST_H_

#include "targets.h"
#include "eventqueue.h"
#include "board.h"
#include "errors.h"

// execute any lingering events
void yield_for_events(void);

void print_commands(bool uppercase, int spaces);

int prodtest_output_lfclk(bool enable);

void prodtest_toggle_dcdc_state(uint8_t dcdc_state);

NRF_GPIO_Type *prodtest_port_for_pin(uint16_t pin);

#endif // _PRODTEST_H_


