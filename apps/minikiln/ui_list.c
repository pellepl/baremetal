#include "ui_list.h"

static void update(ui_list_t *list)
{
    const uint32_t font_h = UI_FONT_ITEM->max_height;
    if (list->selected_index < 0)
        list->selected_index = 0;
    if (list->selected_index >= list->item_count)
        list->selected_index = list->item_count - 1;
    list->y_target = (list->h / 2 - font_h / 2 - list->selected_index * font_h + list->y) << 3;
}

void ui_list_init(ui_list_t *list, const ui_listitem_t *items, uint8_t item_count, uint8_t selected_index, int x, int y, int w, int h)
{
    list->item_count = item_count;
    list->items = items;
    list->selected_index = selected_index;
    list->x = x;
    list->y = y;
    list->w = w;
    list->h = h;
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

void ui_list_reset(ui_list_t *list)
{
    update(list);
    list->y_current = list->y_target;
    list->anim_select = 0;
}

uint8_t ui_list_get_selected_index(ui_list_t *list)
{
    return list->selected_index;
}

#define DELAY 6
#define MAX_DELAY (DELAY * 5)
static void paint_selector(ui_list_t *list, const gfx_ctx_t *ctx, int y)
{
    int anim = list->anim_select;
    if (anim >= MAX_DELAY)
    {
        list->anim_select = MAX_DELAY + 1;
        return;
    }
    gfx_area_t a = {
        .x0 = list->x,
        .y0 = y + 3,
        .x1 = list->x + 4,
        .y1 = y + UI_FONT_ITEM->max_height - 5,
    };
    if (anim > 0)
    {
        a.x0 = lerp(list->x, list->x + list->w, anim - 2 * MAX_DELAY / 3, 1 * MAX_DELAY / 3);
        a.x1 = lerp(list->x + 4, list->x + list->w, anim - 1, MAX_DELAY);
        list->anim_select += 1;
    }
    gfx_fill(ctx, &a, GFX_COL_XOR);
}

void ui_list_select(ui_list_t *list)
{
    list->anim_select = 1;
}

ui_tick_t ui_list_paint(ui_list_t *list, const gfx_ctx_t *ctx)
{
    list->y_current = ui_move_towards(list->y_current, list->y_target);
    int y = (list->y_current + 3) >> 3;
    for (int i = 0; i < list->item_count; i++)
    {
        gfx_string(ctx, i == list->selected_index ? UI_FONT_ITEM : UI_FONT_ITEM_SMALL, list->items[i].string, list->x + 8, y, GFX_COL_SET);
        if (i == list->selected_index)
        {
            paint_selector(list, ctx, y);
        }
        y += UI_FONT_ITEM->max_height;
    }

    return list->y_current != list->y_target || (list->anim_select > 0 && list->anim_select <= MAX_DELAY) ? 0 : UI_TICK_NEVER;
}
