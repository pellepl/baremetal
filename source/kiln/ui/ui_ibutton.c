#include "types.h"
#include "disp.h"
#include "minio.h"
#include "ui.h"
#include "timer.h"
#include "fonts/fonts.h"

static int ibutton_event(ui_component_t *ui, ui_event_t *event, ui_info_t *i) {
    int res = 0;
    if (event->type != EVENT_PRESS && event->type != EVENT_RELEASE && event->type != EVENT_DRAG) return 0;
    if (__ui_is_event_within(ui, event, i)) {
        if (event->type == EVENT_PRESS) {
            __ui_set_flag_state(ui, UI_STATE_PRESSED, 1);
        }

        if (event->type == EVENT_RELEASE) {
            if (ui->state & UI_STATE_PRESSED) {
                __ui_set_flag_state(ui, UI_STATE_PRESSED, 0);
                ui_event_t *e = ui_new_event(EVENT_CB_BUTTON_PRESS);
                e->source = ui;
                ui_post_event(e);
            }
        }

        if (event->type == EVENT_DRAG || event->type == EVENT_HOVER) {
            __ui_set_flag_state(ui, UI_STATE_HIGHLIGHTED, 1);
        } else {
            __ui_set_flag_state(ui, UI_STATE_HIGHLIGHTED, 0);
        }
    } else {
        __ui_set_flag_state(ui, UI_STATE_HIGHLIGHTED, 0);
        __ui_set_flag_state(ui, UI_STATE_PRESSED, 0);
    }
    return res;
}

static void ibutton_paint(ui_component_t *ui, void *ctx, ui_info_t *i) {
    if ((ui->state & UI_STATE_REPAINT) == 0) return;
    ui_ibutton_t *c = (ui_ibutton_t *)ui;
    uint32_t col = c->colfg;
    uint16_t x = c->comp.x + (i ? i->x : 0);
    uint16_t y = c->comp.y + (i ? i->y : 0);
    uint16_t w = c->comp.w;
    uint16_t h = c->comp.h;
    uint32_t bordercol = (ui->state & UI_STATE_HIGHLIGHTED) ? 0xffffffff : col;
    uint32_t fillcol = __ui_colfade(col,0,0xc0);
    uint32_t iconcol = (ui->state & (UI_STATE_ACTIVE)) ? 0xffffffff : 0xff000000;

    if (ui->state & (UI_STATE_PRESSED)) {
        iconcol = col;
        fillcol = 0;
    }

    if ((ui->state & UI_STATE_ENABLED) == 0) {
        bordercol = COL_DISABLED_BG1;
        fillcol = COL_DISABLED_BG2;
        col = COL_DISABLED_FG;
    }

    disp_fill(x+2,y+2,w-2,h-2,fillcol);
    disp_draw_hline(x+1, y, w-1, bordercol, 2);
    disp_draw_hline(x+1, y+h, w-1, bordercol, 2);
    disp_draw_vline(x, y+1, h-1, bordercol, 2);
    disp_draw_vline(x+w, y+1, h-1, bordercol, 2);
    disp_align_t alignh, alignv;
    x += w/2; alignh = DISP_STR_ALIGN_CENTER;
    y += (h-32)/2; alignv = DISP_STR_ALIGN_CENTER;

    char l[2][3] = {0};
    l[0][0] = c->icon*4 + ' ' + 0;
    l[0][1] = c->icon*4 + ' ' + 1;
    l[1][0] = c->icon*4 + ' ' + 2;
    l[1][1] = c->icon*4 + ' ' + 3;

    disp_draw_string(l[0], x+2, y+2, FONT_BIG_ICONS, alignh, alignv, iconcol, 0);
    disp_draw_string(l[1], x+2, y+2+32, FONT_BIG_ICONS, alignh, alignv, iconcol, 0);
}

void ui_ibutton_init(ui_ibutton_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                    uint8_t icon, event_fn_t callback, void *user,
                    uint32_t colfg) {
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "IBT\0";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = ibutton_paint;
    ui->comp.handle_event = ibutton_event;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    ui->call.cb = callback;
    ui->call.user = user;
    ui->icon = icon;
    ui->colfg = colfg;
}

