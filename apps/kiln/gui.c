#include <stddef.h>
#include <stdbool.h>
#include "board.h"
#include "kiln.h"
#include "ui/ui.h"
#include "fonts/fonts.h"
#include "gpio_driver.h"
#include "timer.h"
#include "settings.h"
#include "nvmtnv.h"
#include "minio.h"
#include "assert.h"
#include "flash_driver.h"
#include "disp.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_tim.h"
#include "cli.h"
#include "sys.h"

#define SLEEP_TIMEOUT_S 60
#define VIEW_HISTORY_LEN    5
#define BEEP_TIM_PRESC      1000
#define TITLE_H     40

static bool gui_sleeping = false;

static ui_panel_t main_panel;
static ui_button_t back_button;
static ui_label_t title;
static ui_label_t title_stat1;
static ui_label_t title_stat2;
static ui_keypad_t keypad;
static ui_list_t list;
static ui_confirm_t confirm;
static ui_ibutton_t ibuttons[4];
static ui_label_t temp_label1, temp_label2;
static ui_graph_t temp_graph;

static ui_button_t kiln_temp_buttons[2];
static ui_label_t kiln_temp_labels[2];
static ui_label_t kiln_info_labels[2];
static ui_label_t kiln_pow_label;
static ui_slider_t kiln_pow_slider;
static ui_button_t kiln_stop_button;

static int16_t t_buf[SETTINGS_TEMP_GRAPH_LEN_MAX];

static timer_t beep_timer;
static int32_t gui_temp_graph_last_time;

static int32_t gui_kiln_temp = 20;
static int32_t gui_kiln_power = 0;

static bool gui_attention = false;

#define _str(x) __str(x)
#define __str(x) #x

typedef struct {
    enum {
    VIEW_UNDEF = 0,
    VIEW_MAIN,
    VIEW_SETTINGS,
    VIEW_SETTING_INPUT,
    VIEW_TEMP,
    VIEW_KILN,
    VIEW_PROGRAM,
    VIEW_INFO
    } type;
    uint32_t instance;
    uint32_t index;
    void (*view_setup)(uint32_t instance, uint32_t index, int recall);
    void (*view_teardown)(uint32_t instance, uint32_t index, int remember);
    event_fn_t event_fn;
} view_t;

static struct {
    const uint16_t *freq;
    const uint16_t *time;
    uint8_t len;
} sequencer;

static void gui_view_open(view_t *view, uint32_t instance, uint32_t index);
static void gui_view_close(void);
static view_t *gui_view_get(void);
static void update_title_status(void);

static void gui_hide_all(void) {
    ui_set_visible(&back_button.comp, 0);
    ui_set_visible(&title.comp, 0);
    ui_set_visible(&temp_label1.comp, 0);
    ui_set_visible(&temp_label2.comp, 0);
    ui_set_visible(&temp_graph.comp, 0);
    ui_set_visible(&keypad.comp, 0);
    ui_set_visible(&list.comp, 0);
    ui_set_visible(&confirm.comp, 0);
    for (uint32_t i = 0; i < sizeof(ibuttons)/sizeof(ibuttons[0]); i++) {
        ui_set_visible(&ibuttons[i].comp, 0);
    }
    for (uint32_t i = 0; i < 2; i++) {
        ui_set_visible(&kiln_temp_buttons[i].comp, 0);
        ui_set_visible(&kiln_temp_labels[i].comp, 0);
        ui_set_visible(&kiln_info_labels[i].comp, 0);
    }
    ui_set_visible(&kiln_pow_slider.comp, 0);
    ui_set_visible(&kiln_pow_label.comp, 0);
    ui_set_visible(&kiln_stop_button.comp, 0);
}

static void sequencer_next(void) {
    if (sequencer.len) {
        sequencer.len--;
        gui_beep(*sequencer.freq++, TIMER_MS_TO_TICKS(*sequencer.time++));
    }
}

static void beep_timer_stop_fn(timer_t *t) {
    gui_silence();
    sequencer_next();
}

void gui_silence(void) {
    LL_TIM_DisableARRPreload(TIM3);
    LL_TIM_SetCounter(TIM3, 0);
    LL_TIM_SetAutoReload(TIM3, 0);
    LL_TIM_EnableARRPreload(TIM3);
}
void gui_beep(uint16_t hz, tick_t time) {
    timer_stop(&beep_timer);
    uint32_t period = (cpu_core_clock_freq() / BEEP_TIM_PRESC) / hz;
    LL_TIM_DisableARRPreload(TIM3);
    LL_TIM_SetAutoReload(TIM3, period);
    LL_TIM_SetCounter(TIM3, 0xffff);
    LL_TIM_OC_SetCompareCH3(TIM3, period/2);
    LL_TIM_EnableARRPreload(TIM3);
    timer_start(&beep_timer, beep_timer_stop_fn, time, TIMER_ONESHOT);
}

void gui_melody(const uint16_t *hz, const uint16_t *time, uint8_t len) {
    sequencer.freq = hz;
    sequencer.time = time;
    sequencer.len = len;
    sequencer_next();
}

static void gui_click_cb(ui_event_t *e) {
    if (settings_get_val_for_id(SETTING_AUDIO_KEY_CLICK)) gui_beep(40,400);
}

////////////////////////////////////////////////////////////////// TEMP VIEW

