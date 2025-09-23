#pragma once

#include <stdint.h>

#define FONT_WIDTH  14
#define FONT_HEIGHT  23
#define FONT_BYTES_PER_ROW 2

extern const uint8_t font_data[][FONT_HEIGHT*FONT_BYTES_PER_ROW];
