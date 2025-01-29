#include "bmtypes.h"
#include "disp.h"
#include "minio.h"
#include "fonts/fonts.h"
#include "ui.h"
#include "settings.h"
#include "rtc.h"

#define ACC (8)

#define VAL_TO_Y(val) (y + (((h << ACC) - (((val) - min) * fymul)) >> ACC))
#define CLEAR_LINE(x)                                                            \
    {                                                                            \
        disp_draw_vline(x, y, h, x_grid ? gridcol_dark : bg, 1);                 \
        if (ysep)                                                                \
        {                                                                        \
            for (int32_t sy = y_grid; sy <= max; sy += ysep)                     \
            {                                                                    \
                disp_draw_pixel((x), VAL_TO_Y(sy), sy ? gridcol_dark : gridcol); \
            }                                                                    \
        }                                                                        \
    }

static void graph_paint(ui_component_t *ui, void *ctx, ui_info_t *i)
{
    if ((ui->state & UI_STATE_REPAINT) == 0)
        return;
    static disp_clip_t old_clip;
    ui_graph_t *c = (ui_graph_t *)ui;
    uint32_t fg = c->colfg;
    uint32_t bg = c->colbg;
    uint32_t gridcol = c->colgrid;
    uint32_t gridcol_dark = __ui_colfade(c->colgrid, bg, 0x80);
    uint16_t x = c->comp.x + (i ? i->x : 0);
    uint16_t y = c->comp.y + (i ? i->y : 0);
    uint16_t w = c->comp.w;
    uint16_t h = c->comp.h;

    int32_t min = c->_value_min > (-0x7fff + 5) ? c->_value_min - 5 : -0x7fff;
    int32_t max = c->_value_max < (0x7fff - 5) ? c->_value_max + 5 : 0x7fff;
    if (min > c->user_min)
        min = c->user_min;
    if (max < c->user_max)
        max = c->user_max;

    int ysep = 0;
    int y_grid = 0;
    {
        int32_t d = max - min;
        if (d >= 10000)
            ysep = 2500;
        else if (d >= 5000)
            ysep = 1000;
        else if (d >= 2500)
            ysep = 500;
        else if (d >= 1000)
            ysep = 250;
        else if (d >= 500)
            ysep = 100;
        else if (d >= 250)
            ysep = 50;
        else if (d >= 100)
            ysep = 25;
        else if (d >= 50)
            ysep = 10;
        else if (d >= 25)
            ysep = 5;
        else if (d >= 10)
            ysep = 2;
        else if (d >= 5)
            ysep = 1;
    }
    if (ysep)
        y_grid = (min / ysep) * ysep;

    int32_t fxmul = (c->_value_count << ACC) / w;
    int32_t fymul = (h << ACC) / (max - min);
    uint32_t ix = (c->_value_count < c->_value_size ? 0 : c->_value_ix) << ACC;
    int32_t prev_y = VAL_TO_Y(c->_values[ix >> ACC]);

    uint32_t next_ix_grid = 0;
    int xsep_s = 0;
    int xsep_g = 0;
    int32_t time_rep = c->_value_count * (int32_t)settings_get_val_for_id(SETTING_TEMP_GRAPH_UPDATE);
    {
        if (time_rep >= 3600 * 24)
            xsep_s = 60 * 60 * 8;
        else if (time_rep >= 3600 * 12)
            xsep_s = 60 * 60 * 4;
        else if (time_rep >= 3600 * 8)
            xsep_s = 60 * 60 * 2;
        else if (time_rep >= 3600 * 6)
            xsep_s = 60 * 60;
        else if (time_rep >= 3600 * 3)
            xsep_s = 30 * 60;
        else if (time_rep >= 3600 * 2)
            xsep_s = 20 * 60;
        else if (time_rep >= 1200)
            xsep_s = 10 * 60;
        else if (time_rep >= 900)
            xsep_s = 5 * 60;
        else if (time_rep >= 600)
            xsep_s = 2 * 60;
        else if (time_rep >= 300)
            xsep_s = 1 * 60;
        else if (time_rep >= 120)
            xsep_s = 30;
        else if (time_rep >= 60)
            xsep_s = 20;
        else if (time_rep >= 30)
            xsep_s = 10;
        else if (time_rep >= 10)
            xsep_s = 5;
        else if (time_rep >= 5)
            xsep_s = 1;
        xsep_s <<= ACC;
        xsep_g = xsep_s / (int32_t)settings_get_val_for_id(SETTING_TEMP_GRAPH_UPDATE);
    }
    int32_t starting_grid_sec = 0;
    if (xsep_s)
    {
        int32_t secs = 3600 * c->_last_add_dt.time.hour + 60 * c->_last_add_dt.time.minute + c->_last_add_dt.time.second;
        secs -= time_rep;
        while (secs < 0)
            secs += 3600 * 24;
        int32_t ss = secs % (xsep_s >> ACC);
        ss = (xsep_s >> ACC) - ss;
        starting_grid_sec = secs + ss;
        while (starting_grid_sec < 0)
            starting_grid_sec += 3600 * 24;
        next_ix_grid = ix + (ss << ACC) / (int32_t)settings_get_val_for_id(SETTING_TEMP_GRAPH_UPDATE);
    }
    int16_t first_x_grid = -1;
    disp_get_clip(&old_clip);
    disp_set_clip_d(x, y, x + w, y + h);

    const font_t *f = font_get(1);
    int x_grid = xsep_s && (ix >= next_ix_grid);
    if (x_grid)
        next_ix_grid += xsep_g;
    int least_label_xx = x + 1;
    CLEAR_LINE(x);

    for (uint16_t xx = x + 1; xx < x + w; xx++)
    {
        int16_t val = c->_values[ix >> ACC];
        ix += fxmul;
        if (ix >= (uint32_t)(c->_value_size << ACC))
        {
            ix -= (c->_value_size << ACC);
            next_ix_grid -= (c->_value_size << ACC);
        }
        x_grid = xsep_g && (ix >= next_ix_grid);
        if (x_grid)
        {
            next_ix_grid += xsep_g;
            if (first_x_grid < 0)
                first_x_grid = xx;
        }
        int32_t cur_y = VAL_TO_Y(val);
        CLEAR_LINE(xx);
        if (x_grid)
        {
            if (xx >= least_label_xx)
            {
                int hour = (starting_grid_sec / 3600) % 24;
                int minute = (starting_grid_sec / 60) % 60;
                int second = starting_grid_sec % 60;
                starting_grid_sec += (xsep_s >> ACC);
                if ((xsep_s >> ACC) < 60)
                    disp_draw_stringf("%02dm%02d", xx, y, 1,
                                      DISP_STR_ALIGN_RIGHT_BOTTOM, DISP_STR_ALIGN_LEFT_TOP,
                                      gridcol, 0,
                                      minute, second);
                else
                    disp_draw_stringf("%02dh%02d", xx, y, 1,
                                      DISP_STR_ALIGN_RIGHT_BOTTOM, DISP_STR_ALIGN_LEFT_TOP,
                                      gridcol, 0,
                                      hour, minute);
                least_label_xx = xx + f->advance * (5 + 2);
            }
        }
        disp_draw_line(xx - 1, prev_y, xx, cur_y, fg, 1);
        prev_y = cur_y;
        if (xx == (x + 6 * 4))
        {
            for (int32_t sy = y_grid; sy <= max; sy += ysep)
            {
                disp_draw_stringf("%d", x, VAL_TO_Y(sy), 1,
                                  DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_RIGHT_BOTTOM,
                                  sy ? gridcol_dark : gridcol, 0,
                                  sy);
                if (ysep == 0)
                    break;
            }
        }
    }

    {
        int32_t ix_max_offs = c->_ix_max - (c->_value_count < c->_value_size ? 0 : c->_value_ix);
        if (ix_max_offs < 0)
            ix_max_offs += c->_value_count;
        int32_t x_max = 4 + (((x << ACC) + ((w * ix_max_offs) << ACC) / c->_value_count) >> ACC);
        int32_t y_max = VAL_TO_Y(c->_value_max);
        if (y_max > y + 16)
            y_max -= 16;
        if (x_max > x + w - 8 * 4)
            x_max -= 8 * 4;
        disp_draw_stringf("%d", x_max, y_max, 2,
                          x_max > x + w - 16 ? DISP_STR_ALIGN_RIGHT_BOTTOM : DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP,
                          0xffff7777, 0,
                          c->_value_max);
    }

    disp_set_clip(&old_clip);
}

