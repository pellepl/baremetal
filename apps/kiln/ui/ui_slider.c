#include "bmtypes.h"
#include "disp.h"
#include "minio.h"
#include "ui.h"
#include "timer.h"

static int slider_event(ui_component_t *ui, ui_event_t *event, ui_info_t *i) {
    int res = 0;
    if (__ui_is_event_within(ui, event, i)) {
        if (event->type == EVENT_PRESS) {
            ui_slider_t *c = (ui_slider_t *)ui;
            c->_oval = c->val;
            __ui_set_flag_state(ui, UI_STATE_PRESSED, 1);
        }

        if (event->type == EVENT_RELEASE) {
            if (ui->state & UI_STATE_PRESSED) {
                __ui_set_flag_state(ui, UI_STATE_PRESSED, 0);
                ui_slider_t *c = (ui_slider_t *)ui;
                ui_event_t *e = ui_new_event(EVENT_CB_SLIDER_SELECT);
                e->value = c->val;
                e->source = ui;
                ui_post_event(e);
            }
        }

        if (event->type == EVENT_DRAG || event->type == EVENT_HOVER) {
            __ui_set_flag_state(ui, UI_STATE_HIGHLIGHTED, 1);
            if (ui->state & UI_STATE_PRESSED) {
                ui_slider_t *c = (ui_slider_t *)ui;
                uint16_t x = c->comp.x + (i ? i->x : 0);
                uint16_t y = c->comp.y + (i ? i->y : 0);
                uint16_t w = c->comp.w;
                uint16_t h = c->comp.h;
                x += 2; y += 2; w -= 4; h -= 4;
                int32_t gg = c->hori ? event->x : event->y;
                int32_t gsize = (c->hori ? w : h) - c->knob_h;
                int32_t val = 
                    ((gg - (c->hori ? x : y)) * (c->max - c->min)) / gsize
                    + c->min;
                if (c->hori == 0) val = c->max - val;
                if (val < c->min) val = c->min;
                if (val > c->max) val = c->max;
                if (val != c->val) {
                    c->val = val;
                    ui_event_t *e = ui_new_event(EVENT_CB_SLIDER_ADJUST);
                    e->value = c->val;
                    e->source = ui;
                    ui_post_event(e);
                    ui->state |= UI_STATE_REPAINT;
                }
            }
        } else {
            __ui_set_flag_state(ui, UI_STATE_HIGHLIGHTED, 0);
        }
    } else if (event->type != EVENT_REPAINT) {
        if (ui->state & UI_STATE_PRESSED) {
            __ui_set_flag_state(ui, UI_STATE_PRESSED, 0);
            ui_slider_t *c = (ui_slider_t *)ui;
            if (c->_oval != c->val) {
                c->val = c->_oval;
                ui_event_t *e = ui_new_event(EVENT_CB_SLIDER_SELECT);
                e->value = c->val;
                e->source = ui;
                ui_post_event(e);
                ui->state |= UI_STATE_REPAINT;
            }
        }
        __ui_set_flag_state(ui, UI_STATE_HIGHLIGHTED, 0);
    }
    return res;
}

