#ifndef _TOUCH_H_
#define _TOUCH_H_

#include "spi_sw.h"

#define TOUCH_SPATIAL_FILTER_LEN        9
#define TOUCH_RELEASE_COOLDOWN          500

typedef struct {
    uint16_t x;
    uint16_t y;
} touch_coordinate_t;

typedef struct {
    volatile int touch_pressed_irq_flag;
    touch_coordinate_t last_raw_touch;
    int needs_calibration;
    struct {
        uint16_t minx, maxx, miny, maxy;
    } limits;
    uint8_t xy_filter_ix;
    union {
        touch_coordinate_t calib[5];
        struct {
            uint16_t x[TOUCH_SPATIAL_FILTER_LEN];
            uint16_t y[TOUCH_SPATIAL_FILTER_LEN];
            uint32_t lpx, lpy;
            uint8_t ix;
            uint8_t vals;
            uint32_t x_sum;
            uint32_t y_sum;
        } spatial_filter;
    };
} touch_sys_t;

void touch_init(uint8_t spi_port, spi_sw_t *spi_cfg, uint16_t cs_pin, uint16_t pen_irq_pin,
                touch_sys_t *tsys);
void touch_process(void);
void touch_get(touch_coordinate_t *xy);

#endif // _TOUCH_H_