static int temp_graph_update(int16_t temp) {
    ui_graph_add(&temp_graph, temp);
    if (temp_graph.comp.state & UI_STATE_VISIBLE) {
        char str[32] = {0};
        snprintf(str, sizeof(str)-1, "%d`C", temp);
        ui_label_set_text(&temp_label1, str);
        const int32_t delta_time = 60;
        int32_t graph_update_dt = (int32_t)settings_get_val_for_id(SETTING_TEMP_GRAPH_UPDATE);
        int32_t past_temp = 0;
        uint8_t spls = (delta_time / graph_update_dt);
        for (int i = 2; i <= spls; i++) {
            past_temp += (ui_graph_get_past(&temp_graph, i));
        }
        past_temp = (past_temp*100) / (spls-1);

        int32_t delta_temp = temp*100 - past_temp;
        int neg  = delta_temp < 0;
        if (neg) delta_temp = -delta_temp;
        int32_t t_i = delta_temp; /// delta_time;
        uint32_t t_f = t_i % 100;
        uint32_t t_int = t_i / 100;
        uint32_t t_frac = t_f / 10;
        if (t_int == 0 && t_frac < 5) {
            str[0] = 0;
        } else {
            snprintf(str, sizeof(str)-1, "%c%d.%01d`C/%s", neg ? '-':'+', t_int, t_frac, "m");
        }
        ui_label_set_text(&temp_label2, str);
        temp_label2.colfg = neg ? 0xff88ffff : 0xffffff88;
        return 1;
    }
    return 0;
}
static void view_temp_setup(uint32_t instance, uint32_t index, int recall) {
    ui_set_visible(&back_button.comp, 1);
    ui_set_visible(&title.comp, 1);
    ui_label_set_text(&title, "TEMPERATURE");
    ui_set_visible(&temp_label1.comp, 1);
    ui_set_visible(&temp_label2.comp, 1);
    ui_set_visible(&temp_graph.comp, 1);
}
static void view_temp_teardown(uint32_t instance, uint32_t index, int remember) {
    ui_set_visible(&temp_label1.comp, 0);
    ui_set_visible(&temp_label2.comp, 0);
    ui_set_visible(&temp_graph.comp, 0);
    ui_set_visible(&back_button.comp, 0);
    ui_set_visible(&title.comp, 0);
}
static int view_temp_event_fn(ui_component_t *ui, ui_event_t *event, ui_info_t *info) {
    gui_view_close();
    return 0;
}
static view_t view_temp = {
    .type = VIEW_TEMP,
    .view_setup = view_temp_setup,
    .view_teardown = view_temp_teardown,
    .event_fn = view_temp_event_fn
};

//////////////////////////////////////////////////////////// SETTING_INPUT VIEW

static void view_setting_input_setup(uint32_t instance, uint32_t index, int recall) {
    ui_set_visible(&back_button.comp, 1);
    ui_set_visible(&title.comp, 1);
    const user_setting_t *setting = settings_get(index);
    if (setting->unit == UNIT_RESETTABLE) {
        ui_label_set_text(&title, "CONFIRM");
        if (setting->id == SETTING_TOUCH_CALIB) {
            ui_label_set_text(&confirm.labels[0], "TOUCH RESET");
            ui_label_set_text(&confirm.labels[1], "After reboot you");
            ui_label_set_text(&confirm.labels[2], "must recalibrate");
            ui_label_set_text(&confirm.labels[3], "the touch. Sure?");
        } else if (setting->id == SETTING_FACTORY) {
            ui_label_set_text(&confirm.labels[0], "FACTORY RESET");
            ui_label_set_text(&confirm.labels[1], "All your settings");
            ui_label_set_text(&confirm.labels[2], "will be removed.");
            ui_label_set_text(&confirm.labels[3], "Sure?");
        } else if (setting->id == SETTING_UI_RESET) {
            ui_label_set_text(&confirm.labels[0], "PROCESSOR RESET");
            ui_label_set_text(&confirm.labels[1], "");
            ui_label_set_text(&confirm.labels[2], "Sure?");
            ui_label_set_text(&confirm.labels[3], "");
        }
        ui_set_visible(&confirm.comp, 1);
    } else if (setting->unit == UNIT_BOOLEAN) {
        settings_set_val(index, (void *)((uint32_t)settings_get_val(index) ? 0 : 1));
        gui_view_close();
    } else {
        char title_str[32];
        char val_str[16];
        settings_get_val_as_string(index, val_str, sizeof(val_str), 0);
        const char *u_str = settings_unit_str(index);
        if (u_str[0]) {
            snprintf(title_str, sizeof(title_str), "%s (%s)", setting->ui_name, u_str);
        } else {
            snprintf(title_str, sizeof(title_str), "%s", setting->ui_name);
        }
        keypad.text[0] = 0;
        ui_label_set_text(&title, title_str);
        keypad.label.colfg = COL_INPUT_FG;
        ui_label_set_text(&keypad.label, val_str);
        ui_set_visible(&keypad.comp, 1);
    }
}
static void view_setting_input_teardown(uint32_t instance, uint32_t index, int remember) {
    ui_set_visible(&list.comp, 0);
    ui_set_visible(&back_button.comp, 0);
    const user_setting_t *setting = settings_get(index);
    if (setting->unit == UNIT_RESETTABLE) {
        ui_set_visible(&confirm.comp, 0);
    } else {
        ui_set_visible(&keypad.comp, 0);
    }
}
static int view_setting_input_event_fn(ui_component_t *ui, ui_event_t *event, ui_info_t *info) {
    view_t *view = gui_view_get();
    const user_setting_t *setting = settings_get(view->index);
    if (setting->unit == UNIT_RESETTABLE) {
        if (setting->id < _SETTINGS_VOLATILE) {
            if (event->value) {
                nvmtnv_delete(setting->id);
            }
        } else {
            if (event->value) {
                settings_set_val(view->index, (void *)1);
            }
        }
        gui_view_close();
    } else {
        // some value was entered
        printf("set ix %d, id %d, %s = %d\n", view->index, setting->id, setting->ui_name, event->value);
        if (event->value < (uint32_t)setting->min_value) {
            printf("value for %s too low, %d < %d\n", setting->ui_name, event->value, (uint32_t)setting->min_value);
            keypad.label.colfg = COL_INPUT_ERROR_FG;
            ui_keypad_set_value(&keypad, (int32_t)setting->min_value);
            gui_beep(90, TIMER_MS_TO_TICKS(100));
        } else if (event->value > (uint32_t)setting->max_value) {
            printf("value for %s too high, %d > %d\n", setting->ui_name, event->value, (uint32_t)setting->max_value);
            keypad.label.colfg = COL_INPUT_ERROR_FG;
            ui_keypad_set_value(&keypad, (int32_t)setting->max_value);
            gui_beep(90, TIMER_MS_TO_TICKS(100));
        } else {
            int res = settings_set_val(view->index, (void *)event->value);
            if (res < 0) printf("error saving %d\n", res);
            gui_view_close();
        }
    }
    return 0;
}
static view_t view_setting_input = {
    .type = VIEW_SETTING_INPUT,
    .view_setup = view_setting_input_setup,
    .view_teardown = view_setting_input_teardown,
    .event_fn = view_setting_input_event_fn
};

