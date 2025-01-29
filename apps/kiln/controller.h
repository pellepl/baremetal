#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include "bmtypes.h"

#ifndef CTRL_CYCLES
#define CTRL_CYCLES 10
#endif

typedef enum
{
    CONTROL_OFF = 0,
    CONTROL_MANUAL,
    CONTROL_PROGRAM
} control_state_t;

void ctrl_init(void);
void ctrl_panic(void);
int ctrl_is_panicking(void);
void ctrl_stop(void);
void ctrl_elements_off(void);
void ctrl_manual_set_temp(float temp);
void ctrl_manual_set_power(uint8_t power);
void ctrl_pid_set_epsilon(uint32_t div_by_10000);
void ctrl_pid_set_k_p(uint32_t div_by_10000);
void ctrl_pid_set_k_i(uint32_t div_by_10000);
void ctrl_pid_set_k_d(uint32_t div_by_10000);
void ctrl_second_callback(uint32_t second);

#endif
