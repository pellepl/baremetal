#include <stddef.h>
#include <stdint.h>
#include "cmsis_gcc.h"
#include "cli.h"
#include "minio.h"
#include "gfx.h"
#include "font_pixeloidsans_9.h"
#include "font_roboto_semibold_13.h"
#include "font_roboto_semibold_16.h"
#include "font_roboto_extrabold_30.h"

static const gfx_glyph_t *get_glyph(const gfx_font_t *font, gfx_codepoint_t glyph)
{
    uint32_t offs = 0;
    for (uint8_t i = 0; i < font->ranges_count; i++)
    {
        if (glyph >= font->ranges[i].start && glyph <= font->ranges[i].end)
        {
            return &font->glyph_meta[offs + (glyph - font->ranges[i].start)];
        }
        offs += font->ranges[i].end - font->ranges[i].start + 1;
    }
    return NULL;
}

static void draw_glyph(const gfx_ctx_t *ctx, const gfx_font_t *font, const gfx_glyph_t *g, int x, int y, gfx_color_t c)
{
    if (x >= ctx->clip.x1)
        return;
    if (y >= ctx->clip.y1)
        return;
    if (x + g->width < ctx->clip.x0)
        return;
    if (y + g->height < ctx->clip.y0)
        return;

    const uint8_t *texels_8 = &font->texel_data[g->raw_offset];
    const uint8_t bytes_per_col = (g->height + 7) / 8;
    const uint8_t bytes_to_draw = bytes_per_col + ((y % 8) != 0 ? 1 : 0);
    const uint32_t glyph_y_mask = g->height >= 31 ? 0xffffffff : (uint32_t)((1 << (g->height + 1)) - 1);
    const uint8_t y_lsb = y & 7;
    uint8_t *addr = &ctx->buf[(y & ~7) / 8 * ctx->stride + x];
    uint8_t g_width = x + g->width < ctx->clip.x1 ? g->width : (ctx->clip.x1 - x);
    for (uint8_t gx = 0; gx < g_width; gx++)
    {
        uint32_t t32 = *((const uint32_t *)texels_8);
        texels_8 += bytes_per_col;
        if (x + gx < ctx->clip.x0)
            continue;
        t32 = __REV(__RBIT(t32)) & glyph_y_mask;
        uint32_t t32_rem = t32 >> (24 + 8 - y_lsb);
        t32 <<= y_lsb;
        for (uint8_t gy = 0; gy < bytes_to_draw; gy++)
        {
            if (y + gy * 8 >= ctx->clip.y1)
                break;
            if (y + gy * 8 >= ctx->clip.y0)
            {
                switch (c)
                {
                case GFX_COL_SET:
                    addr[gx + gy * ctx->stride] |= t32 & 0xff;
                    break;
                case GFX_COL_CLR:
                    addr[gx + gy * ctx->stride] &= ~(t32 & 0xff);
                    break;
                case GFX_COL_XOR:
                    addr[gx + gy * ctx->stride] ^= t32 & 0xff;
                    break;
                }
            }
            t32 >>= 8;
            if (gy == 3)
                t32 |= t32_rem;
        }
    }
}

void gfx_glyph(const gfx_ctx_t *ctx, const gfx_font_t *font, gfx_codepoint_t glyph, int x, int y, gfx_color_t c)
{
    const gfx_glyph_t *g = get_glyph(font, glyph);
    if (g == NULL)
        return;
    draw_glyph(ctx, font, g, x, y, c);
}

void gfx_string(const gfx_ctx_t *ctx, const gfx_font_t *font, const char *str, int x, int y, gfx_color_t c)
{
    if (str == NULL)
        return;
    if (x >= ctx->clip.x1)
        return;
    if (y >= ctx->clip.y1)
        return;
    if (y + font->max_height < ctx->clip.y0)
        return;
    gfx_codepoint_t cp;
    while ((cp = *str++) != 0)
    {
        const gfx_glyph_t *g = get_glyph(font, cp);
        if (g == NULL)
            continue;

        if (x + g->width >= ctx->clip.x0)
        {
            draw_glyph(ctx, font, g, x, y, c);
        }
        x += g->width;
        if (x >= ctx->clip.x1)
            return;
    }
}

