#include "bmtypes.h"
#include "disp.h"
#include "minio.h"
#include "ui.h"

#define TXT_BACKSPACE   "\x80\x81"
#define TXT_ENTER       "\x87\x88"

static const char *KEYPAD_BUTTON_TEXTS[12] = {
    "1","4","7","2","5","8","3","6","9",TXT_BACKSPACE,"0",TXT_ENTER
};

static int keypad_cb(ui_component_t *c, ui_event_t *e, ui_info_t *i) {
    ui_button_t *button = (ui_button_t *)c;
    ui_keypad_t *keypad = (ui_keypad_t *)c->parent;
    uint8_t ix = strlen(keypad->text);
    if (ix >= sizeof(keypad->text) -1) return 0;
    if (strcmp(button->text, TXT_BACKSPACE) == 0) {
        if (ix) {
            keypad->text[ix-1] = 0;
            if (ix == 1) ui_set_enabled(&keypad->buttons[9].comp, 0);
        }
    } else if (strcmp(button->text, TXT_ENTER) == 0) {
        if (keypad->call.cb) {
            ui_event_t *cbe = ui_new_event(EVENT_CB_USER_INPUT);
            cbe->source = &keypad->comp;
            cbe->value = atoi(keypad->label.text);
            ui_post_event(cbe);
        }
    } else {
        ui_set_enabled(&keypad->buttons[9].comp, 1);
        if (ix == 1 && keypad->text[0] == '0') {
            ix = 0;
        }
        keypad->text[ix] = button->text[0];
        keypad->text[ix+1] = 0;
    }
    
    ui_label_set_text(&keypad->label, keypad->text);
    return 0;
}


void ui_keypad_set_value(ui_keypad_t *ui, int32_t val) {
    memset(ui->text, 0, sizeof(ui->text));
    snprintf(ui->text, sizeof(ui->text)-1, "%d", val);
    ui_set_enabled(&ui->buttons[9].comp, 1);
    ui_label_set_text(&ui->label, ui->text);
    ui->comp.state |= UI_STATE_REPAINT;
}

void ui_keypad_init(ui_keypad_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    event_fn_t callback, void *user) {
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "KPD";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = __ui_paint_to_children;
    ui->comp.handle_event = __ui_event_to_children;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    ui->call.cb = callback;
    ui->call.user = user;

    memset(ui->text, 0, sizeof(ui->text));
    ui_label_init(&ui->label, 0,0, w,h/4, ui->text, 6, COL_INPUT_FG, COL_INPUT_BG);
    ui_set_alignment(&ui->label.comp, UI_ALIGNH_RIGHT, UI_ALIGNV_CENTER);
    ui_add(&ui->comp, &ui->label.comp);

    int bix = 0;
    for (int xx = 0; xx < 4; xx++) { for (int yy = 0; yy < 3; yy++) {
        uint32_t fg = COL_BUTTON_FG;
        uint32_t bg1 = COL_BUTTON_BG1;
        uint32_t bg2 = COL_BUTTON_BG2;
        if (bix == 11) {
            fg = COL_BUTTON_COMMIT_FG;
            bg1 = COL_BUTTON_COMMIT_BG1;
            bg2 = COL_BUTTON_COMMIT_BG2;
        }
        ui_button_init(&ui->buttons[bix], 
            (xx * w)/4, ((1 + yy) * h)/4, 
            w/4, h/4, 
            KEYPAD_BUTTON_TEXTS[bix], keypad_cb, ui,
            5, fg, bg1, bg2);
        ui_set_alignment(&ui->buttons[bix].comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);
        ui_add(&ui->comp, &ui->buttons[bix].comp);
        bix++;
    }}
    ui_set_enabled(&ui->buttons[9].comp, 0);
}
