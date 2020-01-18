/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "i2c_sw.h"

#ifndef CONFIG_I2C_SW_BUSES
#error Number of i2c software buses must be defined in CONFIG_I2C_SW_BUSES
#endif

static i2c_sw_t _hdl[CONFIG_I2C_SW_BUSES];

#ifndef NO_I2C_SW_DEFAULT_GPIO
#include "gpio_driver.h"

static int i2c_sw_gpio_get(uint16_t pin) {
    gpio_config(pin, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
    return gpio_read(pin);
}

static void i2c_sw_gpio_set(uint16_t pin, uint8_t state) {
    if (state) {
        // i2c busses are pulled up by definition, so it should suffice setting
        // the pin as input
        gpio_config(pin, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
    } else {
        gpio_set(pin, 0);
        gpio_config(pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }
}

#endif // !NO_I2C_SW_DEFAULT_GPIO

#define I2C_SW_GPIO_SCL_GET(p)    i2c_sw_gpio_get((p)->scl_pin)
#define I2C_SW_GPIO_SDA_GET(p)    i2c_sw_gpio_get((p)->sda_pin)
#define I2C_SW_GPIO_SCL_SET(p,v)  i2c_sw_gpio_set((p)->scl_pin, (v))
#define I2C_SW_GPIO_SDA_SET(p,v)  i2c_sw_gpio_set((p)->sda_pin, (v))

static void i2c_sw_delay(i2c_sw_t *hdl) {
    volatile unsigned int t = hdl->delay;
    while (t--);
}

static void i2c_sw_reset(i2c_sw_t *hdl) {
    // set as inputs
    (void)I2C_SW_GPIO_SCL_GET(hdl);
    (void)I2C_SW_GPIO_SDA_GET(hdl);
    hdl->_started = 0;
}

static unsigned int i2c_sw_stretch_check(i2c_sw_t *hdl) {
    if (hdl->stretch_tmo == 0)  return 1; // ignore stretch check
    unsigned int t = hdl->stretch_tmo;
    // delay when slave stretches clock
    while (--t > 0 && !I2C_SW_GPIO_SCL_GET(hdl));
    return t;
}

static int i2c_sw_start_cond(i2c_sw_t *hdl) {
    if (hdl->_started) {
        // already started, REstart condition
        I2C_SW_GPIO_SDA_SET(hdl, 1);
        (void)I2C_SW_GPIO_SCL_GET(hdl); // set as input
        i2c_sw_delay(hdl);
        if (!i2c_sw_stretch_check(hdl)) {
            i2c_sw_reset(hdl);
            return ERR_I2C_SW_HANG;
        }
        i2c_sw_delay(hdl);
    }

    // scl must be high, low flank on sda
    if (!I2C_SW_GPIO_SDA_GET(hdl)) {
        i2c_sw_reset(hdl);
        return ERR_I2C_SW_ARB_LOST;
    }

    I2C_SW_GPIO_SDA_SET(hdl, 0);
    i2c_sw_delay(hdl);
    I2C_SW_GPIO_SCL_SET(hdl, 0);
    hdl->_started = 1;

    return 0;
}

static int i2c_sw_stop_cond(i2c_sw_t *hdl) {
    I2C_SW_GPIO_SDA_SET(hdl, 0);
    (void)I2C_SW_GPIO_SCL_GET(hdl); // set as input
    i2c_sw_delay(hdl);

    if (!i2c_sw_stretch_check(hdl)) {
        i2c_sw_reset(hdl);
        return ERR_I2C_SW_HANG;
    }
    i2c_sw_delay(hdl);

    // scl must be high, high flank on sda
    I2C_SW_GPIO_SDA_SET(hdl, 1);
    i2c_sw_delay(hdl);

    if (!I2C_SW_GPIO_SDA_GET(hdl)) {
        i2c_sw_reset(hdl);
        return ERR_I2C_SW_ARB_LOST;
    }

    i2c_sw_delay(hdl);
    hdl->_started = 0;

    return 0;
}

static int i2c_sw_write_bit(i2c_sw_t *hdl, uint8_t bit) {
    I2C_SW_GPIO_SDA_SET(hdl, bit);
    i2c_sw_delay(hdl);
    I2C_SW_GPIO_SCL_SET(hdl, 1);
    i2c_sw_delay(hdl);
    if (!i2c_sw_stretch_check(hdl)) {
        i2c_sw_reset(hdl);
        return ERR_I2C_SW_HANG;
    }
    // scl high, data valid
    I2C_SW_GPIO_SCL_SET(hdl, 0);

    return 0;
}

static int i2c_sw_read_bit(i2c_sw_t *hdl, uint8_t *bit) {
    if (!bit) return -1;
    (void)I2C_SW_GPIO_SDA_GET(hdl); // set sda as input
    i2c_sw_delay(hdl);
    I2C_SW_GPIO_SCL_SET(hdl, 1);
    if (!i2c_sw_stretch_check(hdl)) {
        i2c_sw_reset(hdl);
        return ERR_I2C_SW_HANG;
    }
    i2c_sw_delay(hdl);
    // scl high, data valid
    *bit = I2C_SW_GPIO_SDA_GET(hdl);

    I2C_SW_GPIO_SCL_SET(hdl, 0);

    return 0;
}

int i2c_sw_tx(unsigned int port, uint8_t data, uint8_t *ack, uint8_t start_cond, uint8_t stop_cond) {
    int res;
    i2c_sw_t *hdl= &_hdl[port];
    if (start_cond) {
        res = i2c_sw_start_cond(hdl);
        if (res) goto err;
    }

    for (uint8_t bix = 0; bix < 8; bix++) {
        res = i2c_sw_write_bit(hdl, (data & 0x80) != 0);
        if (res) goto err;
        data <<= 1;
    }

    uint8_t nack;
    res = i2c_sw_read_bit(hdl, &nack);
    if (res) goto err;

    if (ack) {
        *ack = (nack == 0);
    }

    if (stop_cond) {
        res = i2c_sw_stop_cond(hdl);
    }

err:
    if (res) {
        i2c_sw_reset(hdl);
    }

    return res;
}

int i2c_sw_rx(unsigned int port, uint8_t *data, uint8_t ack, uint8_t stop_cond) {
    i2c_sw_t *hdl= &_hdl[port];

    int res;
    if (!data) return -1;

    uint8_t tmp = 0;
    for (uint8_t bix = 0; bix < 8; bix++) {
        uint8_t bit;
        res = i2c_sw_read_bit(hdl, &bit);
        if (res) goto err;
        tmp = (tmp << 1) | (bit ? 1 : 0);
    }

    *data = tmp;

    res = i2c_sw_write_bit(hdl, ack == 0);
    if (res) goto err;

    if (stop_cond) {
        res = i2c_sw_stop_cond(hdl);
    }

err:
    if (res) {
        i2c_sw_reset(hdl);
    }

    return res;
}

int i2c_sw_tx_buf(unsigned int port, uint8_t addr, const uint8_t *buf, unsigned int len, uint8_t stop_cond) {
    i2c_sw_t *hdl= &_hdl[port];

    int res;
    uint8_t ack;
    res = i2c_sw_tx(port, addr & 0xfe, &ack, 1, len == 0);
    if (res) goto err;
    if (!ack) {
        i2c_sw_reset(hdl);
        return ERR_I2C_SW_NOACK;
    }

    while (!res && len-- > 0) {
        res = i2c_sw_tx(port, *buf++, &ack, 0, stop_cond && (len == 0));
        if (!ack) {
            if (stop_cond) {
                res = i2c_sw_stop_cond(hdl);
                if (res) goto err;
            }
            i2c_sw_reset(hdl);
            return ERR_I2C_SW_NOACK;
        }
    }

err:
    if (res) {
        i2c_sw_reset(hdl);
    }

    return res;
}

int i2c_sw_rx_buf(unsigned int port, uint8_t addr, uint8_t *buf, unsigned int len, uint8_t stop_cond) {
    i2c_sw_t *hdl= &_hdl[port];

    int res;
    uint8_t ack;
    res = i2c_sw_tx(port, addr | 0x01, &ack, 1, len == 0);
    if (res) goto err;
    if (!ack) {
        i2c_sw_stop_cond(hdl);
        i2c_sw_reset(hdl);
        return ERR_I2C_SW_NOACK;
    }

    while (!res && len-- > 0) {
        res = i2c_sw_rx(port, buf, len > 0, stop_cond && (len == 0));
        buf++;
    }

err:
    if (res) {
        i2c_sw_reset(hdl);
    }

    return res;
}

void i2c_sw_init(unsigned int port, uint16_t scl_pin, uint16_t sda_pin, unsigned int delay, unsigned int stretch_tmo) {
    i2c_sw_t *hdl= &_hdl[port];
    hdl->scl_pin = scl_pin;
    hdl->sda_pin = sda_pin;
    hdl->delay = delay;
    hdl->stretch_tmo = stretch_tmo;
    i2c_sw_reset(hdl);
}

void i2c_sw_rescue(unsigned int port) {
    i2c_sw_t *hdl= &_hdl[port];
    int i, j;
    (void)i2c_sw_stop_cond(hdl);
    i2c_sw_delay(hdl);
    // clock until sda becomes released
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 8; i++) {
            I2C_SW_GPIO_SCL_SET(hdl, 0);
            i2c_sw_delay(hdl);
            I2C_SW_GPIO_SCL_SET(hdl, 1);
            i2c_sw_delay(hdl);
        }
        if (I2C_SW_GPIO_SDA_GET(hdl)) {
            break;
        }
        (void)i2c_sw_stop_cond(hdl);
    }
    i2c_sw_reset(hdl);
}
