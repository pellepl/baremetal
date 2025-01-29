#include "board.h"
#include "disp.h"
#include "minio.h"
#include "gpio_driver.h"
#include "fonts/fonts.h"
#include <stdarg.h>
#include "assert.h"
#include "cli.h"

#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_fsmc.h"

#define PORTA(x) (0 * 16 + (x))
#define PORTB(x) (1 * 16 + (x))
#define PORTC(x) (2 * 16 + (x))
#define PORTD(x) (3 * 16 + (x))
#define PORTE(x) (4 * 16 + (x))
#define PORTF(x) (5 * 16 + (x))
#define PORTG(x) (6 * 16 + (x))

// kudos https://github.com/kbiva/stm32f103_libs

typedef enum
{
    COLOR_16BIT = 0x1000,
    COLOR_18BIT = 0xD000
} lgdp4532_color_mode_t;

typedef enum
{
    ORIENTATION_PORTRAIT = 0x0030,
    ORIENTATION_LANDSCAPE = 0x0018,
    ORIENTATION_PORTRAIT_REV = 0x0000,
    ORIENTATION_LANDSCAPE_REV = 0x0028
} lgdp4532_orientation_t;

static lgdp4532_orientation_t orientation = ORIENTATION_LANDSCAPE_REV;

typedef enum
{
    STARTOSCILLATION = 0x00,
    DRIVEROUTPUTCONTROL = 0x01,
    DRIVINGWAVECONTROL = 0x02,
    ENTRYMODE = 0x03,
    RESIZINGCONTROL = 0x04,
    DISPLAYCTRL1 = 0x07,
    DISPLAYCTRL2 = 0x08,
    DISPLAYCTRL3 = 0x09,
    DISPLAYCTRL4 = 0x0A,
    EPROMCONTROLREGISTER1 = 0x40,
    EPROMCONTROLREGISTER2 = 0x41,
    EPROMCONTROLREGISTER3 = 0x42,
    RGBDISPLAYINTERFACECTRL1 = 0x0C,
    RGBDISPLAYINTERFACECTRL2 = 0x0F,
    FRAMEMARKERPOSITION = 0x0D,
    POWERCTRL1 = 0x10,
    POWERCTRL2 = 0x11,
    POWERCTRL3 = 0x12,
    POWERCTRL4 = 0x13,
    POWERCTRL7 = 0x29,
    HORIZONTALADDRESS = 0x20,
    VERTICALADDRESS = 0x21,
    GRAMSTARTWRITING = 0x22,
    FRAMERATEANDCOLORCONTROL = 0x2B,
    GAMMACONTROL_RED1 = 0x30,
    GAMMACONTROL_RED2 = 0x31,
    GAMMACONTROL_RED3 = 0x32,
    GAMMACONTROL_RED4 = 0x33,
    GAMMACONTROL_RED5 = 0x34,
    GAMMACONTROL_RED6 = 0x35,
    GAMMACONTROL_RED7 = 0x36,
    GAMMACONTROL_RED8 = 0x37,
    GAMMACONTROL_RED9 = 0x38,
    GAMMACONTROL_RED10 = 0x39,
    GAMMACONTROL_RED11 = 0x3A,
    GAMMACONTROL_RED12 = 0x3B,
    GAMMACONTROL_RED13 = 0x3C,
    GAMMACONTROL_RED14 = 0x3D,
    GAMMACONTROL_RED15 = 0x3E,
    GAMMACONTROL_RED16 = 0x3F,
    HORIZONTALRAMPOSITIONSTART = 0x50,
    HORIZONTALRAMPOSITIONEND = 0x51,
    VERTICALRAMPOSITIONSTART = 0x52,
    VERTICALRAMPOSITIONEND = 0x53,
    GATESCANCONTROL1 = 0x60,
    GATESCANCONTROL2 = 0x61,
    GATESCANCONTROLSCROLL = 0x6A,
    PARTIALIMAGE1DISPLAYPOSITION = 0x80,
    PARTIALIMAGE1RAMSTARTADDRESS = 0x81,
    PARTIALIMAGE1RAMENDADDRESS = 0x82,
    PARTIALIMAGE2DISPLAYPOSITION = 0x83,
    PARTIALIMAGE2RAMSTARTADDRESS = 0x84,
    PARTIALIMAGE2RAMENDADDRESS = 0x85,
    PANELINTERFACECONTROL1 = 0x90,
    PANELINTERFACECONTROL2 = 0x92,
    PANELINTERFACECONTROL3 = 0x93,
    PANELINTERFACECONTROL4 = 0x95,
    PANELINTERFACECONTROL5 = 0x97,
    PANELINTERFACECONTROL6 = 0x98,
    REGULATORCONTROL = 0x15,
    TESTREGISTER1 = 0xA0,
    TESTREGISTER2 = 0xA1,
    TESTREGISTER3 = 0xA2,
    TESTREGISTER4 = 0xA3,
    GAMMACONTROL_GREEN1 = 0xB0,
    GAMMACONTROL_GREEN2 = 0xB1,
    GAMMACONTROL_GREEN3 = 0xB2,
    GAMMACONTROL_GREEN4 = 0xB3,
    GAMMACONTROL_GREEN5 = 0xB4,
    GAMMACONTROL_GREEN6 = 0xB5,
    GAMMACONTROL_GREEN7 = 0xB6,
    GAMMACONTROL_GREEN8 = 0xB7,
    GAMMACONTROL_GREEN9 = 0xB8,
    GAMMACONTROL_GREEN10 = 0xB9,
    GAMMACONTROL_GREEN11 = 0xBA,
    GAMMACONTROL_GREEN12 = 0xBB,
    GAMMACONTROL_GREEN13 = 0xBC,
    GAMMACONTROL_GREEN14 = 0xBD,
    GAMMACONTROL_GREEN15 = 0xBE,
    GAMMACONTROL_GREEN16 = 0xBF,
    GAMMACONTROL_BLUE1 = 0xC0,
    GAMMACONTROL_BLUE2 = 0xC1,
    GAMMACONTROL_BLUE3 = 0xC2,
    GAMMACONTROL_BLUE4 = 0xC3,
    GAMMACONTROL_BLUE5 = 0xC4,
    GAMMACONTROL_BLUE6 = 0xC5,
    GAMMACONTROL_BLUE7 = 0xC6,
    GAMMACONTROL_BLUE8 = 0xC7,
    GAMMACONTROL_BLUE9 = 0xC8,
    GAMMACONTROL_BLUE10 = 0xC9,
    GAMMACONTROL_BLUE11 = 0xCA,
    GAMMACONTROL_BLUE12 = 0xCB,
    GAMMACONTROL_BLUE13 = 0xCC,
    GAMMACONTROL_BLUE14 = 0xCD,
    GAMMACONTROL_BLUE15 = 0xCE,
    GAMMACONTROL_BLUE16 = 0xCF,
    TIMINGCTRL1 = 0xE3,
    TIMINGCTRL2 = 0xE7,
    TIMINGCTRL3 = 0xEF
} lgdp4532_cmd_t;

