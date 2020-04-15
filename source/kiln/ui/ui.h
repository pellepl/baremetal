#ifndef _UI_H
#define _UI_H

#include "types.h"
#include "timer.h"
#include "rtc.h"

#define UI_STATE_VISIBLE        (1<<0)
#define UI_STATE_ENABLED        (1<<1)
#define UI_STATE_ACTIVE         (1<<2)
#define UI_STATE_FOCUSED        (1<<3)
#define UI_STATE_HIGHLIGHTED    (1<<4)
#define UI_STATE_PRESSED        (1<<5)
#define UI_STATE_REPAINT        (1<<6)
#define UI_STATE_TRANSLATING    (1<<7)

#ifndef UI_EVENT_POOL
#define UI_EVENT_POOL           8
#endif

typedef enum {
    EVENT_REPAINT = 0,
    EVENT_CLOSE,
    EVENT_CLOSE_APPLY,
    EVENT_APPLY,
    EVENT_CANCEL,
    EVENT_FOCUS_GRANTED,
    EVENT_FOCUS_LOST,
    EVENT_HOVER,
    EVENT_PRESS,
    EVENT_RELEASE,
    EVENT_DRAG,

    EVENT_PANIC,
    EVENT_BEEP,
    EVENT_THERMO,
    EVENT_SECOND,
    EVENT_KILN_OFF,
    EVENT_KILN_ON,

    // event_cb is only posted from ui_callbackable_t's
    _EVENT_CB_MIN,
    EVENT_CB_BUTTON_PRESS = _EVENT_CB_MIN,
    EVENT_CB_USER_INPUT,
    EVENT_CB_LIST_SELECT,
    EVENT_CB_SLIDER_ADJUST,
    EVENT_CB_SLIDER_SELECT,
    _EVENT_CB_MAX,
} ui_event_type_t;

typedef enum {
    UI_ALIGNV_TOP = 0,
    UI_ALIGNV_CENTER,
    UI_ALIGNV_BOTTOM,
} ui_align_v_t;

typedef enum {
    UI_ALIGNH_LEFT = 0,
    UI_ALIGNH_CENTER,
    UI_ALIGNH_RIGHT,
} ui_align_h_t;

#define UI_STATE_ALIGNV__MASK   (3<<28)
#define UI_STATE_ALIGNV_TOP     (UI_ALIGNV_TOP<<28)
#define UI_STATE_ALIGNV_CENTER  (UI_ALIGNV_CENTER<<28)
#define UI_STATE_ALIGNV_BOTTOM  (UI_ALIGNV_BOTTOM<<28)
#define UI_STATE_ALIGNH__MASK   (3<<30)
#define UI_STATE_ALIGNH_LEFT    (UI_ALIGNH_LEFT<<30)
#define UI_STATE_ALIGNH_CENTER  (UI_ALIGNH_CENTER<<30)
#define UI_STATE_ALIGNH_RIGHT   (UI_ALIGNH_RIGHT<<30)

struct ui_component;

typedef struct ui_event {
    ui_event_type_t type;
    union {
        struct {
            uint16_t x,y;
        };
        struct {
            uint16_t freq,time;
        } beep;
        uint32_t time, value;
    };
    struct ui_component *source;
    void *user;
    int is_static;
    struct ui_event *next;
    int _posted;
} ui_event_t;

typedef struct {
    uint16_t x;
    uint16_t y;
} ui_info_t;

typedef void (*paint_fn_t)(struct ui_component *ui, void *ctx, ui_info_t *info);
typedef int (*event_fn_t)(struct ui_component *ui, ui_event_t *event, ui_info_t *info);

typedef struct ui_component {
    union {
        uint32_t state;
        tick_t ___align;
    };
    uint16_t x,y,w,h;
    struct ui_component *next;
    struct ui_component *prev;
    struct ui_component *children;
    struct ui_component *parent;
    void *user;
    paint_fn_t paint;
    event_fn_t handle_event;
    const char *_type;
} ui_component_t;

typedef struct {
    ui_component_t comp;
} ui_panel_t;

typedef struct {
    ui_component_t comp;
    event_fn_t cb;
    void *user;
} ui_callbackable_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    const char *text;
    uint8_t font;
    uint32_t colfg;
    uint32_t colbg1;
    uint32_t colbg2;
} ui_button_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    int min, max, val, _oval;
    uint8_t hori;
    uint32_t colfg;
    uint32_t colbg1;
    uint32_t colbg2;
    const char *text;
    uint8_t font;
    uint16_t knob_w, knob_h;
    uint16_t track_w;
} ui_slider_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    uint8_t icon;
    uint32_t colfg;
} ui_ibutton_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    uint32_t items;
    void (*get_item_fn)(uint32_t ix, const char **str_left, const char **str_center, const char **str_right);
    void (*paint_item_fn)(uint32_t ix, int16_t x, int16_t y, uint16_t w, uint16_t h);
    uint8_t font;
    uint32_t colfg;
    uint32_t colbg1;
    uint32_t colbg2;
    uint16_t item_padding;

    uint32_t sel_ix;
    int32_t vp_y;
    int32_t anchor_vp_y;
    tick_t anchor_t;
    uint16_t anchor_x, anchor_y;
    int32_t spin_dvp_y;
    ui_button_t b_up;
    ui_button_t b_down;
} ui_list_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    char text[32];
    uint8_t max_font;
    uint8_t font;
    uint32_t colfg, colbg;
} ui_label_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    char text[32];
    ui_label_t label;
    ui_button_t buttons[12];
} ui_keypad_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    ui_label_t labels[4];
    ui_button_t buttons[2];
} ui_confirm_t;

