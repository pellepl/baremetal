#include "types.h"
#include "disp.h"
#include "minio.h"
#include "ui.h"
#include "timer.h"

static timer_t press_repeat_timer;
static volatile uint32_t repeat_nbr = 0;

static void button_repeat_timer_cb(timer_t *t) {
    ui_button_t *b = (ui_button_t *)t->user;
    if ((b->comp.state & UI_STATE_PRESSED) &&
        (b->comp.state & UI_STATE_VISIBLE) &&
        (b->comp.state & UI_STATE_ENABLED)) {
        timer_start(&press_repeat_timer, button_repeat_timer_cb, 
            TIMER_MS_TO_TICKS(100), TIMER_ONESHOT);
        ui_event_t *e = ui_new_event(EVENT_CB_BUTTON_PRESS);
        e->source = &b->comp;
        e->value = repeat_nbr;
        ui_post_event(e);
        repeat_nbr++;
    }
}

int button_event(ui_component_t *ui, ui_event_t *event, ui_info_t *i) {
    int res = 0;
    if (event->type != EVENT_PRESS && event->type != EVENT_RELEASE && event->type != EVENT_DRAG) return 0;
    //printf("but event this:%08x (%s) %d within %d data %08x\n", ui, ui->_type, event->type, __ui_is_event_within(ui, event, i), event->value);
    if (__ui_is_event_within(ui, event, i)) {
        if (event->type == EVENT_PRESS) {
            press_repeat_timer.user = ui;
            timer_start(&press_repeat_timer, button_repeat_timer_cb, 
                TIMER_MS_TO_TICKS(600), TIMER_ONESHOT);
            __ui_set_flag_state(ui, UI_STATE_PRESSED, 1);
        }

        if (event->type == EVENT_RELEASE) {
            repeat_nbr = 0;
            if (ui->state & UI_STATE_PRESSED) {
                timer_stop(&press_repeat_timer);
                __ui_set_flag_state(ui, UI_STATE_PRESSED, 0);
                ui_event_t *e = ui_new_event(EVENT_CB_BUTTON_PRESS);
                e->value = 1;
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
        if (ui->state & UI_STATE_PRESSED) {
            repeat_nbr = 0;
            timer_stop(&press_repeat_timer);
        }
        __ui_set_flag_state(ui, UI_STATE_PRESSED, 0);
    }
    return res;
}

static void button_paint(ui_component_t *ui, void *ctx, ui_info_t *i) {
    if ((ui->state & UI_STATE_REPAINT) == 0) return;
    ui_button_t *c = (ui_button_t *)ui;
    uint32_t bg1 = (ui->state & UI_STATE_PRESSED) ? __ui_colfade(0xffffffff, c->colbg1, 0x80) : c->colbg1;
    uint32_t bg2 = (ui->state & UI_STATE_PRESSED) ? __ui_colfade(0xffffffff, c->colbg2, 0x80) : c->colbg2;
    uint32_t fg = c->colfg;
    uint16_t x = c->comp.x + (i ? i->x : 0);
    uint16_t y = c->comp.y + (i ? i->y : 0);
    uint16_t w = c->comp.w;
    uint16_t h = c->comp.h;
    const char *str = c->text;
    uint8_t font = c->font;
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
    disp_draw_hline(x+1, y, w-1, bordercol, 2);
    disp_draw_hline(x+1, y+h, w-1, bordercol, 2);
    disp_draw_vline(x, y+1, h-1, bordercol, 2);
    disp_draw_vline(x+w, y+1, h-1, bordercol, 2);

    int yy = y+2; int hh =2*(h-2)/3-4;
    disp_fill(x+2,yy, w-2, hh, bg2);
    yy += hh; hh = 2;
    disp_fill(x+2,yy,w-2,hh, bg102);
    yy += hh; hh = 2;
    disp_fill(x+2,yy,w-2,hh, bg112);
    yy += hh; hh = 2;
    disp_fill(x+2,yy,w-2,hh, bg122);
    yy += hh; hh = 2;
    disp_fill(x+2,yy,w-2,hh, bg132);
    yy += hh; hh = h-(yy-y);
    disp_fill(x+2,yy,w-2,hh, bg1);

    disp_align_t alignh, alignv;
    switch (ui->state & UI_STATE_ALIGNH__MASK) {
        case UI_STATE_ALIGNH_CENTER: x += w/2; alignh = DISP_STR_ALIGN_CENTER; break;
        case UI_STATE_ALIGNH_RIGHT: x += w; alignh = DISP_STR_ALIGN_RIGHT_BOTTOM; break;
        default: alignh = DISP_STR_ALIGN_LEFT_TOP; break;
    }
    switch (ui->state & UI_STATE_ALIGNV__MASK) {
        case UI_STATE_ALIGNV_CENTER: y += h/2; alignv = DISP_STR_ALIGN_CENTER; break;
        case UI_STATE_ALIGNV_BOTTOM: y += h; alignv = DISP_STR_ALIGN_RIGHT_BOTTOM; break;
        default: alignv = DISP_STR_ALIGN_LEFT_TOP; break;
    }

    disp_draw_string(str, x+2, y+2, font, alignh, alignv, fg, 0);
}

void ui_button_init(ui_button_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                    const char *text, event_fn_t cb, void *user, 
                    uint8_t font, uint32_t colfg, uint32_t colbg1, uint32_t colbg2) {
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "BTN\0";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = button_paint;
    ui->comp.handle_event = button_event;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    ui->call.cb = cb;
    ui->call.user = user;
    ui->text = text;
    ui->font = font;
    ui->colfg = colfg;
    ui->colbg1 = colbg1;
    ui->colbg2 = colbg2;
    ui_set_alignment(&ui->comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
}

