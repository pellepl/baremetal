#pragma once

#include <stdint.h>
#include "events.h"
#include "gfx.h"
#include "font_roboto_semibold_13.h"
#include "font_roboto_semibold_16.h"

#define ARRAY_LENGTH(x) (sizeof(x)/(sizeof((x)[0])))

#define UI_FONT_ITEM_SMALL (&font_roboto_semibold_13)
#define UI_FONT_ITEM (&font_roboto_semibold_16)
#define UI_TICK_NEVER (ui_tick_t)0xffffffff

#define UI_DECLARE_VIEW \
    const __attribute__((used, section(".ui_view"))) ui_view_t

extern const intptr_t __ui_views_start;
extern const intptr_t __ui_views_end;
#define UI_VIEWS_START (const ui_view_t *)&__ui_views_start
#define UI_VIEWS_END (const ui_view_t *)&__ui_views_end

typedef uint32_t ui_tick_t;
typedef void (*ui_init_func_t)(void);
typedef void (*ui_enter_func_t)(void);
typedef ui_tick_t (*ui_paint_func_t)(const gfx_ctx_t *ctx);
typedef void (*ui_exit_func_t)(void);

typedef struct
{
    ui_init_func_t init;
    ui_enter_func_t enter;
    ui_exit_func_t exit;
    event_func_t handle_event;
    ui_paint_func_t paint;
} ui_view_t;

static inline int max(int a, int b)
{
    return a > b ? a : b;
}
static inline int min(int a, int b)
{
    return a < b ? a : b;
}
static inline int abs(int a)
{
    return a < 0 ? -a : a;
}
static inline int clamp(int a, int min, int max)
{
    return a < min ? min : (a > max ? max : a);
}
static inline int lerp(int from, int to, int where, int end)
{
    where = clamp(where, 0, end);
    return from + ((to - from) * where) / end;
}

void ui_set_view(const ui_view_t *view);
void ui_trigger_update(void);
void ui_init(void);
void ui_goto_view(const ui_view_t *v, bool back);
int ui_move_towards(int current, int target);
void ui_active(void);
