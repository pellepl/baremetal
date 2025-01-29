#include "bmtypes.h"
#include "disp.h"
#include "fonts/fonts.h"
#include "minio.h"
#include "ui.h"
#include "timer.h"

#define TXT_UP "\x82"
#define TXT_DOWN "\x83"
#define BUT_W 60
#define BUT_H 40

static timer_t scrl_timer;

static uint16_t item_height(ui_list_t *c)
{
    return font_get(c->font)->height + c->item_padding;
}

static int16_t move_viewport_y(ui_list_t *c, int16_t dy)
{
    if (dy < 0)
    {
        if (c->vp_y < -dy)
        {
            c->vp_y = 0;
            dy = 0;
        }
    }
    else
    {
        uint16_t content_h = item_height(c) * c->items;
        if (c->vp_y > content_h - c->comp.h - dy)
        {
            c->vp_y = content_h - c->comp.h;
            dy = 0;
        }
    }
    c->vp_y += dy;
    return dy;
}

static void scrl_timer_cb(timer_t *t)
{
    ui_list_t *c = (ui_list_t *)t->user;
    c->comp.state |= UI_STATE_REPAINT;
    int32_t d = c->spin_dvp_y >> 7;
    if (d > 0)
        d++;
    if (move_viewport_y(c, (int16_t)d) == 0)
    {
        c->spin_dvp_y = 0;
        timer_stop(&scrl_timer);
    }
    c->spin_dvp_y = (c->spin_dvp_y * 5) / 8;
    ui_post_event(ui_new_event(EVENT_REPAINT));
}

static void check_scrl(ui_list_t *c)
{
    if (c->anchor_t == 0)
        return;
    uint32_t dt = (uint32_t)(timer_now() - c->anchor_t);
    int32_t dvp_y = c->vp_y - c->anchor_vp_y;
    if (dt < 15000)
    {
        c->spin_dvp_y = (dvp_y << 8);
        scrl_timer.user = c;
        timer_start(&scrl_timer, scrl_timer_cb, TIMER_MS_TO_TICKS(75), TIMER_REPETITIVE);
    }
    c->anchor_t = 0;
}

static int list_event(ui_component_t *ui, ui_event_t *event, ui_info_t *i)
{
    int res = 0;
    ui_list_t *c = (ui_list_t *)ui;
    c->b_up.comp.handle_event(&c->b_up.comp, event, i);
    c->b_down.comp.handle_event(&c->b_down.comp, event, i);
    if (event->type != EVENT_PRESS && event->type != EVENT_RELEASE && event->type != EVENT_DRAG)
        return 0;
    if (__ui_is_event_within(ui, event, i))
    {
        const uint16_t ih = item_height(c);
        if (event->x < ui->x + (i ? i->x : 0) + ui->w - BUT_W)
        {
            if (event->type == EVENT_PRESS)
            {
                timer_stop(&scrl_timer);
                __ui_set_flag_state(ui, UI_STATE_PRESSED, 1);
                c->anchor_x = event->x;
                c->anchor_y = event->y;
                c->anchor_vp_y = c->vp_y;
                c->anchor_t = timer_now();

                uint32_t p_ix = (c->vp_y + event->y - (c->comp.y + (i ? i->y : 0))) / ih;
                c->sel_ix = p_ix;
                ui->state |= UI_STATE_REPAINT;
            }

            if (event->type == EVENT_RELEASE)
            {
                check_scrl(c);
                if ((ui->state & UI_STATE_TRANSLATING) == 0)
                {
                    uint32_t p_ix = (c->vp_y + event->y - (c->comp.y + (i ? i->y : 0))) / ih;
                    if (p_ix == c->sel_ix)
                    {
                        ui_event_t *e = ui_new_event(EVENT_CB_LIST_SELECT);
                        e->source = ui;
                        e->user = (void *)c->sel_ix;
                        ui_post_event(e);
                    }
                    c->sel_ix = -1;
                }
                ui->state &= ~UI_STATE_TRANSLATING;
                ui->state |= UI_STATE_REPAINT;
            }

            if (ui->state & UI_STATE_PRESSED && (event->type == EVENT_DRAG || event->type == EVENT_HOVER))
            {
                int16_t d = (c->anchor_y - event->y);
                if (d > ih / 2 || -d > ih / 2)
                {
                    ui->state |= UI_STATE_TRANSLATING;
                }
                if (ui->state & UI_STATE_TRANSLATING)
                {
                    if (d)
                    {
                        move_viewport_y(c, d);
                        c->anchor_x = event->x;
                        c->anchor_y = event->y;
                        c->sel_ix = -1;
                        ui->state |= UI_STATE_REPAINT;
                    }
                }
            }
        }
    }
    else
    {
        c->sel_ix = -1;
        check_scrl(c);
        __ui_set_flag_state(ui, UI_STATE_HIGHLIGHTED, 0);
        __ui_set_flag_state(ui, UI_STATE_PRESSED, 0);
        ui->state &= ~UI_STATE_TRANSLATING;
    }
    return res;
}

