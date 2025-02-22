#include "ui_list.h"

static void update(ui_list_t *list)
{
    const uint32_t font_h = UI_FONT_ITEM->max_height;
    if (list->selected_index < 0)
        list->selected_index = 0;
    if (list->selected_index >= list->item_count)
        list->selected_index = list->item_count - 1;
    list->y_target = (DISP_H / 2 - font_h / 2 - list->selected_index * font_h) << 3;
}

void ui_list_init(ui_list_t *list, const ui_listitem_t *items, uint8_t item_count, uint8_t selected_index)
{
    list->item_count = item_count;
    list->items = items;
    list->selected_index = selected_index;
    update(list);
    list->y_current = list->y_target;
}

void ui_list_move_index(ui_list_t *list, int delta)
{
    list->selected_index += delta;
    update(list);
}

void ui_list_set_index(ui_list_t *list, uint8_t index)
{
    list->selected_index = index;
    update(list);
    list->y_current = list->y_target;
}

uint8_t ui_list_get_selected_index(ui_list_t *list)
{
    return list->selected_index;
}

ui_tick_t ui_list_paint(ui_list_t *list, const gfx_ctx_t *ctx)
{
    list->y_current = ui_move_towards(list->y_current, list->y_target);
    int y = (list->y_current + 3) >> 3;
    for (int i = 0; i < list->item_count; i++)
    {
        if (i == list->selected_index) {
            gfx_area_t a = {
                .x0 = 0, .y0 = y + 3,
                .x1 = 4, .y1 = y + UI_FONT_ITEM->max_height - 5,
            };
            gfx_fill(ctx, &a, 1);
        }
        gfx_string(ctx, i == list->selected_index ? UI_FONT_ITEM : UI_FONT_ITEM_SMALL, list->items[i].string, 8, y);
        y += UI_FONT_ITEM->max_height;
    }

    return list->y_current != list->y_target ? 0 : UI_TICK_NEVER;
}
