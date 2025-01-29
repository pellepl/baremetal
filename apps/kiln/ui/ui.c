#include <stddef.h>
#include <stdint.h>
#include "disp.h"
#include "minio.h"
#include "ui.h"
#include "assert.h"
#include "cpu.h"

static volatile uint32_t event_pool_free_mask[(UI_EVENT_POOL + 31) / 32] = {0};
static ui_event_t event_pool[UI_EVENT_POOL];

static ui_event_t *event_q_head = 0;
static void (*ui_click_cb_fn)(ui_event_t *e) = 0;
static ui_panel_t *main_panel;

uint32_t __ui_colfade(uint32_t a, uint32_t b, uint8_t x)
{
    int ra = ((a >> 16) & 0xff);
    int rb = ((b >> 16) & 0xff);
    int ga = ((a >> 8) & 0xff);
    int gb = ((b >> 8) & 0xff);
    int ba = ((a >> 0) & 0xff);
    int bb = ((b >> 0) & 0xff);
    int32_t red = (x * (ra - rb) >> 8) + rb;
    int32_t gre = (x * (ga - gb) >> 8) + gb;
    int32_t blu = (x * (ba - bb) >> 8) + bb;

    if (red > 0xff)
        red = 0xff;
    else if (red < 0)
        red = 0;
    if (gre > 0xff)
        gre = 0xff;
    else if (gre < 0)
        gre = 0;
    if (blu > 0xff)
        blu = 0xff;
    else if (blu < 0)
        blu = 0;

    return 0xff000000 | (red << 16) | (gre << 8) | blu;
}

void __ui_paint_to_children(ui_component_t *ui, void *ctx, ui_info_t *info)
{
    if ((ui->state & UI_STATE_VISIBLE) == 0)
        return;
    info->x += ui->x;
    info->y += ui->y;
    ui_component_t *child = ui->children;
    while (child)
    {
        if ((child->state & UI_STATE_VISIBLE) && child->paint)
        {
            child->paint(child, ctx, info);
            child->state &= ~UI_STATE_REPAINT;
        }
        child = child->next;
    }
    info->x -= ui->x;
    info->y -= ui->y;
}

int __ui_event_to_children(ui_component_t *ui, ui_event_t *event, ui_info_t *info)
{
    int res = 0;
    if ((ui->state & UI_STATE_VISIBLE) == 0)
        return 0;
    if ((ui->state & UI_STATE_ENABLED) == 0)
        return 0;
    info->x += ui->x;
    info->y += ui->y;
    ui_component_t *child = ui->children;
    while (res == 0 && child)
    {
        if ((child->state & UI_STATE_VISIBLE) && (child->state & UI_STATE_ENABLED) && child->handle_event)
        {
            // if (event->type != EVENT_DRAG) printf("pass ev %d from %s to child %s\n", event->type, ui->_type, child->_type);
            res = child->handle_event(child, event, info);
            if ((child->state & UI_STATE_VISIBLE) && (child->state & UI_STATE_REPAINT) && child->paint)
            {
                // if (event->type != EVENT_DRAG) printf("     ev %d from %s to child %s REPAINT\n", event->type, ui->_type, child->_type);
                child->paint(child, 0, info);
            }
            child->state &= ~UI_STATE_REPAINT;
        }
        else
        {
            // if (event->type != EVENT_DRAG) printf("deny ev %d from %s to child %s\n", event->type, ui->_type, child->_type);
        }
        child = child->next;
    }
    info->x -= ui->x;
    info->y -= ui->y;
    return res;
}
int __ui_set_flag_state(ui_component_t *c, uint32_t flag, int e)
{
    if (((e ? 1 : 0) ^ (c->state & flag ? 1 : 0)) == 0)
    {
        return 0;
    }
    if (e)
        c->state |= flag;
    else
        c->state &= ~flag;
    c->state |= UI_STATE_REPAINT; // state change, auto trigger repaint
    return 1;
}

int __ui_is_event_within(ui_component_t *ui, ui_event_t *event, ui_info_t *i)
{
    return event->x >= ui->x + (i ? i->x : 0) && event->x <= ui->x + (i ? i->x : 0) + ui->w &&
           event->y >= ui->y + (i ? i->y : 0) && event->y <= ui->y + (i ? i->y : 0) + ui->h;
}

int ui_add(ui_component_t *parent, ui_component_t *child)
{
    ASSERT(child->parent == 0);
    if (parent->children)
    {
        parent->children->prev = child;
        child->next = parent->children;
        child->prev = 0;
    }
    else
    {
        child->prev = child->next = 0;
    }
    parent->children = child;
    child->parent = parent;
    return 0;
}

int ui_remove(ui_component_t *c)
{
    ASSERT(c->parent);
    if (c->children == c)
    {
        c->children = c->next;
    }
    if (c->prev)
    {
        c->prev->next = c->next;
    }
    if (c->next)
    {
        c->next->prev = c->prev;
    }
    c->parent = 0;
    return 0;
}