typedef struct {
    union {
        ui_component_t comp;
        ui_callbackable_t call;
    };
    uint32_t colfg, colbg, colgrid;
    int16_t user_min, user_max;
    int16_t _value_min, _value_max;
    int16_t *_values;
    uint16_t _value_count, _value_size, _value_ix, _ix_min, _ix_max;
    rtc_datetime_t _last_add_dt;
} ui_graph_t;

void ui_panel_init(ui_panel_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

void ui_button_init(ui_button_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                    const char *text, event_fn_t callback, void *user,
                    uint8_t font, uint32_t colfg, uint32_t colbg1, uint32_t colbg2);
int button_event(ui_component_t *ui, ui_event_t *event, ui_info_t *i);

void ui_ibutton_init(ui_ibutton_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                    uint8_t icon, event_fn_t callback, void *user,
                    uint32_t colfg);

void ui_list_init(ui_list_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                  uint32_t items,
                  void (*get_item_fn)(uint32_t ix, const char **str_left, const char **str_center, const char **str_right),
                  void (*paint_item_fn)(uint32_t ix, int16_t x, int16_t y, uint16_t w, uint16_t h),
                  event_fn_t callback, void *user,
                  uint8_t font, uint32_t colfg, uint32_t colbg1, uint32_t colbg2);
                  
void ui_label_init(ui_label_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    const char *text, uint8_t font, uint32_t colfg, uint32_t colbg);
void ui_label_set_text(ui_label_t *ui, const char *text);

void ui_keypad_init(ui_keypad_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    event_fn_t callback, void *user);
void ui_keypad_set_value(ui_keypad_t *ui, int32_t val);

void ui_confirm_init(ui_confirm_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    event_fn_t callback, void *user, 
                    const char *hdr, const char *txt1,const char *txt2,const char *txt3);

void ui_graph_init(ui_graph_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    event_fn_t callback, void *user, 
                    int16_t *buf, uint16_t buf_size,
                    uint32_t colfg, uint32_t colbg);
void ui_graph_add(ui_graph_t *c, int16_t value);
void ui_graph_clear(ui_graph_t *c);
int16_t ui_graph_get_past(ui_graph_t *c, uint16_t indices_in_past);

void ui_slider_init(ui_slider_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                    event_fn_t cb, void *user, 
                    int min, int max, int initial, int hori_else_veri,
                    const char *text, int font,
                    uint32_t colfg, uint32_t colbg1, uint32_t colbg2);
                    
int ui_add(ui_component_t *parent, ui_component_t *child);
int ui_remove(ui_component_t *c);
int ui_set_visible(ui_component_t *c, int e);
int ui_set_enabled(ui_component_t *c, int e);
int ui_set_alignment(ui_component_t *c, ui_align_h_t hor, ui_align_v_t ver);
int ui_repaint(ui_component_t *c);

void ui_set_main_panel(ui_panel_t *c);
void ui_paint(void *ctx);
void ui_handle_event(ui_event_t *e);

void ui_post_event(ui_event_t *e);
ui_event_t *ui_pop_event(void);
ui_event_t *ui_new_event(ui_event_type_t t);
void ui_free_event(ui_event_t *e);

void ui_set_click_cb(void (*click_cb_fn)(ui_event_t *e));

void __ui_paint_to_children(ui_component_t *ui, void *ctx, ui_info_t *info);
int __ui_event_to_children(ui_component_t *ui, ui_event_t *event, ui_info_t *info);
int __ui_is_event_within(ui_component_t *ui, ui_event_t *event, ui_info_t *info);
int __ui_set_flag_state(ui_component_t *c, uint32_t flag, int e);
uint32_t __ui_colfade(uint32_t a, uint32_t b, uint8_t x);

#define COL_BG                  0xff01010f
#define COL_TITLE_FG            0xff222222
#define COL_TITLE_BG            0xffcccccc
#define COL_INFO_PRIO1          0xffffee88
#define COL_INFO_PRIO2          0xffdddddd
#define COL_BUTTON_FG           0xffffffff
#define COL_BUTTON_BG1          0xff6666aa
#define COL_BUTTON_BG2          0xff111155
#define COL_BUTTON_COMMIT_FG    COL_BUTTON_FG
#define COL_BUTTON_COMMIT_BG1   0xff66aa66
#define COL_BUTTON_COMMIT_BG2   0xff115511
#define COL_BUTTON_CANCEL_FG    COL_BUTTON_FG
#define COL_BUTTON_CANCEL_BG1   0xffaa6666
#define COL_BUTTON_CANCEL_BG2   0xff551111
#define COL_SLIDER_FG           COL_BUTTON_FG
#define COL_SLIDER_BG1          COL_BUTTON_BG1
#define COL_SLIDER_BG2          COL_BUTTON_BG2
#define COL_LIST_FG             0xffffffff
#define COL_LIST_BG1            0xff333399
#define COL_LIST_BG2            0xff111155
#define COL_DISABLED_FG         0xff444444
#define COL_DISABLED_BG1        0xff999999
#define COL_DISABLED_BG2        0xff666666
#define COL_INPUT_BG            0xffffffff
#define COL_INPUT_FG            0xff000000
#define COL_INPUT_ERROR_FG      0xffff0000

#endif
