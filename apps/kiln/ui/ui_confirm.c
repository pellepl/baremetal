#include "bmtypes.h"
#include "disp.h"
#include "minio.h"
#include "ui.h"
#include "fonts/fonts.h"

static void confirm_paint(ui_component_t *ui, void *ctx, ui_info_t *i) {
    ui_confirm_t *c = (ui_confirm_t *)ui;
    if ((ui->state & UI_STATE_REPAINT) == 0) return;
    disp_fill(ui->x + (i?i->x:0), ui->y + (i?i->y:0),
        ui->w, ui->h, COL_INPUT_BG);
    ui_repaint(&c->comp);
    __ui_paint_to_children(ui, ctx, i);
}

static int confirm_cb(ui_component_t *c, ui_event_t *e, ui_info_t *i) {
    ui_button_t *button = (ui_button_t *)c;
    ui_confirm_t *confirm = (ui_confirm_t *)c->parent;
    e->value = (button == &confirm->buttons[0]);
    if (confirm->call.cb) confirm->call.cb(&confirm->comp, e, i);
    return 0;
}

void ui_confirm_init(ui_confirm_t *ui, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    event_fn_t callback, void *user, 
                    const char *hdr, const char *txt1,const char *txt2,const char *txt3) {
    ui->comp.state = UI_STATE_VISIBLE | UI_STATE_ENABLED | UI_STATE_REPAINT;
    ui->comp._type = "CFM";
    ui->comp.x = x;
    ui->comp.y = y;
    ui->comp.w = w;
    ui->comp.h = h;
    ui->comp.paint = confirm_paint;
    ui->comp.handle_event = __ui_event_to_children;
    ui->comp.parent = ui->comp.next = ui->comp.prev = ui->comp.children = 0;
    ui->call.cb = callback;
    ui->call.user = user;
    int16_t yy = 8;
    uint8_t font_hdr=6;
    uint8_t font_hdr_h = font_get(font_hdr)->height+4;
    uint8_t font_txt=5;
    uint8_t font_txt_h = font_get(font_txt)->height+4;
    ui_label_init(&ui->labels[0], 0,yy, w,font_hdr_h, hdr, font_hdr, COL_INPUT_FG, COL_INPUT_BG & 0xffffff);
    yy += font_hdr_h + 8;
    ui_label_init(&ui->labels[1], 4,yy, w-8,font_txt_h, txt1, font_txt, COL_INPUT_FG, COL_INPUT_BG & 0xffffff);
    yy += font_txt_h;
    ui_label_init(&ui->labels[2], 4,yy, w-8,font_txt_h, txt2, font_txt, COL_INPUT_FG, COL_INPUT_BG & 0xffffff);
    yy += font_txt_h;
    ui_label_init(&ui->labels[3], 4,yy, w-8,font_txt_h, txt3, font_txt, COL_INPUT_FG, COL_INPUT_BG & 0xffffff);
    yy += font_txt_h+8;
    for (int i = 0; i < 4; i++) {
        ui_set_alignment(&ui->labels[i].comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);        
        ui_add(&ui->comp, &ui->labels[i].comp);
    }
    for (int i = 0; i < 2; i++) {
        ui_button_init(&ui->buttons[i], 
            4 + i * (w-8) / 2, yy, 
            (w-8)/2, h-4-yy, 
            ((const char *[2]){"OK","CANCEL"})[i], confirm_cb, 0,
            5,
            i==0 ? COL_BUTTON_COMMIT_FG  : COL_BUTTON_CANCEL_FG,
            i==0 ? COL_BUTTON_COMMIT_BG1 : COL_BUTTON_CANCEL_BG1,
            i==0 ? COL_BUTTON_COMMIT_BG2 : COL_BUTTON_CANCEL_BG2
        );
        ui_set_alignment(&ui->buttons[i].comp, UI_ALIGNH_CENTER, UI_ALIGNV_CENTER);        
        ui_add(&ui->comp, &ui->buttons[i].comp);
    }
}