int ui_set_visible(ui_component_t *c, int e)
{
    if (__ui_set_flag_state(c, UI_STATE_VISIBLE, e))
    {
        ui_component_t *child = c->children;
        while (child)
        {
            ui_set_visible(child, e);
            child = child->next;
        }
        return 1;
        if (e)
            ui_repaint(c);
    }
    return 0;
}
int ui_set_enabled(ui_component_t *c, int e)
{
    if (__ui_set_flag_state(c, UI_STATE_ENABLED, e))
    {
        ui_component_t *child = c->children;
        while (child)
        {
            ui_set_enabled(child, e);
            child = child->next;
        }
        return 1;
    }
    return 0;
}
int ui_repaint(ui_component_t *c)
{
    if (c == NULL)
    {
        ASSERT(main_panel);
        c = &main_panel->comp;
    }
    c->state |= UI_STATE_REPAINT;
    ui_component_t *child = c->children;
    while (child)
    {
        ui_repaint(child);
        child = child->next;
    }
    return 0;
}

int ui_set_alignment(ui_component_t *c, ui_align_h_t hor, ui_align_v_t ver)
{
    c->state &= ~UI_STATE_ALIGNH__MASK;
    c->state |= (hor << 30);
    c->state &= ~UI_STATE_ALIGNV__MASK;
    c->state |= (ver << 28);
    c->state |= UI_STATE_REPAINT;
    return 1;
}

void ui_panel_init(ui_panel_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = __ui_paint_to_children;
    ui->comp.handle_event = __ui_event_to_children;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
}

void ui_set_main_panel(ui_panel_t *c)
{
    main_panel = c;
}

void ui_paint(void *ctx)
{
    if (main_panel->comp.paint)
    {
        ui_info_t info = {0};
        main_panel->comp.paint(&main_panel->comp, ctx, &info);
    }
}

void ui_set_click_cb(void (*click_cb_fn)(ui_event_t *e))
{
    ui_click_cb_fn = click_cb_fn;
}

void ui_handle_event(ui_event_t *e)
{
    if (e->type >= _EVENT_CB_MIN && e->type <= _EVENT_CB_MAX)
    {
        if (ui_click_cb_fn && (e->type == EVENT_CB_BUTTON_PRESS ||
                               e->type == EVENT_CB_LIST_SELECT))
        {
            ui_click_cb_fn(e);
        }
        ui_callbackable_t *b = ((ui_callbackable_t *)e->source);
        if (b->cb)
            b->cb((ui_component_t *)e->source, e, 0);
    }
    else if (main_panel->comp.handle_event)
    {
        ui_info_t info = {0};
        main_panel->comp.handle_event(&main_panel->comp, e, &info);
    }
}

void ui_post_event(ui_event_t *e)
{
    cpu_interrupt_disable();
    // put posted event first in queue
    // that way, they aren't processed until next run, i.e.
    // an event cannot enter a neverending loop by posting new events
    if (!e->_posted)
    {
        ui_event_t *ehead = event_q_head;
        event_q_head = e;
        e->next = ehead;
        e->_posted = 1;
    }
    else
    {
        printf("posting posted event %d\n", e->type);
    }
    cpu_interrupt_enable();
}

ui_event_t *ui_pop_event(void)
{
    ui_event_t *res = 0;
    cpu_interrupt_disable();
    if (event_q_head)
    {
        res = event_q_head;
        res->_posted = 0;
        event_q_head = res->next;
        res->next = 0;
    }
    cpu_interrupt_enable();
    return res;
}

ui_event_t *ui_new_event(ui_event_type_t type)
{
    const int bits = 8 * sizeof(event_pool_free_mask[0]);
    cpu_interrupt_disable();
    for (uint32_t i = 0; i < sizeof(event_pool_free_mask) / sizeof(event_pool_free_mask[0]); i++)
    {
        uint32_t free_chunk = event_pool_free_mask[i];
        if (free_chunk == 0xffffffff)
            continue;
        uint32_t end_i = UI_EVENT_POOL - i * bits;
        if (end_i > 31)
            end_i = 31;
        for (uint32_t j = 0; j < end_i; j++)
        {
            if ((free_chunk & (1 << j)) == 0)
            {
                event_pool_free_mask[i] |= (1 << j);
                event_pool[i * bits + j].source = 0;
                cpu_interrupt_enable();
                event_pool[i * bits + j].type = type;
                return &event_pool[i * bits + j];
            }
        }
    }
    cpu_interrupt_enable();
    ASSERT(0);
    return 0;
}

void ui_free_event(ui_event_t *e)
{
    if (e == 0 || e->is_static)
        return;
    cpu_interrupt_disable();
    const int bits = 8 * sizeof(event_pool_free_mask[0]);
    uint32_t ix = ((intptr_t)e - (intptr_t)event_pool) / sizeof(event_pool[0]);
    ASSERT(ix < UI_EVENT_POOL);
    if ((event_pool_free_mask[ix / bits] & (1 << (ix % bits))) == 0)
    {
        printf(
            "ui_free_event freeing free event!\n"
            "ix:%d\nis_static:%d\nsource:%08x\nsource_type:%s\ntype:%d\nvalue:%08x\n",
            ix, e->is_static, e->source, e->source->_type, e->type, e->value);
        ASSERT(0);
    }
    event_pool_free_mask[ix / bits] &= ~(1 << (ix % bits));
    cpu_interrupt_enable();
}