///////////////////////////////////////////////////////////////// SETTINGS VIEW

static void view_setting_list_get_item(uint32_t ix, const char **l, const char **c, const char **r) {
    static char list_settings_str_r[32];
    *l = settings_get(ix)->ui_name;
    settings_get_val_as_string(ix, list_settings_str_r, sizeof(list_settings_str_r), 1);
    *r = list_settings_str_r;
    *c = 0;
}
static void view_settings_setup(uint32_t instance, uint32_t index, int recall) {
    ui_set_visible(&back_button.comp, 1);
    ui_set_visible(&title.comp, 1);
    ui_label_set_text(&title, "SETTINGS");
    list.items = settings_count();
    list.get_item_fn = view_setting_list_get_item;
    if (!recall) list.vp_y = 0;
    ui_set_visible(&list.comp, 1);
}
static void view_settings_teardown(uint32_t instance, uint32_t index, int remember) {
    ui_set_visible(&list.comp, 0);
    ui_set_visible(&back_button.comp, 0);
    ui_set_visible(&title.comp, 0);
}
static int view_settings_event_fn(ui_component_t *ui, ui_event_t *event, ui_info_t *info) {
    if (event->source != &list.comp) return 0;
    uint32_t ix = (uint32_t)event->user;
    gui_view_open(&view_setting_input, 0, ix);
    return 0;
}
static view_t view_settings = {
    .type = VIEW_SETTINGS,
    .view_setup = view_settings_setup,
    .view_teardown = view_settings_teardown,
    .event_fn = view_settings_event_fn
};

///////////////////////////////////////////////////////////////// KILN VIEW

static void view_kiln_update_gui(int pow) {
    char str[32] = {0};
    snprintf(str, sizeof(str)-1, "%d%%", pow*10);
    ui_label_set_text(&kiln_pow_label, pow == 0 ? "OFF" : str);
    snprintf(str, sizeof(str)-1, "%d", gui_kiln_temp);
    ui_label_set_text(&kiln_temp_labels[1], str);
}
static void view_kiln_setup(uint32_t instance, uint32_t index, int recall) {
    ui_set_visible(&back_button.comp, 1);
    ui_set_visible(&title.comp, 1);
    for (uint32_t i = 0; i < 2; i++) {
        ui_set_visible(&kiln_temp_buttons[i].comp, 1);
        ui_set_visible(&kiln_temp_labels[i].comp, 1);
        ui_set_visible(&kiln_info_labels[i].comp, 1);
    }
    ui_set_visible(&kiln_pow_slider.comp, 1);
    ui_set_visible(&kiln_pow_label.comp, 1);
    ui_set_visible(&kiln_stop_button.comp, 1);
    ui_label_set_text(&title, "KILN");
}
static void view_kiln_teardown(uint32_t instance, uint32_t index, int remember) {
    ui_set_visible(&back_button.comp, 0);
    ui_set_visible(&title.comp, 0);
    for (uint32_t i = 0; i < 2; i++) {
        ui_set_visible(&kiln_temp_buttons[i].comp, 0);
        ui_set_visible(&kiln_temp_labels[i].comp, 0);
        ui_set_visible(&kiln_info_labels[i].comp, 0);
    }
    ui_set_visible(&kiln_pow_label.comp, 0);
    ui_set_visible(&kiln_pow_slider.comp, 0);
    ui_set_visible(&kiln_stop_button.comp, 0);
}
static int view_kiln_event_fn(ui_component_t *ui, ui_event_t *event, ui_info_t *info) {
    if (event->source == &kiln_temp_labels[0].comp) {
        gui_view_open(&view_temp, 0,0);
    }
    else if (event->source == &kiln_temp_labels[1].comp) {
        gui_view_open(&view_setting_input, 0, SETTING_UI_KILN_TEMP);
    }
    else if (event->source == &kiln_temp_buttons[0].comp || event->source == &kiln_temp_buttons[1].comp) {
        printf("ui event kiln temp button\n");
        uint8_t d = 1;
        if (event->value > 30) d = 25;
        else if (event->value > 20) d = 10;
        else if (event->value >= 10) d = 5;
        if (event->source == &kiln_temp_buttons[0].comp)
            gui_kiln_temp -= d;
        else
            gui_kiln_temp += d;
        if (gui_kiln_temp > (int32_t)settings_get_val_for_id(SETTING_KILN_MAX_TEMP)) 
            gui_kiln_temp = (int32_t)settings_get_val_for_id(SETTING_KILN_MAX_TEMP);
        else if (gui_kiln_temp < 0)
            gui_kiln_temp = 0;
        settings_set_val(SETTING_UI_KILN_TEMP, (void*)gui_kiln_temp);
        view_kiln_update_gui(gui_kiln_power);
    } else if (event->source == &kiln_pow_slider.comp) {
        printf("ui event kiln pow slider type %d\n", event->type);
        if (event->type == EVENT_CB_SLIDER_ADJUST) {
            kiln_pow_label.colfg = COL_INPUT_BG;
        } else {
            kiln_pow_label.colfg = COL_INFO_PRIO1;
            gui_kiln_power = event->value;
            settings_set_val(SETTING_UI_KILN_POWER, (void*)gui_kiln_power);
        }
        view_kiln_update_gui(event->value);
    } else if (event->source == &kiln_stop_button.comp) {
        printf("ui event kiln stop\n");
        gui_kiln_power = 0;
        settings_set_val(SETTING_UI_KILN_POWER, (void*)gui_kiln_power);
        kiln_pow_slider.val = 0;
        ui_repaint(&kiln_pow_slider.comp);
        view_kiln_update_gui(gui_kiln_power);
    }
    return 0;
}
static view_t view_kiln = {
    .type = VIEW_KILN,
    .view_setup = view_kiln_setup,
    .view_teardown = view_kiln_teardown,
    .event_fn = view_kiln_event_fn
};

