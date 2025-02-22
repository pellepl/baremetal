#pragma once

#include <stdint.h>

#define INPUT_ROTARY_DIVISOR 2
#define INPUT_LONG_PRESS_SEC 3

typedef enum
{
    INPUT_BUTTON_ROTARY,
    _INPUT_BUTTON_COUNT
} input_button_t;

void input_init(void);
void input_handle_rotary(void);