#define lgdp4532_reg (*((volatile uint16_t *)0x60000000))
#define lgdp4532_dat (*((volatile uint16_t *)0x60020000))

enum
{
    _MEM16B_START = 0,
    PIN_DB0 = _MEM16B_START,
    PIN_DB1,
    PIN_DB2,
    PIN_DB3,
    PIN_DB4,
    PIN_DB5,
    PIN_DB6,
    PIN_DB7,
    PIN_DB8,
    PIN_DB9,
    PIN_DB10,
    PIN_DB11,
    PIN_DB12,
    PIN_DB13,
    PIN_DB14,
    PIN_DB15,
    PIN_RW,
    PIN_RD,
    PIN_RS,
    PIN_CS,
    _MEM16B_END = PIN_CS,
    PIN_RST
};

static uint16_t pindef_mem_16b_bus[] = {
    PORTD(14), PORTD(15), PORTD(0), PORTD(1),
    PORTE(7), PORTE(8), PORTE(9), PORTE(10),
    PORTE(11), PORTE(12), PORTE(13), PORTE(14),
    PORTE(15), PORTD(8), PORTD(9), PORTD(10),
    PORTD(5), PORTD(4), PORTD(11), PORTD(7),
    PORTE(1)};

static int16_t cx0, cy0, cx1, cy1;
static uint8_t light = 0xff;

