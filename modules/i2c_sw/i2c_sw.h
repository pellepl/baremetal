/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _I2C_SW_H_
#define _I2C_SW_H_

// bit banging i2c master

#include "bmtypes.h"

#ifndef ERR_I2C_SW_BASE
#define ERR_I2C_SW_BASE       (20)
#endif
#define ERR_I2C_SW_ARB_LOST   -(ERR_I2C_SW_BASE+0)
#define ERR_I2C_SW_HANG       -(ERR_I2C_SW_BASE+1)
#define ERR_I2C_SW_NOACK      -(ERR_I2C_SW_BASE+2)

typedef struct {
    uint8_t _started;
    uint16_t scl_pin;
    uint16_t sda_pin;
    unsigned int delay; // busy loop delay between pin transitions.
    unsigned int stretch_tmo; // set to zero to disable stretch check
} i2c_sw_t;

void i2c_sw_init(unsigned int port, uint16_t scl_pin, uint16_t sda_pin, unsigned int delay, unsigned int max_stretch_time);
int i2c_sw_tx(unsigned int port, uint8_t data, uint8_t *ack, uint8_t start_cond, uint8_t stop_cond);
int i2c_sw_rx(unsigned int port, uint8_t *data, uint8_t ack, uint8_t stop_cond);
int i2c_sw_tx_buf(unsigned int port, uint8_t addr, const uint8_t *buf, unsigned int len, uint8_t stop_cond);
int i2c_sw_rx_buf(unsigned int port, uint8_t addr, uint8_t *buf, unsigned int len, uint8_t stop_cond);
// clocks scl until sda becomes high
void i2c_sw_rescue(unsigned int port);

#ifdef NO_I2C_SW_DEFAULT_GPIO
extern int i2c_sw_gpio_get(uint16_t pin);
extern void i2c_sw_gpio_set(uint16_t pin, uint8_t state);
#endif // !NO_I2C_SW_DEFAULT_GPIO

#endif /* _I2C_SW_H_ */