///////////////////////////////////////////////////////////////// MAIN VIEW

static void view_main_setup(uint32_t instance, uint32_t index, int recall) {
    disp_clear(COL_BG);
    ui_set_visible(&back_button.comp, 0);
    ui_set_visible(&title.comp, 0);
    for (uint32_t i = 0; i < sizeof(ibuttons)/sizeof(ibuttons[0]); i++) {
        ui_set_visible(&ibuttons[i].comp, 1);
    }
    title_stat1.colbg = COL_BG;title_stat1.colfg = COL_TITLE_BG;
    title_stat2.colbg = COL_BG;title_stat2.colfg = COL_TITLE_BG;
    ui_repaint(&title_stat1.comp);ui_repaint(&title_stat2.comp);
}
static void view_main_teardown(uint32_t instance, uint32_t index, int remember) {
    for (uint32_t i = 0; i < sizeof(ibuttons)/sizeof(ibuttons[0]); i++) {
        ui_set_visible(&ibuttons[i].comp, 0);
    }
    title_stat1.colbg = COL_TITLE_BG;title_stat1.colfg = COL_TITLE_FG;
    title_stat2.colbg = COL_TITLE_BG;title_stat2.colfg = COL_TITLE_FG;
    ui_repaint(&title_stat1.comp);ui_repaint(&title_stat2.comp);
}
static int view_main_event_fn(ui_component_t *ui, ui_event_t *event, ui_info_t *info) {
    if (event->source == &ibuttons[0].comp) {
        gui_view_open(&view_temp, 0, 0);
    }
    // TODO PROGRAM
    else if (event->source == &ibuttons[2].comp) {
        gui_view_open(&view_settings, 0, 0);
    }
    else if (event->source == &ibuttons[3].comp) {
        gui_view_open(&view_kiln, 0, 0);
    }
    return 0;
}
static view_t view_main = {
    .type = VIEW_MAIN,
    .view_setup = view_main_setup,
    .view_teardown = view_main_teardown,
    .event_fn = view_main_event_fn
};

///////////////////////////////////////////////////////////////// INFO VIEW

extern uint32_t _estack[];
extern uint32_t __bss_end__[];

static void view_info_setup(uint32_t instance, uint32_t index, int recall) {
    disp_clear(COL_BG);
    ui_set_visible(&back_button.comp, 1);
    ui_set_visible(&title.comp, 1);
    ui_label_set_text(&title, "INFO");
    int16_t y = TITLE_H+4;
    uint8_t f = 2;
    uint8_t fh = font_get(f)->height + 4;
    disp_draw_stringf("%s %s on %s", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
        _str(BUILD_INFO_GIT_COMMIT), _str(BUILD_INFO_GIT_TAG), _str(BUILD_INFO_GIT_BRANCH));
    y += fh;
    disp_draw_stringf("%s/%s/%s", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
        _str(BUILD_INFO_TARGET_ARCH), _str(BUILD_INFO_TARGET_FAMILY), _str(BUILD_INFO_TARGET_PROC));
    y += fh;
    disp_draw_stringf("%s@%s %s-%s %s", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
        _str(BUILD_INFO_TARGET_BOARD),
       _str(BUILD_INFO_HOST_WHO), _str(BUILD_INFO_HOST_NAME), 
       _str(BUILD_INFO_HOST_WHEN_DATE), _str(BUILD_INFO_HOST_WHEN_TIME));
    y += fh;
    y += fh;
    disp_draw_stringf("stack used:%d/%d bytes", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
        __get_used_stack(), (uint32_t)_estack - (uint32_t)__bss_end__);
    y += fh;
    {
        uint32_t sect, scount, fsize, used;
        fsize = used = 0;
        flash_get_sectors_for_type(FLASH_TYPE_CODE_BANK0, &sect, &scount);
        for (uint32_t s = sect; s < sect+scount; s++) {
            uint32_t size = flash_get_sector_size(s);
            fsize += size;
            uint32_t offs = 0;
            int stop = 0;
            while (!stop && offs < size) {
                uint8_t buf[256];
                uint32_t rem = (size - offs);
                int res = flash_read(s, offs, buf, rem > sizeof(buf) ? sizeof(buf) : rem);
                if (res <= 0) break;
                for (int i = 0; i < res; i++) {
                    if (buf[i] != 0xff) {
                        used += size;
                        stop = 1;
                        break;
                    }
                }
                offs += res;
            }
            disp_draw_stringf("flash used:%d/%d bytes", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, COL_BG,
               used, fsize);
        }
    }

    y += fh;
    y += fh;
#   define portstr(x) "ABCDEFG"[x>>4], x&15
    disp_draw_stringf("dbg rx:%c%d tx:%c%d", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
       portstr(PIN_UART_RX), portstr(PIN_UART_TX));
    y += fh;
    disp_draw_stringf("ipc rx:%c%d tx:%c%d", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
       portstr(PIN_UART_RX_IPC), portstr(PIN_UART_TX_IPC));
    y += fh;
    disp_draw_stringf("thermo clk:%c%d cs:%c%d miso:%c%d", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
       portstr(PIN_THERMOC_CLK), portstr(PIN_THERMOC_CS), portstr(PIN_THERMOC_MISO));
    y += fh;
    disp_draw_stringf("thermo sample period:%d ms", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
       THERMOCOUPLE_SAMPLE_PERIOD_MS);
    y += fh;
    disp_draw_stringf("element ctrl 1:%c%d 2:%c%d 3:%c%d 4:%c%d", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
       portstr(PIN_ELEMENT_1), portstr(PIN_ELEMENT_2), portstr(PIN_ELEMENT_3), portstr(PIN_ELEMENT_4));
    y += fh;
    disp_draw_stringf("speaker pwm:%c%d", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
       portstr(PIN_SPEAKER_PWM));
    y += fh;
    y += fh;
    {
        uint32_t bootcnt,unexp;
        reset_reason_t rr = sys_reset_reason(&bootcnt, &unexp);
        disp_draw_stringf("reset %s%s bootcnt:%d", 0, y, f, DISP_STR_ALIGN_LEFT_TOP, DISP_STR_ALIGN_LEFT_TOP, 0xffffffff, 0,
        (const char *[6]){"LOWPWR","WWDG","IWDG","SW","PORPDR","PIN"}[rr], unexp?" UNEXPECTED":"", bootcnt);
    }
}
static void view_info_teardown(uint32_t instance, uint32_t index, int remember) {
    ui_set_visible(&back_button.comp, 0);
    ui_set_visible(&title.comp, 0);
}
static view_t view_info = {
    .type = VIEW_INFO,
    .view_setup = view_info_setup,
    .view_teardown = view_info_teardown,
    .event_fn = 0
};

