#include <stdbool.h>

#include "touch.h"
#include "gpio_driver.h"
#include "minio.h"
#include "ui/ui.h"
#include "disp.h"

#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_exti.h"

#define LP_FILTER 0x30

enum XPT2046_bits
{
    PD0 = (1 << 0),
    PD1 = (1 << 1),
    MODE0 = (1 << 2),
    MODE1 = (1 << 3),
    A0 = (1 << 4),
    A1 = (1 << 5),
    A2 = (1 << 6),
    START = (1 << 7),
};

static uint8_t _spi_port;
static uint16_t _cs_pin;
static touch_sys_t *_sys;

static void gpio_setup_pen_irq(uint16_t irq_pin)
{
    gpio_config(irq_pin, GPIO_DIRECTION_INPUT, GPIO_PULL_DOWN);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    LL_GPIO_AF_SetEXTISource(LL_GPIO_AF_EXTI_PORTB, LL_GPIO_AF_EXTI_LINE6);
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_6);
    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_6);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void EXTI9_5_IRQHandler(void);
void EXTI9_5_IRQHandler(void)
{
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    if (LL_EXTI_ReadFlag_0_31(LL_EXTI_LINE_6))
    {
        _sys->touch_pressed_irq_flag = 1;
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);
    }
}

// int spi_sw_txrx(unsigned int port, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len)
static void sample(uint8_t cmd_1, uint8_t cmd_n, uint16_t *dst, uint8_t count)
{
    uint8_t txbuf[3] = {cmd_1, 0, 0};
    uint8_t rxbuf[3];
    spi_sw_txrx(_spi_port, txbuf, rxbuf, 3); // ignore first result, it sucks
    while (count > 1)
    {
        spi_sw_txrx(_spi_port, txbuf, rxbuf, 3);
        *dst++ = ((rxbuf[1] & 0x7f) << 5) | (rxbuf[2] >> 3);
        count--;
    }
    txbuf[0] = cmd_n;
    spi_sw_txrx(_spi_port, txbuf, rxbuf, 3);
    *dst++ = ((rxbuf[1] & 0x7f) << 5) | (rxbuf[2] >> 3);
}