int gfx_string_width(const gfx_font_t *font, const char *str)
{
    gfx_codepoint_t c;
    int res = 0;
    while ((c = *str++) != 0)
    {
        const gfx_glyph_t *g = get_glyph(font, c);
        if (g == NULL)
            continue;
        res += g->width;
    }
    return res;
}

void gfx_fill(const gfx_ctx_t *ctx, const gfx_area_t *a, gfx_color_t c)
{
    int x0 = a->x0;
    int y0 = a->y0;
    int x1 = a->x1;
    int y1 = a->y1;
    if (x1 < x0)
    {
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    if (x0 < ctx->clip.x0)
        x0 = ctx->clip.x0;
    if (x1 >= ctx->clip.x1)
        x1 = ctx->clip.x1 - 1;
    if (x1 < x0)
        return;
    if (y1 < y0)
    {
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    if (y0 < ctx->clip.y0)
        y0 = ctx->clip.y0;
    if (y1 >= ctx->clip.y1)
        y1 = ctx->clip.y1 - 1;
    if (y1 < y0)
        return;

    const uint8_t bp_vline_start = 0xff << (y0 & 7);
    const uint8_t bp_vline_end = 0xff >> (7 - (y1 & 7));
    const uint8_t page_start = y0 / 8;
    const uint8_t page_end = y1 / 8;
    uint8_t *addr = &ctx->buf[page_start * ctx->stride];
    if (page_start == page_end)
    {
        uint8_t bp = bp_vline_start & bp_vline_end;
        switch (c)
        {
        case GFX_COL_SET:
            for (int x = x0; x <= x1; x++)
                addr[x] |= bp;
            break;
        case GFX_COL_CLR:
            for (int x = x0; x <= x1; x++)
                addr[x] &= ~bp;
            break;
        case GFX_COL_XOR:
            for (int x = x0; x <= x1; x++)
                addr[x] ^= bp;
            break;
        }
        return;
    }
    switch (c)
    {
    case GFX_COL_SET:
        for (int x = x0; x <= x1; x++)
            addr[x] |= bp_vline_start;
        break;
    case GFX_COL_CLR:
        for (int x = x0; x <= x1; x++)
            addr[x] &= ~bp_vline_start;
        break;
    case GFX_COL_XOR:
        for (int x = x0; x <= x1; x++)
            addr[x] ^= bp_vline_start;
        break;
    }
    addr += ctx->stride;
    switch (c)
    {
    case GFX_COL_SET:
        for (uint8_t page = page_start + 1; page < page_end; page++, addr += ctx->stride)
            memset(&addr[x0], 0xff, x1 - x0 + 1);
        break;
    case GFX_COL_CLR:
        for (uint8_t page = page_start + 1; page < page_end; page++, addr += ctx->stride)
            memset(&addr[x0], 0x00, x1 - x0 + 1);
        break;
    case GFX_COL_XOR:
        for (uint8_t page = page_start + 1; page < page_end; page++, addr += ctx->stride)
            for (int xx = x0; xx <= x1; xx++)
                addr[xx] ^= 0xff;
        break;
    }
    switch (c)
    {
    case GFX_COL_SET:
        for (int x = x0; x <= x1; x++)
            addr[x] |= bp_vline_end;
        break;
    case GFX_COL_CLR:
        for (int x = x0; x <= x1; x++)
            addr[x] &= ~bp_vline_end;
        break;
    case GFX_COL_XOR:
        for (int x = x0; x <= x1; x++)
            addr[x] ^= bp_vline_end;
        break;
    }
}

void gfx_vline(const gfx_ctx_t *ctx, int x, int y0, int y1, gfx_color_t c)
{
    if (x < ctx->clip.x0 || x >= ctx->clip.x1)
        return;
    if (y1 < y0)
    {
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    if (y0 < ctx->clip.y0)
        y0 = ctx->clip.y0;
    if (y1 >= ctx->clip.y1)
        y1 = ctx->clip.y1 - 1;
    if (y1 < y0)
        return;
    const uint8_t bp_vline_start = 0xff << (y0 & 7);
    const uint8_t bp_vline_end = 0xff >> (7 - (y1 & 7));
    const uint8_t page_start = y0 / 8;
    const uint8_t page_end = y1 / 8;
    uint8_t *addr = &ctx->buf[x + page_start * ctx->stride];
    if (page_start == page_end)
    {
        uint8_t bp = bp_vline_start & bp_vline_end;
        switch (c)
        {
        case GFX_COL_SET:
            *addr |= bp;
            break;
        case GFX_COL_CLR:
            *addr &= ~bp;
            break;
        case GFX_COL_XOR:
            *addr ^= bp;
            break;
        }
        return;
    }
    switch (c)
    {
    case GFX_COL_SET:
        *addr |= bp_vline_start;
        break;
    case GFX_COL_CLR:
        *addr &= ~bp_vline_start;
        break;
    case GFX_COL_XOR:
        *addr ^= bp_vline_start;
        break;
    }
    addr += ctx->stride;
    switch (c)
    {
    case GFX_COL_SET:
        for (uint8_t page = page_start + 1; page < page_end; page++, addr += ctx->stride)
            *addr = 0xff;
        break;
    case GFX_COL_CLR:
        for (uint8_t page = page_start + 1; page < page_end; page++, addr += ctx->stride)
            *addr = 0x00;
        break;
    case GFX_COL_XOR:
        for (uint8_t page = page_start + 1; page < page_end; page++, addr += ctx->stride)
            *addr ^= 0xff;
        break;
    }
    switch (c)
    {
    case GFX_COL_SET:
        *addr |= bp_vline_end;
        break;
    case GFX_COL_CLR:
        *addr &= ~bp_vline_end;
        break;
    case GFX_COL_XOR:
        *addr ^= bp_vline_end;
        break;
    }
}

void gfx_hline(const gfx_ctx_t *ctx, int x0, int x1, int y, gfx_color_t c)
{
    if (y < ctx->clip.y0 || y >= ctx->clip.y1)
        return;
    if (x1 < x0)
    {
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    if (x0 < ctx->clip.x0)
        x0 = ctx->clip.x0;
    if (x1 >= ctx->clip.x1)
        x1 = ctx->clip.x1 - 1;
    if (x1 < x0)
        return;

    uint8_t bp = 1 << (y & 7);
    uint8_t *addr = &ctx->buf[(y / 8) * ctx->stride];
    for (int x = x0; x <= x1; x++)
        switch (c)
        {
        case GFX_COL_SET:
            addr[x] |= bp;
            break;
        case GFX_COL_CLR:
            addr[x] &= ~bp;
            break;
        case GFX_COL_XOR:
            addr[x] ^= bp;
            break;
        }
}

void gfx_plot(const gfx_ctx_t *ctx, int x, int y, gfx_color_t c)
{
    if (x < ctx->clip.x0 || x >= ctx->clip.x1 || y < ctx->clip.y0 || y >= ctx->clip.y1)
        return;
    uint8_t bp = 1 << (y & 7);
    uint8_t *addr = &ctx->buf[x + (y / 8) * ctx->stride];
    switch (c)
    {
    case GFX_COL_SET:
        *addr |= bp;
        break;
    case GFX_COL_CLR:
        *addr &= ~bp;
        break;
    case GFX_COL_XOR:
        *addr ^= bp;
        break;
    }
}

void gfx_ctx_init(gfx_ctx_t *ctx)
{
    ctx->buf = disp_gbuf();
    ctx->clip = (gfx_area_t){
        .x0 = 0, .y0 = 0, .x1 = DISP_W, .y1 = DISP_H};
    ctx->stride = DISP_W;
    memset(ctx->buf, 0, DISP_W * DISP_H / 8);
}

void gfx_ctx_move(gfx_ctx_t *ctx, int dx, int dy)
{
    ctx->buf += dx;
    ctx->clip.x0 -= dx;
    ctx->clip.x1 -= dx;

    dy &= ~7;
    ctx->buf += (dy / 8) * ctx->stride;
    ctx->clip.y0 -= dy;
    ctx->clip.y1 -= dy;
}

void gfx_init(void)
{
    disp_init();
}