static uint32_t rbg32_to_rgb16(uint32_t rgb)
{
    if (light == 0)
        return 0;
    if (light == 0xff)
        return (((rgb) & 0xf80000) >> 8 | ((rgb) & 0xfc00) >> 5 | ((rgb) & 0xf8) >> 3);
    return ((((rgb >> 16) & 0xff) * light) & 0b1111100000000000) |
           (((((rgb >> 8) & 0xff) * light) >> 5) & 0b0000011111100000) |
           ((((rgb & 0xff) * light) >> 11) & 0b0000000000011111);
}

static inline void _lgdp4532_cmd_wr(uint16_t reg)
{
    lgdp4532_reg = reg;
}

static inline void _lgdp4532_reg_wr(uint16_t reg, uint16_t val)
{
    lgdp4532_reg = reg;
    lgdp4532_dat = val;
}

static inline void _lgdp4532_dat_wr(uint16_t val)
{
    lgdp4532_dat = val;
}

static inline uint16_t _lgdp4532_reg_rd(uint16_t reg)
{
    lgdp4532_reg = reg;
    return (lgdp4532_dat);
}

static inline uint16_t _lgdp4532_dat_rd(void)
{
    return (lgdp4532_dat);
}

static void lgdp4532_read_register(uint8_t reg, uint8_t length, uint16_t *val)
{
    uint8_t i;
    // first read is dummy read
    val[0] = _lgdp4532_reg_rd(reg);
    for (i = 0; i < length; i++)
    {
        val[i] = _lgdp4532_dat_rd();
    }
}

uint16_t disp_width(void)
{
    //    switch(orientation) {
    //    case ORIENTATION_LANDSCAPE:
    //    case ORIENTATION_LANDSCAPE_REV:
    return 320;
    //    break;
    //    default:
    //        return 240;
    //    break;
    //    }
}

uint16_t disp_height(void)
{
    //    switch(orientation) {
    //    case ORIENTATION_LANDSCAPE:
    //    case ORIENTATION_LANDSCAPE_REV:
    return 240;
    //    break;
    //    default:
    //        return 320;
    //    break;
    //    }
}

static void disp_set_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1, lgdp4532_orientation_t ori)
{
    switch (ori)
    {
    case ORIENTATION_LANDSCAPE:
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, y0);
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, y1);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, 319 - x1);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, 319 - x0);
        _lgdp4532_reg_wr(HORIZONTALADDRESS, y0);
        _lgdp4532_reg_wr(VERTICALADDRESS, 319 - x0);
        break;
    case ORIENTATION_LANDSCAPE_REV:
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, 239 - y1);
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, 239 - y0);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, x0);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, x1);
        _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0);
        _lgdp4532_reg_wr(VERTICALADDRESS, x0);
        break;
    case ORIENTATION_PORTRAIT:
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, x0);
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, x1);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, y0);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, y1);
        _lgdp4532_reg_wr(HORIZONTALADDRESS, x0);
        _lgdp4532_reg_wr(VERTICALADDRESS, y0);
        break;
    case ORIENTATION_PORTRAIT_REV:
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, 239 - x1);
        _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, 239 - x0);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, 319 - y1);
        _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, 319 - y0);
        _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - x0);
        _lgdp4532_reg_wr(VERTICALADDRESS, 319 - y0);
        break;
    }
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, 239 - y1);
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, 239 - y0);
    _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, x0);
    _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, x1);
    _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0);
    _lgdp4532_reg_wr(VERTICALADDRESS, x0);
    _lgdp4532_cmd_wr(GRAMSTARTWRITING);
}