///////////////////////////////////////////////////////////////////////// COMMON

static int8_t view_history_ix = -1;
static view_t *view_history[VIEW_HISTORY_LEN];

static int gui_event_cb(ui_component_t *ui, ui_event_t *event, ui_info_t *info) {
    if (ui == &back_button.comp) {
        gui_view_close();
        return 0;
    } else {
        return view_history[view_history_ix]->event_fn(ui, event, info);
    }
}

static view_t *gui_view_get(void) {
    return view_history[view_history_ix];
}

static void gui_view_open(view_t *view, uint32_t instance, uint32_t index) {
    if (view_history_ix >= 0) {
        view_history[view_history_ix]->view_teardown(
            view_history[view_history_ix]->instance, view_history[view_history_ix]->index, 1);
    }
    view->instance = instance;
    view->index = index;
    view_history_ix++;
    view_history[view_history_ix] = view;
    view->view_setup(instance, index, 0);
}

static void gui_view_close(void) {
    if (view_history_ix == 0) return;
    view_history[view_history_ix]->view_teardown(
        view_history[view_history_ix]->instance, view_history[view_history_ix]->index, 0);
    view_history_ix--;
    view_history[view_history_ix]->view_setup(
        view_history[view_history_ix]->instance, view_history[view_history_ix]->index, 1);
}


static control_state_t gui_status_ctrl;
static int gui_status_temp;
static int gui_status_target_temp;
static int gui_status_power;

void gui_status_report_ctrl(control_state_t ctrl, int temp, int target_temp, int power) {
    gui_status_ctrl = ctrl;
    gui_status_target_temp = target_temp;
    gui_status_temp = temp;
    gui_status_power = power;
}

#define ICON_SMALL_NONE             0x20
#define ICON_SMALL_TEMP_COOL        0x21
#define ICON_SMALL_TEMP_HOT1        0x22
#define ICON_SMALL_TEMP_HOT2        0x23
#define ICON_SMALL_TEMP_HOT3        0x24
#define ICON_SMALL_SIMULATION       0x25
#define ICON_SMALL_DISK_IO          0x26
#define ICON_SMALL_WARNING          0x27
#define ICON_SMALL_WIFI_NONE        0x28
#define ICON_SMALL_WIFI_1           0x29
#define ICON_SMALL_WIFI_2           0x2a
#define ICON_SMALL_WIFI_3           0x2b
#define ICON_SMALL_TEMP_EQ          0x2c
#define ICON_SMALL_TEMP_UP          0x2d
#define ICON_SMALL_TEMP_DOWN        0x2e
#define ICON_SMALL_POW_NONE         0x30
#define ICON_SMALL_POW_1            0x31
#define ICON_SMALL_POW_2            0x32
#define ICON_SMALL_POW_3            0x33
#define ICON_SMALL_POW_4            0x34
#define ICON_SMALL_POW_5            0x35
#define ICON_SMALL_PROGRAM          0x36
#define ICON_SMALL_MANUAL           0x37

static void update_title_status(void) {
    static int bl = 0;
    bl++;
    char temp = ICON_SMALL_TEMP_COOL;
    if (gui_status_temp > 500) temp = ICON_SMALL_TEMP_HOT3;
    else if (gui_status_temp > 250) temp = ICON_SMALL_TEMP_HOT2;
    else if (gui_status_temp > 50) temp = ICON_SMALL_TEMP_HOT1;
    if (gui_status_ctrl && (bl & 1)) {
        if (gui_status_target_temp/8 > gui_status_temp/8)
            temp = ICON_SMALL_TEMP_UP;
        else if (gui_status_target_temp/8 < gui_status_temp/8)
            temp = ICON_SMALL_TEMP_DOWN;
        else
            temp = ICON_SMALL_TEMP_EQ;
    }
    char power = ICON_SMALL_NONE; //ICON_SMALL_POW_NONE;
    if (gui_status_ctrl) {
        if (gui_status_power >= 100) power = ICON_SMALL_POW_5;
        else if (gui_status_power >= 75) power = ICON_SMALL_POW_4;
        else if (gui_status_power >= 50) power = ICON_SMALL_POW_3;
        else if (gui_status_power >= 25) power = ICON_SMALL_POW_2;
        else power = ICON_SMALL_POW_1;
    }
    char ctrl = ICON_SMALL_NONE;
    if (gui_status_ctrl) {
        ctrl = gui_status_ctrl == CONTROL_MANUAL ? 
            ICON_SMALL_MANUAL : ICON_SMALL_PROGRAM;
    }
    char info = ICON_SMALL_NONE;
    if (settings_get_val_for_id(SETTING_KILN_SIMULATION)) {
        info = ICON_SMALL_SIMULATION;
    }
    if ((bl & 1) && gui_attention) {
        info = ICON_SMALL_WARNING;
    }
    char stat5 = ICON_SMALL_NONE;
    char stat6 = ICON_SMALL_NONE;
    title_stat1.text[0] = temp;
    title_stat1.text[1] = power;
    title_stat1.text[2] = ctrl;
    title_stat2.text[0] = info;
    title_stat2.text[1] = stat5;
    title_stat2.text[2] = stat6;
    ui_repaint(&title_stat1.comp);ui_repaint(&title_stat2.comp);
    ui_post_event(ui_new_event(EVENT_REPAINT));
}

// SETTING CALLBACKS

