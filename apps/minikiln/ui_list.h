#pragma once
#include <stdint.h>
#include "ui.h"

typedef struct
{
    const char *string;
} ui_listitem_t;

typedef struct
{
    const ui_listitem_t *items;
    uint8_t item_count;
    int selected_index;
    int y_current;
    int y_target;
} ui_list_t;

void ui_list_init(ui_list_t *list, const ui_listitem_t *items, uint8_t item_count, uint8_t selected_index);
ui_tick_t ui_list_paint(ui_list_t *list, const gfx_ctx_t *ctx);
void ui_list_move_index(ui_list_t *list, int delta);
void ui_list_set_index(ui_list_t *list, uint8_t index);
uint8_t ui_list_get_selected_index(ui_list_t *list);
