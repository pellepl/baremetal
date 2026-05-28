/* Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _DS18B20_H_
#define _DS18B20_H_

// Bit banging DS18B20 / 1-wire temperature sensor driver.

#include "bmtypes.h"

#ifndef ERR_DS18B20_BASE
#define ERR_DS18B20_BASE             (40)
#endif

#define ERR_DS18B20_ARG              -(ERR_DS18B20_BASE + 0)
#define ERR_DS18B20_NO_PRESENCE      -(ERR_DS18B20_BASE + 1)
#define ERR_DS18B20_CRC              -(ERR_DS18B20_BASE + 2)
#define ERR_DS18B20_BUS              -(ERR_DS18B20_BASE + 3)

#define DS18B20_SCRATCHPAD_SIZE      (9)
#define DS18B20_ROM_SIZE             (8)

typedef enum
{
    DS18B20_RESOLUTION_9BIT = 0x1f,
    DS18B20_RESOLUTION_10BIT = 0x3f,
    DS18B20_RESOLUTION_11BIT = 0x5f,
    DS18B20_RESOLUTION_12BIT = 0x7f,
} ds18b20_resolution_t;

typedef struct
{
    uint16_t pin;
    uint8_t strong_pullup;
} ds18b20_t;

void ds18b20_init(ds18b20_t *dev, uint16_t pin, uint8_t strong_pullup);
void ds18b20_release_bus(const ds18b20_t *dev);
int ds18b20_reset(const ds18b20_t *dev);

int ds18b20_start_conversion(const ds18b20_t *dev, const uint8_t *rom);
int ds18b20_conversion_done(const ds18b20_t *dev, uint8_t *done);
uint16_t ds18b20_conversion_time_ms(ds18b20_resolution_t resolution);

int ds18b20_read_scratchpad(const ds18b20_t *dev, const uint8_t *rom, uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE]);
int ds18b20_read_temp_raw(const ds18b20_t *dev, const uint8_t *rom, int16_t *raw);
int ds18b20_read_temp_mdeg(const ds18b20_t *dev, const uint8_t *rom, int32_t *mdeg);
int ds18b20_set_resolution(const ds18b20_t *dev, const uint8_t *rom, ds18b20_resolution_t resolution);
int ds18b20_read_power_supply(const ds18b20_t *dev, const uint8_t *rom, uint8_t *externally_powered);

int32_t ds18b20_raw_to_mdeg(int16_t raw);
uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len);
uint8_t ds18b20_crc8_update(uint8_t crc, uint8_t data);

#ifdef NO_DS18B20_DEFAULT_GPIO
extern int ds18b20_gpio_get(uint16_t pin);
extern void ds18b20_gpio_release(uint16_t pin);
extern void ds18b20_gpio_drive_low(uint16_t pin);
extern void ds18b20_gpio_drive_high(uint16_t pin);
#endif // NO_DS18B20_DEFAULT_GPIO

#ifdef NO_DS18B20_DEFAULT_DELAY
extern void ds18b20_delay_us(uint32_t us);
#endif // NO_DS18B20_DEFAULT_DELAY

#endif // _DS18B20_H_
