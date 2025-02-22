#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DISP_W 128
#define DISP_H 64
#define DISP_TRANSFER_TICKS 450

typedef void (*disp_cb_t)(int err_code);

void disp_init(void);
void disp_set_enabled(bool enable, disp_cb_t done_cb);
bool disp_screen_update(disp_cb_t done_cb);
uint8_t *disp_gbuf(void);
