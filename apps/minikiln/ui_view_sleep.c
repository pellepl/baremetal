#include "minio.h"
#include "ui.h"
#include "font_roboto_semibold_13.h"

static void handle_event(uint32_t type, void *arg)
{
    switch (type)
    {
    default:
        break;
    }
}

static ui_tick_t paint(const gfx_ctx_t *ctx)
{
    return UI_TICK_NEVER;
}

const ui_view_t view_sleep = {
    .handle_event = handle_event,
    .paint = paint};
