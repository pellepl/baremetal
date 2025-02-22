#pragma once
#include "event.h"

enum {
    EVENT_SECOND_TICK = 1,
    EVENT_THERMO,
    EVENT_BUTTON_PRESS,
    EVENT_BUTTON_RELEASE,
    EVENT_UI_DISP_UPDATE,
    EVENT_UI_SCRL,
    EVENT_UI_PRESS,
    EVENT_UI_PRESSHOLD,
    EVENT_UI_BACK,
};

#define EVENT_HANDLER(x) \
    static const __attribute__(( used, section (".event_cb") )) event_func_t ev_hdler = (x)

extern const intptr_t __event_cb_start;
extern const intptr_t __event_cb_end;
#define EVENT_HANDLERS_START (const event_func_t *)&__event_cb_start
#define EVENT_HANDLERS_END (const event_func_t *)&__event_cb_end