void ui_graph_init(ui_graph_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   event_fn_t callback, void *user,
                   int16_t *buf, uint16_t buf_size,
                   uint32_t colfg, uint32_t colbg)
{
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "GRA";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = graph_paint;
    ui->comp.handle_event = button_event;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    ui->call.cb = callback;
    ui->call.user = user;
    ui->colfg = colfg;
    ui->colbg = colbg;
    ui->_values = buf;
    ui->_value_size = buf_size;
    ui->_value_count = ui->_value_max = ui->_value_min = ui->_value_ix = 0;
    ui->_ix_min = ui->_ix_max = 0;
    ui->user_max = -0x7ff0;
    ui->user_min = 0x7ff0;
    ui->colgrid = 0xff00aa00;
}

void ui_graph_add(ui_graph_t *c, int16_t value)
{
    rtc_get_date_time(&c->_last_add_dt);
    c->comp.state |= UI_STATE_REPAINT;
    if (c->_value_count == 0)
    {
        c->_value_max = c->_value_min = value;
        c->_ix_max = c->_ix_min = 0;
    }
    else
    {
        if (value >= c->_value_max)
        {
            c->_value_max = value;
            c->_ix_max = c->_value_ix;
        }
        if (value <= c->_value_min)
        {
            c->_value_min = value;
            c->_ix_min = c->_value_ix;
        }
    }
    c->_values[c->_value_ix] = value;
    int new_limits = 0;
    if (c->_value_count > c->_value_size)
    {
        if (c->_value_ix == c->_ix_max || c->_value_ix == c->_ix_min)
        {
            new_limits = 1;
        }
    }
    else
    {
        c->_value_count++;
    }
    c->_value_ix++;
    if (c->_value_ix >= c->_value_size)
        c->_value_ix = 0;
    if (new_limits)
    {
        c->_value_max = c->_value_min = value;
        for (uint16_t i = 0; i < c->_value_size; i++)
        {
            int16_t v = c->_values[i];
            if (v >= c->_value_max)
            {
                c->_value_max = v;
                c->_ix_max = i;
            }
            if (v <= c->_value_min)
            {
                c->_value_min = v;
                c->_ix_min = i;
            }
        }
    }
}

int16_t ui_graph_get_past(ui_graph_t *c, uint16_t indices_in_past)
{
    if (indices_in_past >= c->_value_count)
    {
        indices_in_past = c->_value_count - 1;
    }
    uint16_t ix = c->_value_ix < indices_in_past ? (c->_value_ix + c->_value_size) - indices_in_past : c->_value_ix - indices_in_past;
    return c->_values[ix];
}

void ui_graph_clear(ui_graph_t *c)
{
    c->comp.state |= UI_STATE_REPAINT;
    c->_value_count = c->_value_max = c->_value_min = c->_value_ix = 0;
    c->_ix_min = c->_ix_max = 0;
}
