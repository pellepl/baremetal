#include "minio.h"
#include "ui.h"
#include "ui_list.h"
#include "ui_views.h"

static struct
{
    ui_list_t list;
} me;

static const ui_listitem_t items[] = {
    (ui_listitem_t){.string = "Settings"},
    (ui_listitem_t){.string = "Set kiln"},
    (ui_listitem_t){.string = "Temp history"},
};

static void init(void)
{
    ui_list_init(&me.list, items, ARRAY_LENGTH(items), ARRAY_LENGTH(items) / 2, 0, 0, DISP_W, DISP_H);
}

static void enter(void)
{
    ui_list_reset(&me.list);
}

static void handle_event(uint32_t type, void *arg)
{
    switch (type)
    {
    case EVENT_UI_CLICK:
        ui_list_select(&me.list);
        ui_goto_view(&view_sleep, false);
        ui_trigger_update();
        break;
    case EVENT_UI_BACK:
        ui_trigger_update();
        break;
    case EVENT_UI_SCRL:
        ui_list_move_index(&me.list, (int)arg);
        ui_trigger_update();
        break;
    default:
        break;
    }
}

static ui_tick_t paint(const gfx_ctx_t *ctx)
{
    return ui_list_paint(&me.list, ctx);
}

UI_DECLARE_VIEW view_menu = {
    .init = init,
    .enter = enter,
    .handle_event = handle_event,
    .paint = paint,
};