void touch_init(uint8_t spi_port, spi_sw_t *spi_cfg, uint16_t cs_pin, uint16_t pen_irq_pin,
                touch_sys_t *tsys)
{
    _cs_pin = cs_pin;
    _spi_port = spi_port;
    _sys = tsys;
    gpio_setup_pen_irq(pen_irq_pin);
    gpio_config(cs_pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_set(cs_pin, 1);
    gpio_config(spi_cfg->mosi_pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(spi_cfg->miso_pin, GPIO_DIRECTION_INPUT, GPIO_PULL_UP);
    gpio_config(spi_cfg->clk_pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    spi_sw_init(spi_port,
                SPI_MODE_1,
                SPI_TRANSMISSION_MSB_FIRST,
                SPI_TRANSMISSION_MSB_FIRST,
                spi_cfg->mosi_pin,
                spi_cfg->miso_pin,
                spi_cfg->clk_pin,
                spi_cfg->delay);
}

static uint32_t process_sample_data(const uint16_t *data, uint8_t count)
{
    // get average
    uint32_t sum = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        sum += data[i];
    }
    uint32_t avg = sum / count;

    // sort away the 2 most differenced values
    uint16_t max_d1 = 0;
    uint8_t max_d1_ix = 0;
    uint16_t max_d2 = 0;
    uint8_t max_d2_ix = 1;
    for (uint8_t i = 0; i < count; i++)
    {
        uint16_t d = data[i];
        uint16_t delta = d > avg ? (d - avg) : (avg - d);
        if (delta > max_d1)
        {
            max_d1 = delta;
            max_d1_ix = i;
        }
        else if (delta > max_d2)
        {
            max_d2 = delta;
            max_d2_ix = i;
        }
    }
    // take new average
    return (sum - data[max_d1_ix] - data[max_d2_ix]) / (count - 2);
}

void touch_get(touch_coordinate_t *xy)
{
    gpio_set(_cs_pin, 0);
    uint16_t xvals[5];
    uint16_t yvals[5];
    const uint8_t count = sizeof(xvals) / sizeof(xvals[0]);
    // x channel, do not power down
    sample(A0 | START | PD0 | PD1, A0 | START | PD0 | PD1, xvals, count);
    // y channel, power down
    sample(A0 | A2 | START | PD0 | PD1, A0 | A2 | START, yvals, count);
    gpio_set(_cs_pin, 1);
    if (xy)
    {
        xy->x = process_sample_data(xvals, count);
        xy->y = process_sample_data(yvals, count);
    }
}

void touch_process(void)
{
    static bool _prev_touch = false;
    static int _touch_glitch_filter = 0;
    static int _drag_cnt = 0;
    int irq = _sys->touch_pressed_irq_flag;
    int gpio = gpio_read(1 * 16 + 6) == 0;
    bool press = false;
    bool drag = false;
    bool release = false;
    if (_touch_glitch_filter)
        _touch_glitch_filter--;
    if (irq || gpio)
    {
        if (_touch_glitch_filter)
            return;
        touch_get(&_sys->last_raw_touch);
        if (irq)
            _sys->touch_pressed_irq_flag = 0;
        if (!_prev_touch)
        {
            press = true;
            _prev_touch = true;
            _drag_cnt = 0;
        }
        else
        {
            drag = true;
        }
    }
    else
    {
        if (_prev_touch)
        {
            release = true;
            _prev_touch = false;
            _touch_glitch_filter = TOUCH_RELEASE_COOLDOWN;
        }
    }

    if (_sys->needs_calibration)
    {
        if (drag)
        {
            _drag_cnt++;
            if (_drag_cnt == 10)
            {
                memcpy(&_sys->calib[_sys->needs_calibration - 1],
                       &_sys->last_raw_touch, sizeof(touch_coordinate_t));
            }
        }
        if (release)
            _sys->needs_calibration++;
    }
    else
    {
        if (press || drag || release)
        {
            if (press)
            {
                _sys->spatial_filter.x_sum = _sys->spatial_filter.y_sum = 0;
                _sys->spatial_filter.ix = _sys->spatial_filter.vals = 0;
            }
            int gx, gy;
            gx = ((_sys->last_raw_touch.x - _sys->limits.minx) * disp_width()) / (_sys->limits.maxx - _sys->limits.minx);
            gy = ((_sys->last_raw_touch.y - _sys->limits.miny) * disp_height()) / (_sys->limits.maxy - _sys->limits.miny);
            // filtering
            {
                _sys->spatial_filter.x_sum += gx;
                _sys->spatial_filter.y_sum += gy;
                if (_sys->spatial_filter.vals >= TOUCH_SPATIAL_FILTER_LEN)
                {
                    _sys->spatial_filter.x_sum -= _sys->spatial_filter.x[_sys->spatial_filter.ix];
                    _sys->spatial_filter.y_sum -= _sys->spatial_filter.y[_sys->spatial_filter.ix];
                }
                _sys->spatial_filter.x[_sys->spatial_filter.ix] = gx;
                _sys->spatial_filter.y[_sys->spatial_filter.ix] = gy;
                _sys->spatial_filter.ix++;
                if (_sys->spatial_filter.ix >= TOUCH_SPATIAL_FILTER_LEN)
                    _sys->spatial_filter.ix = 0;
                if (_sys->spatial_filter.vals < TOUCH_SPATIAL_FILTER_LEN + 1)
                    _sys->spatial_filter.vals++;
                // get avg
                int avg_x = _sys->spatial_filter.x_sum / _sys->spatial_filter.vals;
                int avg_y = _sys->spatial_filter.y_sum / _sys->spatial_filter.vals;
                if (_sys->spatial_filter.vals == TOUCH_SPATIAL_FILTER_LEN)
                {
                    _sys->spatial_filter.lpx = avg_x;
                    _sys->spatial_filter.lpy = avg_y;
                }
                if (_sys->spatial_filter.vals >= TOUCH_SPATIAL_FILTER_LEN)
                {
                    // sort away the two with worst distance
                    int max_d1 = -1;
                    uint8_t max_d1_ix = 0;
                    int max_d2 = -1;
                    uint8_t max_d2_ix = 0;
                    for (uint8_t i = 0; i < TOUCH_SPATIAL_FILTER_LEN; i++)
                    {
                        int deltax = _sys->spatial_filter.x[i] - avg_x;
                        int deltay = _sys->spatial_filter.y[i] - avg_y;
                        int delta = deltax * deltax + deltay * deltay;
                        if (delta > max_d1)
                        {
                            max_d1 = delta;
                            max_d1_ix = i;
                        }
                        else if (delta > max_d2)
                        {
                            max_d2 = delta;
                            max_d2_ix = i;
                        }
                    }
                    // new avg
                    avg_x = (_sys->spatial_filter.x_sum - _sys->spatial_filter.x[max_d1_ix] - _sys->spatial_filter.x[max_d2_ix]) / (TOUCH_SPATIAL_FILTER_LEN - 2);
                    avg_y = (_sys->spatial_filter.y_sum - _sys->spatial_filter.y[max_d1_ix] - _sys->spatial_filter.y[max_d2_ix]) / (TOUCH_SPATIAL_FILTER_LEN - 2);
                    // into lp
                    _sys->spatial_filter.lpx = (avg_x * LP_FILTER + _sys->spatial_filter.lpx * (0x100 - LP_FILTER)) >> 8;
                    _sys->spatial_filter.lpy = (avg_y * LP_FILTER + _sys->spatial_filter.lpy * (0x100 - LP_FILTER)) >> 8;
                    gx = _sys->spatial_filter.lpx;
                    gy = _sys->spatial_filter.lpy;
                }
            }
            static ui_event_t touch_event;
            if (press)
                touch_event.type = EVENT_PRESS;
            else if (release)
                touch_event.type = EVENT_RELEASE;
            else
                touch_event.type = EVENT_DRAG;
            touch_event.x = gx;
            touch_event.y = gy;
            touch_event.is_static = 1;
            ui_post_event(&touch_event);
        }
    }
}
