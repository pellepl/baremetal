#include <stddef.h>

#include "events.h"
#include "minio.h"
#include "ui.h"
#include "cli.h"
#include "rtc.h"
#include "timer.h"
#include "ui_views.h"
#include "settings.h"
#include "sys.h"

static struct
{
    volatile bool inactivate;
    volatile bool activate;
    volatile bool disp_enabled;
    volatile bool ongoing_disp_command;
    volatile bool pending_disp_command;
    const ui_view_t *view_active;
    const ui_view_t *view_inactive;
    ui_tick_t next_paint_update;
    event_t ev_scrl;
    event_t ev_press;
    event_t ev_back;
    event_t ev_repaint;
    timer_t timer_repaint;
    gfx_ctx_t ctx;
    uint32_t last_active_s;
} me;

static void repaint_timer_cb(timer_t *timer)
{
    event_add(&me.ev_repaint, EVENT_UI_DISP_UPDATE, NULL);
}

static void disp_command_done_cb(void) {
    me.ongoing_disp_command = false;
    if (me.pending_disp_command) {
        me.pending_disp_command = false;
        event_add(&me.ev_repaint, EVENT_UI_DISP_UPDATE, NULL);
    }
}

static void disp_enable_done_cb(int err)
{
    disp_command_done_cb();
}

static void disp_update_done_cb(int err)
{
    disp_command_done_cb();
    if (me.next_paint_update == UI_TICK_NEVER)
    {
        return;
    }
    if (me.next_paint_update == 0)
    {
        event_add(&me.ev_repaint, EVENT_UI_DISP_UPDATE, NULL);
        return;
    }
    timer_start(&me.timer_repaint, repaint_timer_cb, me.next_paint_update * DISP_TRANSFER_TICKS, TIMER_ONESHOT);
}

static void ui_disp_update(void)
{
    me.ctx.tick = timer_now() / DISP_TRANSFER_TICKS;
    if (me.ongoing_disp_command) {
        me.pending_disp_command = true;
        return;
    }
    timer_stop(&me.timer_repaint);

    if (me.activate)
    {
        if (!me.disp_enabled)
        {
            printf("activate disp\n");
            me.ongoing_disp_command = true;
            me.pending_disp_command = true; // force another disp update after enabling it, in order to call current view update func
            disp_set_enabled(true, disp_enable_done_cb);
            me.disp_enabled = true;
            me.activate = false;
        }
        return;
    }
    else if (me.inactivate)
    {
        if (me.disp_enabled)
        {
            printf("inactivate disp\n");
            me.ongoing_disp_command = true;
            disp_set_enabled(false, disp_enable_done_cb);
            me.disp_enabled = false;
            me.inactivate = false;
        }
        return;
    }
    if (!me.disp_enabled)
        return;

    gfx_ctx_init(&me.ctx);
    ui_tick_t paint_update = UI_TICK_NEVER;
    if (me.view_inactive)
    {
        ui_tick_t pu = me.view_inactive->paint(&me.ctx);
        if (pu < paint_update)
            paint_update = pu;
    }
    if (me.view_active)
    {
        ui_tick_t pu = me.view_active->paint(&me.ctx);
        if (pu < paint_update)
            paint_update = pu;
    }
    me.next_paint_update = paint_update;
    me.ongoing_disp_command = true;
    disp_screen_update(disp_update_done_cb);
}

void ui_trigger_update(void)
{
    event_add(&me.ev_repaint, EVENT_UI_DISP_UPDATE, NULL);
}

void ui_set_view(const ui_view_t *v)
{
    if (me.view_active && me.view_active->exit)
    {
        me.view_active->exit();
    }
    if (v->enter)
    {
        v->enter();
    }
    me.view_active = v;
    ui_trigger_update();
}

int ui_move_towards(int current, int target)
{
#define DIV 8
    if (current == target)
        return target;
    int d = target - current;
    if (d < 0 && d > -DIV)
        return current - 1;
    if (d > 0 && d < DIV)
        return current + 1;
    return current + d / DIV;
}

void ui_init(void)
{
    me.disp_enabled = true;
    ui_set_view(&view_menu);
}

void ui_active(void)
{
    me.last_active_s = sys_get_up_second();
    if (!me.disp_enabled)
    {
        me.inactivate = false;
        me.activate = true;
        ui_trigger_update();
    }
}

static void ui_event(uint32_t type, void *arg)
{
    switch (type)
    {
    case EVENT_UI_BACK:
    case EVENT_UI_PRESS:
    case EVENT_UI_SCRL:
        ui_active();
        break;

    case EVENT_SECOND_TICK:
        if (me.disp_enabled)
        {
            uint32_t now_s = (uint32_t)arg;
            if (now_s - me.last_active_s > (uint32_t)settings_get_val_for_id(SETTING_SCREEN_TIMEOUT))
            {
                me.activate = false;
                me.inactivate = true;
                ui_trigger_update();
            }
        }
        break;
    case EVENT_UI_DISP_UPDATE:
        ui_disp_update();
        return;
    default:
        break;
    }
    if (me.view_active == NULL)
        return;
    me.view_active->handle_event(type, arg);
}
EVENT_HANDLER(ui_event);

static int cli_ui_ev_scrl(int argc, const char **argv)
{
    event_add(&me.ev_scrl, EVENT_UI_SCRL, (void *)(argc == 0 ? 1 : strtol(argv[0], NULL, 0)));
    return 0;
}
CLI_FUNCTION(cli_ui_ev_scrl, "ev_scrl", "");
static int cli_ui_ev_press(int argc, const char **argv)
{
    event_add(&me.ev_press, EVENT_UI_PRESS, (void *)(argc == 0 ? 1 : strtol(argv[0], NULL, 0)));
    return 0;
}
CLI_FUNCTION(cli_ui_ev_press, "ev_press", "");
static int cli_ui_ev_back(int argc, const char **argv)
{
    event_add(&me.ev_back, EVENT_UI_BACK, (void *)(argc == 0 ? 1 : strtol(argv[0], NULL, 0)));
    return 0;
}
CLI_FUNCTION(cli_ui_ev_back, "ev_back", "");