static void slider_paint(ui_component_t *ui, void *ctx, ui_info_t *i) {
    if ((ui->state & UI_STATE_REPAINT) == 0) return;
    ui_slider_t *c = (ui_slider_t *)ui;
    uint32_t bg1 = (ui->state & UI_STATE_PRESSED) ? __ui_colfade(0xffffffff, c->colbg1, 0x80) : c->colbg1;
    uint32_t bg2 = (ui->state & UI_STATE_PRESSED) ? __ui_colfade(0xffffffff, c->colbg2, 0x80) : c->colbg2;
    uint32_t fg = c->colfg;
    uint16_t x = c->comp.x + (i ? i->x : 0);
    uint16_t y = c->comp.y + (i ? i->y : 0);
    uint16_t w = c->comp.w;
    uint16_t h = c->comp.h;
    disp_fill(x,y,w,h,COL_BG);
    uint32_t bordercol = c->comp.state & UI_STATE_HIGHLIGHTED ? fg : c->colbg1;
    if ((ui->state & UI_STATE_ENABLED) == 0) {
        fg = COL_DISABLED_FG;
        bg1 = COL_DISABLED_BG1;
        bg2 = COL_DISABLED_BG2;
        bordercol = COL_DISABLED_BG2;
    }
    uint32_t bg102 = __ui_colfade(bg1, bg2, 0x29);
    uint32_t bg112 = __ui_colfade(bg1, bg2, 0x52);
    uint32_t bg122 = __ui_colfade(bg1, bg2, 0x7b);
    uint32_t bg132 = __ui_colfade(bg1, bg2, 0xa5);
    w-=2; h-=2;
    x++;y++;
    uint32_t gsize = (c->hori ? w-2 : h-2) - c->knob_h;
    if (c->hori) {
        int gg = x + 1 + c->knob_h/2;
        disp_draw_hline(gg, y + h/2, gsize, bg1, c->track_w);
        disp_draw_hline(gg, y + h/2, gsize - gsize/4, bg1, c->track_w-2);
        gg += gsize - gsize/4;
        disp_draw_hline(gg, y + h/2, gsize/16, bg132, c->track_w-2);
        gg += gsize/16;
        disp_draw_hline(gg, y + h/2, gsize/16, bg122, c->track_w-2);
        gg += gsize/16;
        disp_draw_hline(gg, y + h/2, gsize/16, bg112, c->track_w-2);
        gg += gsize/16;
        disp_draw_hline(gg, y + h/2, gsize/16, bg102, c->track_w-2);
    } else {
        int gg = y + 1 + c->knob_h/2;
        disp_draw_vline(x + w/2, gg, gsize, bg1, c->track_w);
        disp_draw_vline(x + w/2, gg, gsize-gsize/4, bg2, c->track_w-2);
        gg += gsize - gsize/4;
        disp_draw_vline(x + w/2, gg, gsize/16, bg102, c->track_w-2);
        gg += gsize/16;
        disp_draw_vline(x + w/2, gg, gsize/16, bg112, c->track_w-2);
        gg += gsize/16;
        disp_draw_vline(x + w/2, gg, gsize/16, bg122, c->track_w-2);
        gg += gsize/16;
        disp_draw_vline(x + w/2, gg, gsize/16, bg132, c->track_w-2);
    }
    {
        int32_t val_g = ((gsize) * (c->hori ? c->val : (c->max - c->val))) / (c->max - c->min);
        int16_t bw = c->hori ? c->knob_h : c->knob_w;
        int16_t bh = c->hori ? c->knob_w : c->knob_h;
        int16_t bx = c->hori ? (x + 1 + val_g) : (x + w/2 - c->knob_w/2);
        int16_t by = c->hori ? (y + h/2 - c->knob_w/2) : (y + 1 + val_g);
        disp_draw_hline(bx+1, by, bw-1, bordercol, 2);
        disp_draw_hline(bx+1, by+bh, bw-1, bordercol, 2);
        disp_draw_vline(bx, by+1, bh-1, bordercol, 2);
        disp_draw_vline(bx+bw, by+1, bh-1, bordercol, 2);
        disp_fill(bx+2,by+2, bw-2,(bh-2)/2-4, bg2);
        disp_fill(bx+2,by+2+(bh-2)/2-4, bw-2,2, bg102);
        disp_fill(bx+2,by+2+(bh-2)/2-2, bw-2,2, bg112);
        disp_fill(bx+2,by+2+(bh-2)/2+0, bw-2,2, bg122);
        disp_fill(bx+2,by+2+(bh-2)/2+2, bw-2,2, bg132);
        disp_fill(bx+2,by+2+((bh-2)/2+4), bw-2,(bh-2)/2-4, bg1);
    }
}

void ui_slider_init(ui_slider_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                    event_fn_t cb, void *user, 
                    int min, int max, int initial, int hori_else_veri,
                    const char *text, int font,
                    uint32_t colfg, uint32_t colbg1, uint32_t colbg2) {
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "SLD\0";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = slider_paint;
    ui->comp.handle_event = slider_event;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    ui->call.cb = cb;
    ui->call.user = user;
    ui->min = min;
    ui->max = max;
    ui->val = initial;
    ui->hori = hori_else_veri;
    ui->colfg = colfg;
    ui->colbg1 = colbg1;
    ui->colbg2 = colbg2;
    ui->knob_w = 50;
    ui->knob_h = 30;
    ui->track_w = 10;
}
