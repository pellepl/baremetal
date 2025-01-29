#ifndef __FONTS_H_
#define __FONTS_H_

#define FONT_BIG_ICONS 7
#define FONT_SMALL_ICONS 8

typedef struct
{
    unsigned char width;
    unsigned char height;
    unsigned char advance;
    const unsigned char *data;
} font_t;
int font_count(void);
const font_t *font_get(int ix);
#endif