void disp_clear(uint32_t color)
{
    disp_fill(0, 0, disp_width(), disp_height(), color);
}

void disp_set_light(uint8_t l)
{
    light = l;
}

void disp_fill(int16_t x0, int16_t y0, int w, int h, uint32_t color)
{
    if (x0 < cx0)
    {
        w += (cx0 - x0);
        x0 = cx0;
    }
    if (y0 < cy0)
    {
        h += (cy0 - y0);
        y0 = cy0;
    }
    if (x0 + w >= cx1)
        w = cx1 - x0;
    if (y0 + h >= cy1)
        h = cy1 - y0;
    uint32_t j = w * h;
    if (j == 0)
        return;
    uint16_t dst_col = rbg32_to_rgb16(color);

    disp_set_window(x0, y0, x0 + w - 1, y0 + h - 1, orientation);
    for (uint32_t i = 0; i < j; i++)
    {
        _lgdp4532_dat_wr(dst_col);
    }
}

void disp_sleep(void)
{

    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0132);
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0122);
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0102);
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0000);
    cpu_halt(1);
    _lgdp4532_reg_wr(POWERCTRL1, _lgdp4532_reg_rd(POWERCTRL1) | (1 << 1));
}

void disp_wakeup(void)
{
    _lgdp4532_reg_wr(POWERCTRL1, _lgdp4532_reg_rd(POWERCTRL1) & ~(1 << 1));
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0001);
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0021);
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0023);
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0033);
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0133);
}

void disp_draw_hline(int16_t x0, int16_t y0, int w, uint32_t col, uint8_t thickness)
{
    uint8_t adj_min = (thickness - 1) / 2;
    uint8_t adj_max = thickness / 2;
    if (x0 < cx0)
    {
        w += (cx0 - x0);
        x0 = cx0;
    }
    if (x0 + w >= cx1)
        w = cx1 - x0;
    if (w <= 0)
        return;
    int16_t y1 = y0 + adj_max;
    y0 = y0 - adj_min;
    if (y1 < cy0 || y0 > cy1)
        return;
    if (y1 > cy1)
        y1 = cy1;
    if (y0 < cy0)
        y0 = cy0;
    uint16_t c = rbg32_to_rgb16(col);
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, 239 - (y1));
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, 239 - (y0));
    _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0);
    _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, x0);
    _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, x0 + w);
    _lgdp4532_reg_wr(VERTICALADDRESS, x0);
    _lgdp4532_cmd_wr(GRAMSTARTWRITING);
    uint32_t fill = (w + 1) * (y1 - y0 + 1);
    while (fill--)
    {
        _lgdp4532_dat_wr(c);
    }
}

void disp_draw_vline(int16_t x0, int16_t y0, int h, uint32_t col, uint8_t thickness)
{
    uint8_t adj_min = (thickness - 1) / 2;
    uint8_t adj_max = thickness / 2;
    if (y0 < cy0)
    {
        h += (cy0 - y0);
        y0 = cy0;
    }
    if (y0 + h >= cy1)
        h = cy1 - y0;
    if (h <= 0)
        return;
    int16_t x1 = x0 + adj_max;
    x0 = x0 - adj_min;
    if (x1 < cx0 || x0 > cx1)
        return;
    if (x1 > cx1)
        x1 = cx1;
    if (x0 < cx0)
        x0 = cx0;
    uint16_t c = rbg32_to_rgb16(col);
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, 239 - (y0 + h));
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, 239 - y0);
    _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0);
    _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, x0);
    _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, x1);
    _lgdp4532_reg_wr(VERTICALADDRESS, x0);
    _lgdp4532_cmd_wr(GRAMSTARTWRITING);
    uint32_t fill = (h + 1) * (x1 - x0 + 1);
    while (fill--)
    {
        _lgdp4532_dat_wr(c);
    }
}