static void list_paint(ui_component_t *ui, void *ctx, ui_info_t *i)
{
    static disp_clip_t old_clip;
    ui_list_t *c = (ui_list_t *)ui;
    c->b_up.comp.paint(&c->b_up.comp, ctx, i);
    c->b_down.comp.paint(&c->b_down.comp, ctx, i);
    if ((ui->state & UI_STATE_REPAINT) == 0)
        return;
    disp_get_clip(&old_clip);
    const uint16_t ih = item_height(c);
    uint32_t ix = c->vp_y / ih;
    int16_t y_offs = -(c->vp_y % ih);
    int16_t x = c->comp.x + (i ? i->x : 0);
    int16_t w = c->comp.w - BUT_W;
    int16_t y = c->comp.y + (i ? i->y : 0);
    int16_t y1 = y + c->comp.h;
    int16_t txl, txc, txr, ty;
    disp_align_t alignv;
    txc = x + 8 + (w - 10) / 2;
    txr = x + 8 + (w - 10);
    txl = x + 8;

    switch (ui->state & UI_STATE_ALIGNV__MASK)
    {
    case UI_STATE_ALIGNV_CENTER:
        ty = y + ih / 2;
        alignv = DISP_STR_ALIGN_CENTER;
        break;
    case UI_STATE_ALIGNV_BOTTOM:
        ty = y + ih;
        alignv = DISP_STR_ALIGN_RIGHT_BOTTOM;
        break;
    default:
        ty = y;
        alignv = DISP_STR_ALIGN_LEFT_TOP;
        break;
    }

    disp_set_clip_d(x, y, x + w - 1, y1);
    y += y_offs;
    ty += y_offs;
    if (c->paint_item_fn)
    {
        while (ix < c->items && y < y1)
        {
            c->paint_item_fn(ix, x, y, w, ih);
            ix++;
            y += ih;
            ty += ih;
        }
    }
    else
    {
        uint32_t cfg = c->colfg;
        uint32_t cbg1a = c->colbg1;
        uint32_t cbg1b = __ui_colfade(cbg1a, cfg, 0xe0);
        uint32_t cbg2 = c->colbg2;
        while (ix < c->items && y < y1)
        {
            uint32_t ccbg1 = (ix & 1) ? cbg1a : cbg1b;
            uint32_t fg = ix == c->sel_ix ? cbg2 : cfg;
            uint32_t bg1 = ix == c->sel_ix ? cfg : ccbg1;
            uint32_t bg2 = ix == c->sel_ix ? ccbg1 : cbg2;
            disp_fill(x, y, w, ih, bg1);
            disp_draw_hline(x, y, w, bg2, 2);
            disp_draw_hline(x, y + ih - 1, w, bg2, 2);
            disp_draw_vline(x, y, ih, bg2, 2);
            disp_draw_vline(x + w - 1, y, ih, bg2, 2);

            if (c->get_item_fn)
            {
                const char *strl, *strc, *strr;
                c->get_item_fn(ix, &strl, &strc, &strr);
                if (strl)
                    disp_draw_string(strl, txl + 2, ty + 1, c->font, DISP_STR_ALIGN_LEFT_TOP, alignv, fg, bg1);
                if (strc)
                    disp_draw_string(strc, txc + 2, ty + 1, c->font, DISP_STR_ALIGN_CENTER, alignv, fg, bg1);
                if (strr)
                    disp_draw_string(strr, txr + 2, ty + 1, c->font, DISP_STR_ALIGN_RIGHT_BOTTOM, alignv, fg, bg1);
            }
            else
            {
                char str[12];
                snprintf(str, sizeof(str) - 1, "ix %d", ix);
                disp_draw_string(str, txl + 2, ty + 1, c->font,
                                 UI_ALIGNH_LEFT, alignv, fg, bg1);
            }

            ix++;
            y += ih;
            ty += ih;
        }
    }
    if (y < y1)
    {
        disp_fill(x, y, w, y1 - y, c->colbg2);
    }
    disp_set_clip(&old_clip);
}

static int list_but_cb(ui_component_t *c, ui_event_t *e, ui_info_t *i)
{
    ui_button_t *button = (ui_button_t *)c;
    ui_list_t *list = (ui_list_t *)c->parent;
    const uint16_t ih = item_height(list);
    move_viewport_y(list, button->text[0] == TXT_UP[0] ? -ih : ih);
    timer_stop(&scrl_timer);
    list->comp.state |= UI_STATE_REPAINT;
    return 0;
}

void ui_list_init(ui_list_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  uint32_t items,
                  void (*get_item_fn)(uint32_t ix, const char **str_left, const char **str_center, const char **str_right),
                  void (*paint_item_fn)(uint32_t ix, int16_t x, int16_t y, uint16_t w, uint16_t h),
                  event_fn_t callback, void *user,
                  uint8_t font, uint32_t colfg, uint32_t colbg1, uint32_t colbg2)
{
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "LST\0";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = list_paint;
    ui->comp.handle_event = list_event;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    ui->call.cb = callback;
    ui->call.user = user;
    ui->items = items;
    ui->sel_ix = -1;
    ui->get_item_fn = get_item_fn;
    ui->paint_item_fn = paint_item_fn;
    ui->font = font;
    ui->colfg = colfg;
    ui->colbg1 = colbg1;
    ui->colbg2 = colbg2;
    ui->item_padding = 6;
    ui_button_init(&ui->b_up, x + w - BUT_W, y, BUT_W, h / 2, TXT_UP, list_but_cb,
                   ui, 5, COL_BUTTON_FG, COL_BUTTON_BG1, COL_BUTTON_BG2);
    ui_button_init(&ui->b_down, x + w - BUT_W, y + h / 2, BUT_W, h / 2, TXT_DOWN, list_but_cb,
                   ui, 5, COL_BUTTON_FG, COL_BUTTON_BG1, COL_BUTTON_BG2);
    ui_set_alignment(&ui->b_up.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
    ui_set_alignment(&ui->b_down.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
    ui_add(&ui->comp, &ui->b_up.comp);
    ui_add(&ui->comp, &ui->b_down.comp);
}
