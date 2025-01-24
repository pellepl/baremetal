#include "fonts.h"
#include "font-4x6.c"
#include "font-6x8.c"
#include "font-8x8.c"
#include "font-8x16.c"
#include "font-16x16.c"
#include "font-16x24.c"
#include "font-24x32.c"
#include "font-32x32.c"
#include "font-12x12.c"

static const font_t _fonts[] = {
    (font_t) { .width = 4, .height = 6, .advance = 4, .data = (const unsigned char *)&__font_4x6[0] },
    (font_t) { .width = 6, .height = 8, .advance = 6, .data = (const unsigned char *)&__font_6x8[0] },
    (font_t) { .width = 8, .height = 8, .advance = 8, .data = (const unsigned char *)&__font_8x8[0] },
    (font_t) { .width = 8, .height = 16, .advance = 8, .data = (const unsigned char *)&__font_8x16[0] },
    (font_t) { .width = 16, .height = 16, .advance = 14, .data = (const unsigned char *)&__font_16x16[0] },
    (font_t) { .width = 16, .height = 24, .advance = 16, .data = (const unsigned char *)&__font_16x24[0] },
    (font_t) { .width = 24, .height = 32, .advance = 20, .data = (const unsigned char *)&__font_24x32[0] },
    (font_t) { .width = 32, .height = 32, .advance = 32, .data = (const unsigned char *)&__font_32x32[0] },
    (font_t) { .width = 12, .height = 12, .advance = 13, .data = (const unsigned char *)&__font_12x12[0] },
};

const font_t *font_get(int ix) {
    if (ix < 0 || ix >= font_count()) return (void *)0;
    return &_fonts[ix];
}
int font_count(void) {
    return sizeof(_fonts) / sizeof(_fonts[0]);
}
