#pragma once

#include <stdint.h>

#define INPUT_ROTARY_DIVISOR    2

void input_init(void);
int16_t input_rot_read(void);
void input_rot_reset(void);