void disp_draw_pixel(int16_t x0, int16_t y0, uint32_t col)
{
    uint16_t c = rbg32_to_rgb16(col);
    if (x0 < cx0 || x0 > cx1 ||
        y0 < cy0 || y0 > cy1)
        return;
    disp_set_window(x0, y0, x0, y0, orientation);
    _lgdp4532_dat_wr(c);
}

void disp_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t col, uint8_t thickness)
{
    uint16_t c = rbg32_to_rgb16(col);
    // TODO CLIP
    disp_set_window(x0, y0, x1, y1, orientation);
    int w = x1 - x0;
    int h = y1 - y0;
    int dx0 = 0, dy0 = 0, dx1 = 0, dy1 = 0;
    if (w < 0)
        dx0 = -1;
    else if (w > 0)
        dx0 = 1;
    if (h < 0)
        dy0 = -1;
    else if (h > 0)
        dy0 = 1;
    dx1 = dx0;
    int longest = w < 0 ? -w : w;
    int shortest = h < 0 ? -h : h;
    if (longest < shortest)
    {
        int t = longest;
        longest = shortest;
        shortest = t;
        dy1 = dy0;
        dx1 = 0;
    }
    int num = longest >> 1;
    for (int i = 0; i < longest; i++)
    {
        _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0);
        _lgdp4532_reg_wr(VERTICALADDRESS, x0);
        _lgdp4532_cmd_wr(GRAMSTARTWRITING);
        _lgdp4532_dat_wr(c);
        num += shortest;
        if (num >= longest)
        {
            num -= longest;
            x0 += dx0;
            y0 += dy0;
        }
        else
        {
            x0 += dx1;
            y0 += dy1;
        }
    }
}

