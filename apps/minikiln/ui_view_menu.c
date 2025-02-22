#include "minio.h"
#include "ui.h"
#include "ui_list.h"
#include "ui_scrolltext.h"
#include "font_roboto_extrabold_30.h"

static struct
{
    ui_list_t list;
    ui_scrolltext_t scrl;
} me;

static const ui_listitem_t items[] = {
    (ui_listitem_t){.string = "Settings"},
    (ui_listitem_t){.string = "Calibration"},
    (ui_listitem_t){.string = "Kiln"},
    (ui_listitem_t){.string = "Temperature"},
    (ui_listitem_t){.string = "Set time"},
    (ui_listitem_t){.string = "Reset"},
    (ui_listitem_t){.string = "Factory"}};

static void enter(void)
{
    static bool init = false;
    if (!init)
    {
        ui_list_init(&me.list, items, 7, 4);
        ui_scrolltext_init(&me.scrl, "blahabvlalaskdjlaksdjkal sj dlkasj dlksajdl kjaslkd jalsjd... WRAP!", &font_roboto_extrabold_30, 0, 0, DISP_W);
        init = true;
    }
    ui_list_set_index(&me.list, ui_list_get_selected_index(&me.list));
}

static void handle_event(uint32_t type, void *arg)
{
    switch (type)
    {
    case EVENT_UI_PRESS:
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
    ui_tick_t t1 = ui_list_paint(&me.list, ctx);
    ui_tick_t t2 = ui_scrolltext_paint(&me.scrl, ctx);
    return t1 < t2 ? t1 : t2;
}

const ui_view_t view_menu = {

    .enter = enter,
    .handle_event = handle_event,
    .paint = paint,
};
