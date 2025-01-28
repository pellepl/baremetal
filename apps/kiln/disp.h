#ifndef _LGDP4532_H_
#define _LGDP4532_H_

typedef enum {
  DISP_STR_ALIGN_LEFT_TOP,
  DISP_STR_ALIGN_CENTER,
  DISP_STR_ALIGN_RIGHT_BOTTOM,
} disp_align_t;

typedef struct {
  int16_t x0,y0,x1,y1;
} disp_clip_t;

void disp_init(void);
uint16_t disp_width(void);
uint16_t disp_height(void);
void disp_wakeup(void);
void disp_sleep(void);
void disp_set_clip_d(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void disp_set_clip(disp_clip_t *clip);
void disp_get_clip(disp_clip_t *clip);
void disp_fill(int16_t x, int16_t y, int w, int h, uint32_t color);
void disp_set_light(uint8_t light);
void disp_clear(uint32_t color);
void disp_draw_pixel(int16_t x0, int16_t y0, uint32_t col);
void disp_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t col, uint8_t thickness);
void disp_draw_hline(int16_t x0, int16_t y0, int w, uint32_t col, uint8_t thickness);
void disp_draw_vline(int16_t x0, int16_t y0, int h, uint32_t col, uint8_t thickness);
void disp_draw_char(char c, int16_t x0, int16_t y0, int font, uint32_t fg, uint32_t bg);
void disp_draw_string(const char *str, int16_t x0, int16_t y0, 
    int font, disp_align_t halign, disp_align_t valign, uint32_t fg, uint32_t bg);
void disp_draw_stringf(const char *f, int16_t x0, int16_t y0, 
    int font, disp_align_t halign, disp_align_t valign, uint32_t fg, uint32_t bg, ...);

#endif // _LGDP4532_H_