void disp_draw_string(const char *str, int16_t x0, int16_t y0,
                      int font, disp_align_t halign, disp_align_t valign, uint32_t fg, uint32_t bg)
{
    uint16_t fg16 = rbg32_to_rgb16(fg);
    uint16_t bg16 = rbg32_to_rgb16(bg);
    const font_t *font_hdl = font_get(font);
    if (font_hdl == 0)
        return;
    const uint8_t w = font_hdl->width;
    const uint8_t adv = font_hdl->advance;
    const uint8_t fh = font_hdl->height;
    const uint8_t *base_font_data = font_hdl->data;
    const uint16_t strw = adv * strlen(str);
    uint8_t h = fh; // glyph h

    if (halign == DISP_STR_ALIGN_CENTER)
    {
        x0 = (x0 - strw / 2);
    }
    else if (halign == DISP_STR_ALIGN_RIGHT_BOTTOM)
    {
        x0 = (x0 - strw);
    }

    if (valign == DISP_STR_ALIGN_CENTER)
    {
        y0 = (y0 - h / 2);
    }
    else if (valign == DISP_STR_ALIGN_RIGHT_BOTTOM)
    {
        y0 = (y0 - h);
    }

    int16_t x1 = x0 + w - 1;
    int16_t y1 = y0 + h - 1;

    int16_t start_y = 0;
    if (y0 < cy0)
        start_y = cy0 - y0;
    if (y0 > cy1 || start_y > h)
        return;
    if (y0 + h > cy1)
        h = cy1 - y0;

    char c;
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONSTART, 239 - y1);
    _lgdp4532_reg_wr(HORIZONTALRAMPOSITIONEND, 239 - y0);
    _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0);

    while (x0 < cx1 && (c = *str++) != 0)
    {
        if (c >= ' ' && x0 + w >= cx0)
        {
            const uint8_t *font_data = base_font_data + (c - ' ') * w * ((fh + 7) / 8);
            const uint8_t *font_data8 = &font_data[w];
            const uint8_t *font_data16 = &font_data[2 * w];
            const uint8_t *font_data24 = &font_data[3 * w];
            _lgdp4532_reg_wr(VERTICALRAMPOSITIONSTART, x0);
            _lgdp4532_reg_wr(VERTICALRAMPOSITIONEND, x1);
            _lgdp4532_cmd_wr(GRAMSTARTWRITING);

            if ((fg & 0xff000000) == 0)
            {
                for (uint8_t x = 0; x < w; x++)
                {
                    uint32_t bm = *font_data |
                                  (h > 8 ? *font_data8 << 8 : 0) |
                                  (h > 16 ? *font_data16 << 16 : 0) |
                                  (h > 24 ? *font_data24 << 24 : 0);
                    font_data++;
                    font_data8++;
                    font_data16++;
                    font_data24++;
                    if (x0 + x < cx0)
                        continue;
                    _lgdp4532_reg_wr(VERTICALADDRESS, x0 + x);
                    for (uint8_t y = start_y; y < h; y++)
                    {
                        if ((bm & (1 << y)) == 0)
                        {
                            _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0 - y);
                            _lgdp4532_cmd_wr(GRAMSTARTWRITING);
                            _lgdp4532_dat_wr(bg16);
                        }
                    }
                    if (x0 + x > cx1)
                        return;
                }
            }
            else if ((bg & 0xff000000) == 0)
            {
                for (uint8_t x = 0; x < w; x++)
                {
                    uint32_t bm = *font_data |
                                  (h > 8 ? *font_data8 << 8 : 0) |
                                  (h > 16 ? *font_data16 << 16 : 0) |
                                  (h > 24 ? *font_data24 << 24 : 0);
                    font_data++;
                    font_data8++;
                    font_data16++;
                    font_data24++;
                    if (bm == 0)
                        continue;
                    if (x0 + x < cx0)
                        continue;
                    _lgdp4532_reg_wr(VERTICALADDRESS, x0 + x);
                    for (uint8_t y = start_y; y < h; y++)
                    {
                        if ((bm & (1 << y)) != 0)
                        {
                            _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0 - y);
                            _lgdp4532_cmd_wr(GRAMSTARTWRITING);
                            _lgdp4532_dat_wr(fg16);
                        }
                    }
                    if (x0 + x > cx1)
                        return;
                }
            }
            else
            {
                for (uint8_t x = 0; x < w; x++)
                {
                    uint32_t bm = *font_data |
                                  (h > 8 ? *font_data8 << 8 : 0) |
                                  (h > 16 ? *font_data16 << 16 : 0) |
                                  (h > 24 ? *font_data24 << 24 : 0);
                    font_data++;
                    font_data8++;
                    font_data16++;
                    font_data24++;
                    if (x0 + x < cx0)
                        continue;
                    _lgdp4532_reg_wr(VERTICALADDRESS, x0 + x);
                    for (uint8_t y = start_y; y < h; y++)
                    {
                        _lgdp4532_reg_wr(HORIZONTALADDRESS, 239 - y0 - y);
                        _lgdp4532_cmd_wr(GRAMSTARTWRITING);
                        _lgdp4532_dat_wr(bm & (1 << y) ? fg16 : bg16);
                    }
                    if (x0 + x > cx1)
                        return;
                }
            }
        }
        x0 += adv;
    }

    _lgdp4532_reg_wr(ENTRYMODE, COLOR_16BIT | orientation);
}

void disp_draw_stringf(const char *f, int16_t x0, int16_t y0,
                       int font, disp_align_t halign, disp_align_t valign, uint32_t fg, uint32_t bg, ...)
{
    char buf[256];
    va_list arg_p;
    va_start(arg_p, bg);
    vn_printf((unsigned int)(intptr_t)buf, f, sizeof(buf) - 1, arg_p);
    va_end(arg_p);
    disp_draw_string(buf, x0, y0, font, halign, valign, fg, bg);
}

