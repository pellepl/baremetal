#pragma once

#include <stdint.h>

int disp_ch3613_init(void);
int disp_ch3613_setup(void);
void disp_ch3613_deinit(void);
int disp_ch3613_palette(const uint32_t *palette);
void disp_ch3613_clear(uint8_t colour);
void disp_ch3613_fill(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t colour);
int disp_ch3613_string(const char *str, int16_t x, int16_t y, uint8_t colour);
int disp_ch3613_flush(void);
