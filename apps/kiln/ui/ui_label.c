#include "bmtypes.h"
#include "disp.h"
#include "minio.h"
#include "fonts/fonts.h"
#include "ui.h"

static void label_paint(ui_component_t *ui, void *ctx, ui_info_t *i)
{
    if ((ui->state & UI_STATE_REPAINT) == 0)
        return;
    static disp_clip_t old_clip;
    ui_label_t *c = (ui_label_t *)ui;
    uint32_t fg = c->colfg;
    uint32_t bg = c->colbg;
    uint16_t x = c->comp.x + (i ? i->x : 0);
    uint16_t y = c->comp.y + (i ? i->y : 0);
    uint16_t w = c->comp.w;
    uint16_t h = c->comp.h;
    const char *str = c->text;
    uint8_t font = c->font;
    uint32_t bordercol = c->comp.state & UI_STATE_HIGHLIGHTED ? fg : bg;
    w -= 1;
    h -= 1;
    if (bordercol & 0xff000000)
    {
        disp_draw_hline(x, y, w, bordercol, 1);
        disp_draw_hline(x, y + h, w, bordercol, 1);
        disp_draw_vline(x, y, h, bordercol, 1);
        disp_draw_vline(x + w, y, h, bordercol, 1);
    }

    if (bg & 0xff000000)
    {
        disp_fill(x + 1, y + 1, w - 1, h - 1, bg);
    }

    disp_get_clip(&old_clip);
    disp_set_clip_d(x + 1, y + 1, x + w - 2, y + h - 2);

    disp_align_t alignh, alignv;
    switch (ui->state & UI_STATE_ALIGNH__MASK)
    {
    case UI_STATE_ALIGNH_CENTER:
        x += w / 2;
        alignh = DISP_STR_ALIGN_CENTER;
        break;
    case UI_STATE_ALIGNH_RIGHT:
        x += w - 2;
        alignh = DISP_STR_ALIGN_RIGHT_BOTTOM;
        break;
    default:
        alignh = DISP_STR_ALIGN_LEFT_TOP;
        break;
    }
    switch (ui->state & UI_STATE_ALIGNV__MASK)
    {
    case UI_STATE_ALIGNV_CENTER:
        y += h / 2;
        alignv = DISP_STR_ALIGN_CENTER;
        break;
    case UI_STATE_ALIGNV_BOTTOM:
        y += h - 2;
        alignv = DISP_STR_ALIGN_RIGHT_BOTTOM;
        break;
    default:
        alignv = DISP_STR_ALIGN_LEFT_TOP;
        break;
    }

    x += c->offs_x;
    y += c->offs_y;

    disp_draw_string(str, x + 1, y + 1, font, alignh, alignv, fg, bg);
    disp_set_clip(&old_clip);
}

void ui_label_init(ui_label_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   const char *text, uint8_t font, uint32_t colfg, uint32_t colbg)
{
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "LBL";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = label_paint;
    ui->comp.handle_event = __ui_event_to_children;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    strncpy(ui->text, text, sizeof(ui->text) - 1);
    ui->font = ui->max_font = font;
    ui->colfg = colfg;
    ui->colbg = colbg;
    ui_set_alignment(&ui->comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
}

void ui_label_set_text(ui_label_t *ui, const char *text)
{
    strncpy(ui->text, text, sizeof(ui->text) - 1);
    uint8_t num = strlen(ui->text);
    for (int8_t f = ui->max_font; f >= 0; f--)
    {
        int16_t w = font_get(f)->advance * num;
        if (w <= ui->comp.w)
        {
            ui->font = f;
            break;
        }
    }
    ui->comp.state |= UI_STATE_REPAINT;
}