static void gui_setting_change_cb(setting_t id, void *val) {
        printf("settings change %d %d\n", id, val);
    switch (id) {
        case SETTING_TEMP_GRAPH_UPDATE:
        ui_graph_clear(&temp_graph);
        ui_label_set_text(&temp_label1, "");
        ui_label_set_text(&temp_label2, "");
        break;
        case SETTING_TEMP_GRAPH_LENGTH:
        ui_graph_clear(&temp_graph);
        ui_label_set_text(&temp_label1, "");
        ui_label_set_text(&temp_label2, "");
        temp_graph._value_size = (uint32_t)val;
        break;
        case SETTING_UI_KILN_TEMP: {
            printf("setting kiln temp changed\n");
            gui_kiln_temp = (int32_t)val;
            view_kiln_update_gui(gui_kiln_power);
            ctrl_manual_set_temp(gui_kiln_temp);
        }
        break;
        case SETTING_UI_KILN_POWER: {
            printf("setting kiln pow changed\n");
            gui_kiln_power = (int32_t)val;
            view_kiln_update_gui(gui_kiln_power);
            ctrl_manual_set_power(gui_kiln_power * 10);
        }
        break;
        case SETTING_UI_RESET: {
            if ((int32_t)val) cpu_reset();
        }
        break;
        case SETTING_PID_K_P: ctrl_pid_set_k_p((uint32_t)val); break;
        case SETTING_PID_K_I: ctrl_pid_set_k_i((uint32_t)val); break;
        case SETTING_PID_K_D: ctrl_pid_set_k_d((uint32_t)val); break;
        case SETTING_PID_EPSILON: ctrl_pid_set_epsilon((uint32_t)val); break;

        default:break;
    }
}
static const user_setting_t *gui_setting_get_cb(uint32_t ix) {
    static user_setting_t gui_setting;
    switch (ix) {
        case SETTING_UI_KILN_TEMP: 
        gui_setting.id = SETTING_UI_KILN_TEMP;
        gui_setting.max_value = settings_get_val_for_id(SETTING_KILN_MAX_TEMP);
        gui_setting.min_value = 0;
        gui_setting.ui_name = "Target temperature";
        gui_setting.unit = UNIT_DEGREE_C;
        break;
        case SETTING_UI_KILN_POWER: 
        gui_setting.id = SETTING_UI_KILN_POWER;
        gui_setting.max_value = (void *)10;
        gui_setting.min_value = 0;
        gui_setting.ui_name = "Kiln power";
        gui_setting.unit = UNIT_INTEGER;
        break;
        default: 
        ASSERT(0);
        break;
    }
    return &gui_setting;
}
static void *gui_setting_val_cb(uint32_t ix) {
    switch (ix) {
        case SETTING_UI_KILN_TEMP: return (void *)gui_kiln_temp;
        case SETTING_UI_KILN_POWER: return (void *)gui_kiln_power;
        default: return 0;
    }
}

// SETUP & LOOP

static void beep_init(void) {
    uint32_t freq = 90;
    uint32_t cpu_freq = cpu_core_clock_freq();
    uint32_t period = (cpu_freq / BEEP_TIM_PRESC) / freq;
    gpio_config(PIN_SPEAKER_PWM, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_NONE);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
    LL_TIM_SetClockDivision(TIM3, LL_TIM_CLOCKDIVISION_DIV1);
    LL_TIM_SetCounterMode(TIM3, LL_TIM_COUNTERMODE_UP);
    LL_TIM_SetPrescaler(TIM3, BEEP_TIM_PRESC-1);
    LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetRepetitionCounter(TIM3, 0);
    LL_TIM_SetOnePulseMode(TIM3, LL_TIM_ONEPULSEMODE_REPETITIVE);
    LL_TIM_SetCounter(TIM3, period);
    LL_TIM_SetAutoReload(TIM3, period);
    
    LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH3);

    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH3);
    LL_TIM_OC_ConfigOutput(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCPOLARITY_HIGH);
    LL_TIM_OC_SetMode(TIM3, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM1);
    LL_TIM_OC_SetCompareCH3(TIM3, period/2);
    LL_TIM_EnableARRPreload(TIM3);
    
    LL_TIM_EnableCounter(TIM3);
    gui_beep(90, TIMER_MS_TO_TICKS(1));
}

