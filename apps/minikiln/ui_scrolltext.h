#pragma once
#include <stdint.h>
#include "ui.h"

typedef struct
{
    const char *string;
    const gfx_font_t *font;
    int str_width;
    int x;
    int y;
    int width;
    ui_tick_t tick;
} ui_scrolltext_t;

void ui_scrolltext_init(ui_scrolltext_t *st, const char *string, const gfx_font_t *font, int x, int y, int width);
ui_tick_t ui_scrolltext_paint(ui_scrolltext_t *st, const gfx_ctx_t *ctx);
void ui_scrolltext_reset(ui_scrolltext_t *st);
void ui_scrolltext_set_text(ui_scrolltext_t *st, const char *string);
