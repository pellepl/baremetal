#pragma once

#include <stdint.h>
#include "events.h"
#include "gfx.h"
#include "font_roboto_semibold_13.h"
#include "font_roboto_semibold_16.h"

#define UI_FONT_ITEM_SMALL (&font_roboto_semibold_13)
#define UI_FONT_ITEM (&font_roboto_semibold_16)
#define UI_TICK_NEVER (ui_tick_t)0xffffffff

typedef uint32_t ui_tick_t;
typedef void (*ui_enter_func_t)(void);
typedef ui_tick_t (*ui_paint_func_t)(const gfx_ctx_t *ctx);
typedef void (*ui_exit_func_t)(void);

typedef struct
{
    ui_enter_func_t enter;
    ui_exit_func_t exit;
    event_func_t handle_event;
    ui_paint_func_t paint;
} ui_view_t;

void ui_set_view(const ui_view_t *view);
void ui_trigger_update(void);
void ui_init(void);
int ui_move_towards(int current, int target);
void ui_active(void);