void gui_init(void) {
    beep_init();
    const uint16_t w = disp_width();
    const uint16_t h = disp_height();
    ui_set_main_panel(&main_panel);
    ui_panel_init(&main_panel, 0, 0, w, h);

    { // title
        ui_button_init(&back_button, 0,0,TITLE_H,TITLE_H, "\x86",  
            gui_event_cb, 0, 
            5, COL_BUTTON_CANCEL_FG, COL_BUTTON_CANCEL_BG1, COL_BUTTON_CANCEL_BG2);
        ui_label_init(&title, TITLE_H, 0, w-TITLE_H*2, TITLE_H, "", 5, COL_TITLE_FG, COL_TITLE_BG);
        ui_set_alignment(&title.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
        ui_label_init(&title_stat1, w-TITLE_H, 0, TITLE_H, TITLE_H/2, "   ",
            FONT_SMALL_ICONS, COL_TITLE_FG, COL_TITLE_BG);
        ui_label_init(&title_stat2, w-TITLE_H, TITLE_H/2, TITLE_H, TITLE_H/2, "   ", 
            FONT_SMALL_ICONS, COL_TITLE_FG, COL_TITLE_BG);
        ui_set_alignment(&title.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
        ui_set_alignment(&title_stat1.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
        ui_set_alignment(&title_stat2.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
    }

    { // main
        for (uint32_t i = 0; i < sizeof(ibuttons)/sizeof(ibuttons[0]); i++) {
            ui_ibutton_init(&ibuttons[i], (i & 1) * 100 + (w-100*2)/2,
                    ((i>>1) & 1) * 100 + (h-100*2)/2, 90,90,
                i, gui_event_cb, 0,
                (uint32_t[4]){0xffff8800,0xff44ff44,0xff44aaff,0xffff4444}[i]);
            ui_add(&main_panel.comp, &ibuttons[i].comp);
        }
    }


    { // settings
        ui_keypad_init(&keypad, 0, TITLE_H,  w, h-TITLE_H, gui_event_cb, 0);

        ui_list_init(&list, 0, TITLE_H,  w, h-TITLE_H, 
            settings_count(), 0, 0, gui_event_cb, 0,
            3, COL_LIST_FG, COL_LIST_BG1, COL_LIST_BG2);
        list.item_padding = 30;
        ui_set_alignment(&list.comp, UI_ALIGNH_LEFT, UI_ALIGNV_CENTER);

        ui_confirm_init(&confirm, 0,TITLE_H,w, h-TITLE_H,
                        gui_event_cb, 0,
                        "", "", "", "");
    }

    { // graph
        ui_label_init(&temp_label1, 0, TITLE_H, w/2, 32, "", 6, 0xffffffff, 0xff000000);
        ui_set_alignment(&temp_label1.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
        ui_label_init(&temp_label2, w/2, TITLE_H, w/2, 32, "", 6, 0xffffffff, 0xff000000);
        ui_set_alignment(&temp_label2.comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
        ui_graph_init(&temp_graph, 0, TITLE_H+32, w, h-TITLE_H-32,
            gui_event_cb,0,
            t_buf, sizeof(t_buf)/sizeof(t_buf[0]),
            0xffffffff, 0xff000000);
        temp_graph.user_max = 100;
        temp_graph.user_min = -5;
    }

    { // kiln
        int y = TITLE_H;
        const int but_sz = 50;
        ui_label_init(&kiln_info_labels[0], 0, y, w, 24, "T E M P E R A T U R E",
            3, COL_DISABLED_FG, COL_DISABLED_BG1);
        y += 24;

        ui_button_init(&kiln_temp_buttons[0], w/2-but_sz, y, but_sz, but_sz, "-", 
            gui_event_cb, 0, 
            6, COL_BUTTON_FG, COL_BUTTON_BG1, COL_BUTTON_BG2);
        ui_button_init(&kiln_temp_buttons[1], w-but_sz, y, but_sz, but_sz, "+", 
            gui_event_cb, 0, 
            6, COL_BUTTON_FG, COL_BUTTON_BG1, COL_BUTTON_BG2);
        ui_label_init(&kiln_temp_labels[0], 0, y, w/2-but_sz, but_sz, "", 
            6, COL_INFO_PRIO1, COL_BG);
        ui_label_init(&kiln_temp_labels[1], w/2, y, w/2-but_sz, but_sz, "20", 
            6, COL_INPUT_FG, COL_INPUT_BG);
        kiln_temp_labels[0].comp.handle_event = button_event;
        kiln_temp_labels[0].call.cb  = gui_event_cb;
        kiln_temp_labels[1].comp.handle_event = button_event;
        kiln_temp_labels[1].call.cb  = gui_event_cb;
        y += but_sz;

        ui_button_init(&kiln_stop_button, w/2,y,w/2,h-y,
            "STOP", gui_event_cb, 0, 
            6, COL_BUTTON_CANCEL_FG, COL_BUTTON_CANCEL_BG1, COL_BUTTON_CANCEL_BG2);

        ui_label_init(&kiln_info_labels[1], 0, y, w/2, 24, "P O W E R",
            3, COL_DISABLED_FG, COL_DISABLED_BG1);
        y += 24;

        ui_label_init(&kiln_pow_label, 0, y, w/2, but_sz-5, "OFF", 
            6, COL_INFO_PRIO1, COL_BG);
        ui_set_alignment(&kiln_temp_labels[0].comp, UI_ALIGNH_LEFT, UI_ALIGNV_CENTER);
        ui_set_alignment(&kiln_pow_label.comp, UI_ALIGNH_LEFT, UI_ALIGNV_CENTER);
        y += but_sz-5;
        ui_slider_init(&kiln_pow_slider, 0, y, w/2, h - y,
            gui_event_cb, 0, 
            0,10,gui_kiln_power,
            1,
            "\x35", FONT_SMALL_ICONS,
            COL_SLIDER_FG, COL_SLIDER_BG1, COL_SLIDER_BG2);
    }

    ui_add(&main_panel.comp, &back_button.comp);
    ui_add(&main_panel.comp, &title.comp);
    ui_add(&main_panel.comp, &title_stat1.comp);
    ui_add(&main_panel.comp, &title_stat2.comp);
    ui_add(&main_panel.comp, &keypad.comp);
    ui_add(&main_panel.comp, &list.comp);
    ui_add(&main_panel.comp, &confirm.comp);
    ui_add(&main_panel.comp, &temp_label1.comp);
    ui_add(&main_panel.comp, &temp_label2.comp);
    ui_add(&main_panel.comp, &temp_graph.comp);
    for (uint32_t i = 0; i < 2; i++) {
        ui_add(&main_panel.comp, &kiln_temp_buttons[i].comp);
        ui_add(&main_panel.comp, &kiln_temp_labels[i].comp);
        ui_add(&main_panel.comp, &kiln_info_labels[i].comp);
    }
    ui_add(&main_panel.comp, &kiln_pow_label.comp);
    ui_add(&main_panel.comp, &kiln_pow_slider.comp);
    ui_add(&main_panel.comp, &kiln_stop_button.comp);
    gui_hide_all();

    gui_view_open(&view_main, 0, 0);

    ui_set_click_cb(gui_click_cb);

    settings_set_cb_fns(gui_setting_change_cb, gui_setting_get_cb, gui_setting_val_cb);
    ui_post_event(ui_new_event(EVENT_REPAINT)); // trigger a repaint

    if (gpio_read(BOARD_BUTTON_GPIO_PIN[1]) == BOARD_BUTTON_GPIO_ACTIVE[1]) {
        gui_view_open(&view_info, 0, 0);
    }
}

void gui_set_attention(kiln_attention_t att) {
    gui_attention |= true;
}

static uint32_t gui_second = 0;
static uint32_t idle_second = 0;
void gui_loop(void) {
    static uint32_t gui_graph_thermo_sum = 0;
    static uint32_t gui_graph_thermo_div = 0;
    static uint32_t gui_ctrl_thermo_sum = 0;
    static uint32_t gui_ctrl_thermo_div = 0;
    ui_event_t *ui_event;
    bool repaint = false;
    while ((ui_event = ui_pop_event())) {
        switch (ui_event->type) {
            case EVENT_BEEP:
            gui_beep(ui_event->beep.freq, ui_event->beep.time);
            break;
            case EVENT_SECOND: {
                gui_second++;
                idle_second++;
                if (idle_second >= SLEEP_TIMEOUT_S && !gui_sleeping) {
                    gui_sleeping = true;
                    disp_set_light(0x00);
                    ui_repaint(NULL);
                    repaint = true;
                }

                ctrl_second_callback(gui_second);
                int32_t now = (int32_t)ui_event->time;
                if (now - gui_temp_graph_last_time >=
                    (int32_t)settings_get_val_for_id(SETTING_TEMP_GRAPH_UPDATE)) {
                    gui_temp_graph_last_time = now;
                    uint32_t thermo = (gui_graph_thermo_sum + gui_graph_thermo_div/2)/gui_graph_thermo_div;
                    if (gui_graph_thermo_div) repaint |= temp_graph_update(thermo);
                    gui_graph_thermo_div = gui_graph_thermo_sum = 0;
                }
                if (kiln_temp_labels[0].comp.state & UI_STATE_VISIBLE) {
                    char str[32] = {0};
                    snprintf(str, sizeof(str)-1, "%d`C", (gui_ctrl_thermo_sum + gui_ctrl_thermo_div/2)/gui_ctrl_thermo_div);
                    ui_label_set_text(&kiln_temp_labels[0], str);
                    gui_ctrl_thermo_div = gui_ctrl_thermo_sum = 0;
                    repaint = 1;
                }
                update_title_status();
                gui_attention = ctrl_is_panicking() != 0;
            }
            break;
            case EVENT_THERMO: {
                uint32_t thermo_error = (uint32_t)ui_event->user;
                int32_t temp = ui_event->value;
                gui_graph_thermo_sum += temp;
                gui_graph_thermo_div++;
                gui_ctrl_thermo_sum += temp;
                gui_ctrl_thermo_div++;
                gui_attention |= (thermo_error) != 0;
            }            
            break;
            case EVENT_KILN_ON:
            ui_set_enabled(&ibuttons[2].comp, 0);
            if (!ctrl_is_panicking()) {
                if (ui_event->value == CONTROL_MANUAL) {
                    ibuttons[1].comp.state &= ~UI_STATE_ACTIVE;
                    ibuttons[3].comp.state |=  UI_STATE_ACTIVE;
                    static const uint16_t tune_kiln_on_manual_f[] = {880, 1108, 1318};
                    static const uint16_t tune_kiln_on_manual_t[] = {110, 110, 110};
                    gui_melody(tune_kiln_on_manual_f, tune_kiln_on_manual_t, sizeof(tune_kiln_on_manual_f)/sizeof(tune_kiln_on_manual_f[0]));
                } else {
                    ibuttons[1].comp.state |=  UI_STATE_ACTIVE;
                    ibuttons[3].comp.state &= ~UI_STATE_ACTIVE;
                    static const uint16_t tune_kiln_on_manual_f[] = {1108, 880, 1318};
                    static const uint16_t tune_kiln_on_manual_t[] = {110, 110, 110};
                    gui_melody(tune_kiln_on_manual_f, tune_kiln_on_manual_t, sizeof(tune_kiln_on_manual_f)/sizeof(tune_kiln_on_manual_f[0]));
                }
            }
            repaint = true;
            break;
            case EVENT_KILN_OFF:
            ibuttons[1].comp.state &= ~UI_STATE_ACTIVE;
            ibuttons[3].comp.state &= ~UI_STATE_ACTIVE;
            ui_set_enabled(&ibuttons[2].comp, 1);
            static const uint16_t tune_kiln_off_f[] = {784, 650};
            static const uint16_t tune_kiln_off_t[] = {110, 110};
            gui_melody(tune_kiln_off_f, tune_kiln_off_t, sizeof(tune_kiln_off_f)/sizeof(tune_kiln_off_f[0]));
            repaint = true;
            break;
            case EVENT_PANIC:
            ibuttons[1].comp.state &= ~UI_STATE_ACTIVE;
            ibuttons[3].comp.state &= ~UI_STATE_ACTIVE;
            ui_set_enabled(&ibuttons[0].comp, 1);
            ui_set_enabled(&ibuttons[1].comp, 0);
            ui_set_enabled(&ibuttons[2].comp, 1);
            ui_set_enabled(&ibuttons[3].comp, 0);
            break;
            default:
            if (!gui_sleeping) {
                ui_handle_event(ui_event);
                repaint = true;
            }
            break;
        } // switch event type

        if (ui_event->type != EVENT_SECOND && 
            ui_event->type != EVENT_REPAINT && 
            ui_event->type != EVENT_THERMO) {
            idle_second = 0;
            if (gui_sleeping) {
                gui_sleeping = false;
                disp_set_light(0xff);
                ui_repaint(NULL);
                repaint = true;
            }
        }
        
        ui_free_event(ui_event);

    }
    if (repaint) ui_paint(NULL);
}

static int cli_graph_fake(int argc, const char **argv) {
    for (int i = 0; i < atoi(argv[0]); i++) {
        ui_graph_add(&temp_graph, i);
    }
    return 0;
}
CLI_FUNCTION(cli_graph_fake, "graph_fake", ": fake data to graph");

static int cli_gui_sleep(int argc, const char **argv) {
    disp_set_light(0);
    ui_repaint(NULL);
    ui_post_event(ui_new_event(EVENT_REPAINT));
    return 0;
}
CLI_FUNCTION(cli_gui_sleep, "gui_sleep", ": sleep");

static int cli_gui_wup(int argc, const char **argv) {
    disp_set_light(0xff);
    ui_repaint(NULL);
    ui_post_event(ui_new_event(EVENT_REPAINT));
    return 0;
}
CLI_FUNCTION(cli_gui_wup, "gui_wup", ": wakeup");