void disp_set_clip_d(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    cx0 = x0;
    cx1 = x1;
    cy0 = y0;
    cy1 = y1;
    ASSERT(cx1 > cx0);
    ASSERT(cy1 > cy0);
}
void disp_set_clip(disp_clip_t *clip)
{
    cx0 = clip->x0;
    cy0 = clip->y0;
    cx1 = clip->x1;
    cy1 = clip->y1;
    ASSERT(cx1 > cx0);
    ASSERT(cy1 > cy0);
}
void disp_get_clip(disp_clip_t *clip)
{
    if (!clip)
        return;
    clip->x0 = cx0;
    clip->y0 = cy0;
    clip->x1 = cx1;
    clip->y1 = cy1;
}

void disp_init(void)
{
    // setup gpio, enter reset
    gpio_config(pindef_mem_16b_bus[PIN_RST], GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_set(pindef_mem_16b_bus[PIN_RST], 0);
    for (uint16_t pin = _MEM16B_START; pin <= _MEM16B_END; pin++)
    {
        gpio_config(pindef_mem_16b_bus[pin], GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_NONE);
    }

    // setup norsram bus ifc
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_FSMC);

    FSMC_NORSRAM_TimingTypeDef timing = {
        .AddressSetupTime = 1,
        .AddressHoldTime = 1,
        .DataSetupTime = 1,
        .BusTurnAroundDuration = 0x00,
        .CLKDivision = 2,
        .DataLatency = 2,
        .AccessMode = FSMC_ACCESS_MODE_B};

    FSMC_NORSRAM_InitTypeDef init = {
        .NSBank = FSMC_NORSRAM_BANK1,
        .DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE,
        .MemoryType = FSMC_MEMORY_TYPE_NOR,
        .MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16,
        .BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE,
        .WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW,
        .WrapMode = FSMC_WRAP_MODE_DISABLE,
        .WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS,
        .WriteOperation = FSMC_WRITE_OPERATION_ENABLE,
        .WaitSignal = FSMC_WAIT_SIGNAL_DISABLE,
        .ExtendedMode = FSMC_EXTENDED_MODE_DISABLE,
        .WriteBurst = FSMC_WRITE_BURST_DISABLE,

        .PageSize = FSMC_PAGE_SIZE_NONE,
        .AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE,
    };

    (void)FSMC_NORSRAM_Init(FSMC_NORSRAM_DEVICE, &init);
    (void)FSMC_NORSRAM_Timing_Init(FSMC_NORSRAM_DEVICE, &timing, FSMC_NORSRAM_BANK1);

    __FSMC_NORSRAM_ENABLE(FSMC_NORSRAM_DEVICE, FSMC_NORSRAM_BANK1);

    // release reset
    gpio_set(pindef_mem_16b_bus[PIN_RST], 1);
    cpu_halt(10);

    uint16_t id = 0x0000;
    lgdp4532_read_register(0x00, 1, &id);
    if (id != 0x4532)
    {
        __FSMC_NORSRAM_DISABLE(FSMC_NORSRAM_DEVICE, FSMC_NORSRAM_BANK1);
        printf("ERROR: unexpected display response\n");
        return;
    }

    // power on
    _lgdp4532_reg_wr(STARTOSCILLATION, 0x0001); // Start oscillation
    cpu_halt(1);
    _lgdp4532_reg_wr(REGULATORCONTROL, 0x0030); // Regulator Control
    _lgdp4532_reg_wr(POWERCTRL2, 0x0040);       // Power Control 2 //set dc1,dc0,vc2:0:0040
    _lgdp4532_reg_wr(POWERCTRL1, 0x1628);       // Power Control 1 //set bt,sap,ap:1628
    _lgdp4532_reg_wr(POWERCTRL3, 0x0000);       // Power Control 3 //set vrh
    _lgdp4532_reg_wr(POWERCTRL4, 0x104d);       // Power Control 4 //set vdv,vcm
    cpu_halt(1);
    _lgdp4532_reg_wr(POWERCTRL3, 0x0010); // Power Control 3 //set vrh:0010
    cpu_halt(1);
    _lgdp4532_reg_wr(POWERCTRL1, 0x2620); // Power Control 1 //set bt,sap,ap:2620
    _lgdp4532_reg_wr(POWERCTRL4, 0x344d); // Power Control 4 //set vdv,vcm
    cpu_halt(1);
    // end power on

    _lgdp4532_reg_wr(DRIVEROUTPUTCONTROL, 0x0100); // Driver output control //set sm,ss
    _lgdp4532_reg_wr(DRIVINGWAVECONTROL, 0x0300);  // LCD Driving Wave Control ///set line/frame inversion ,BC0,EOR,NW5-0

    _lgdp4532_reg_wr(ENTRYMODE, COLOR_16BIT | orientation);

    _lgdp4532_reg_wr(DISPLAYCTRL2, 0x0604); // Display Control 2 //set fp,bp 0604
    _lgdp4532_reg_wr(DISPLAYCTRL3, 0x0000); // Display Control 3 // PTG normal scan
    _lgdp4532_reg_wr(DISPLAYCTRL4, 0x0008); // Display Control 4 // FMARK on, interval 1 frame

    _lgdp4532_reg_wr(EPROMCONTROLREGISTER2, 0x0002);  // EPROM Control Register 2
    _lgdp4532_reg_wr(GATESCANCONTROL1, 0x2700);       // Driver Output Control // set GS bit, lines 320
    _lgdp4532_reg_wr(GATESCANCONTROL2, 0x0003);       // Base Image Display Control // scroll enable
    _lgdp4532_reg_wr(PANELINTERFACECONTROL1, 0x0182); // Panel Interface Control 1 //set DIV1-0,RTN4-0 ,0199
    _lgdp4532_reg_wr(PANELINTERFACECONTROL3, 0x0001); // Panel Interface Control 3
    _lgdp4532_reg_wr(TESTREGISTER4, 0x0010);          // Test Register 4
    cpu_halt(1);

    // set gamma
    _lgdp4532_reg_wr(GAMMACONTROL_RED1, 0x0000);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED2, 0x0502);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED3, 0x0307);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED4, 0x0305);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED5, 0x0004);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED6, 0x0402);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED7, 0x0707);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED8, 0x0503);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED9, 0x1505);  // Red Gamma Control 1-16
    _lgdp4532_reg_wr(GAMMACONTROL_RED10, 0x1505); // Red Gamma Control 1-16
    cpu_halt(1);
    // end gamma set

    // display on
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0001); // Display Control 1
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0021); // Display Control 1
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0023); // Display Control 1
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0033); // Display Control 1
    cpu_halt(1);
    _lgdp4532_reg_wr(DISPLAYCTRL1, 0x0133); // Display Control 1
    // end display on
    cx0 = 0;
    cy0 = 0;
    cx1 = disp_width();
    cy1 = disp_height();

    disp_clear(0x000000);
}

static int cli_disp_clear(int argc, const char **argv)
{
    disp_clear(argc == 0 ? 0 : minio_strtol(argv[0], NULL, 0));
    return 0;
}
CLI_FUNCTION(cli_disp_clear, "disp_clear", "(col): clear display");
static int cli_disp_sleep(int argc, const char **argv)
{
    disp_sleep();
    return 0;
}
CLI_FUNCTION(cli_disp_sleep, "disp_sleep", ": display enter sleep");
static int cli_disp_wup(int argc, const char **argv)
{
    disp_wakeup();
    return 0;
}
CLI_FUNCTION(cli_disp_wup, "disp_wup", ": display wakeup");
