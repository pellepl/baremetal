#pragma once

#include <stdint.h>
#include "disp.h"

typedef struct
{
    int x0, x1, y0, y1;
} gfx_area_t;

typedef struct
{
    gfx_area_t clip;
    uint8_t *buf;
    uint16_t stride;
    uint32_t tick;
} gfx_ctx_t;

typedef uint8_t gfx_codepoint_t;

typedef struct
{
    uint32_t raw_offset;
    uint8_t width;
    uint8_t y_begin;
    uint8_t height;
} gfx_glyph_t;

typedef struct
{
    gfx_codepoint_t start;
    gfx_codepoint_t end;
} gfx_codepoint_range_t;

typedef struct gl4_font
{
    const gfx_glyph_t *glyph_meta;
    const uint8_t *texel_data;
    const gfx_codepoint_range_t *ranges;
    uint16_t glyph_cnt;
    uint8_t ranges_count;
    uint8_t max_height;
} gfx_font_t;

void gfx_ctx_init(gfx_ctx_t *ctx);
void gfx_fill(const gfx_ctx_t *ctx, const gfx_area_t *a, int color);
void gfx_vline(const gfx_ctx_t *ctx, int x, int y0, int y1, int color);
void gfx_hline(const gfx_ctx_t *ctx, int x0, int x1, int y, int color);
void gfx_plot(const gfx_ctx_t *ctx, int x, int y, int color);
/** glyphs are only clipped every 8th pixel vertically */
void gfx_glyph(const gfx_ctx_t *ctx, const gfx_font_t *font, gfx_codepoint_t glyph, int x, int y);
/** glyphs are only clipped every 8th pixel vertically */
void gfx_string(const gfx_ctx_t *ctx, const gfx_font_t *font, const char *str, int x, int y);
int gfx_string_width(const gfx_font_t *font, const char *str);
void gfx_init(void);
