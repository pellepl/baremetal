/* Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "ds18b20.h"

#ifndef NO_DS18B20_DEFAULT_GPIO
#include "gpio_driver.h"

static int ds18b20_gpio_get(uint16_t pin)
{
    gpio_config(pin, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
    return gpio_read(pin);
}

static void ds18b20_gpio_release(uint16_t pin)
{
    gpio_config(pin, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
}

static void ds18b20_gpio_drive_low(uint16_t pin)
{
    gpio_set(pin, 0);
    gpio_config(pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
}

static void ds18b20_gpio_drive_high(uint16_t pin)
{
    gpio_set(pin, 1);
    gpio_config(pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
}
#endif // !NO_DS18B20_DEFAULT_GPIO

#ifndef NO_DS18B20_DEFAULT_DELAY
#include "cpu.h"

static void ds18b20_delay_us(uint32_t us)
{
    cpu_halt_us(us);
}
#endif // !NO_DS18B20_DEFAULT_DELAY

#ifndef DS18B20_CRITICAL_ENTER
#define DS18B20_CRITICAL_ENTER()
#endif

#ifndef DS18B20_CRITICAL_EXIT
#define DS18B20_CRITICAL_EXIT()
#endif

#ifndef DS18B20_RESET_LOW_US
#define DS18B20_RESET_LOW_US         (480)
#endif
#ifndef DS18B20_RESET_SAMPLE_US
#define DS18B20_RESET_SAMPLE_US      (70)
#endif
#ifndef DS18B20_RESET_FINISH_US
#define DS18B20_RESET_FINISH_US      (410)
#endif
#ifndef DS18B20_WRITE_1_LOW_US
#define DS18B20_WRITE_1_LOW_US       (6)
#endif
#ifndef DS18B20_WRITE_1_FINISH_US
#define DS18B20_WRITE_1_FINISH_US    (64)
#endif
#ifndef DS18B20_WRITE_0_LOW_US
#define DS18B20_WRITE_0_LOW_US       (60)
#endif
#ifndef DS18B20_WRITE_0_FINISH_US
#define DS18B20_WRITE_0_FINISH_US    (10)
#endif
#ifndef DS18B20_READ_LOW_US
#define DS18B20_READ_LOW_US          (6)
#endif
#ifndef DS18B20_READ_SAMPLE_US
#define DS18B20_READ_SAMPLE_US       (9)
#endif
#ifndef DS18B20_READ_FINISH_US
#define DS18B20_READ_FINISH_US       (55)
#endif

#define DS18B20_CMD_MATCH_ROM        (0x55)
#define DS18B20_CMD_SKIP_ROM         (0xcc)
#define DS18B20_CMD_CONVERT_T        (0x44)
#define DS18B20_CMD_WRITE_SCRATCHPAD (0x4e)
#define DS18B20_CMD_READ_SCRATCHPAD  (0xbe)
#define DS18B20_CMD_READ_POWER_SUPPLY (0xb4)

#define DS18B20_GPIO_GET(d)          ds18b20_gpio_get((d)->pin)
#define DS18B20_GPIO_RELEASE(d)      ds18b20_gpio_release((d)->pin)
#define DS18B20_GPIO_LOW(d)          ds18b20_gpio_drive_low((d)->pin)
#define DS18B20_GPIO_HIGH(d)         ds18b20_gpio_drive_high((d)->pin)
#define DS18B20_DELAY_US(us)         ds18b20_delay_us(us)

void ds18b20_init(ds18b20_t *dev, uint16_t pin, uint8_t strong_pullup)
{
    if (!dev)
        return;

    dev->pin = pin;
    dev->strong_pullup = strong_pullup ? 1 : 0;
    ds18b20_release_bus(dev);
}

void ds18b20_release_bus(const ds18b20_t *dev)
{
    if (!dev)
        return;

    DS18B20_GPIO_RELEASE(dev);
}

int ds18b20_reset(const ds18b20_t *dev)
{
    int presence;

    if (!dev)
        return ERR_DS18B20_ARG;

    DS18B20_GPIO_RELEASE(dev);
    DS18B20_DELAY_US(5);
    if (!DS18B20_GPIO_GET(dev))
        return ERR_DS18B20_BUS;

    DS18B20_GPIO_LOW(dev);
    DS18B20_DELAY_US(DS18B20_RESET_LOW_US);
    DS18B20_GPIO_RELEASE(dev);
    DS18B20_DELAY_US(DS18B20_RESET_SAMPLE_US);

    presence = DS18B20_GPIO_GET(dev) == 0;
    DS18B20_DELAY_US(DS18B20_RESET_FINISH_US);

    if (!presence)
        return ERR_DS18B20_NO_PRESENCE;

    if (!DS18B20_GPIO_GET(dev))
        return ERR_DS18B20_BUS;

    return 0;
}

static void ds18b20_write_bit(const ds18b20_t *dev, uint8_t bit)
{
    DS18B20_CRITICAL_ENTER();
    DS18B20_GPIO_LOW(dev);
    if (bit)
    {
        DS18B20_DELAY_US(DS18B20_WRITE_1_LOW_US);
        DS18B20_GPIO_RELEASE(dev);
        DS18B20_DELAY_US(DS18B20_WRITE_1_FINISH_US);
    }
    else
    {
        DS18B20_DELAY_US(DS18B20_WRITE_0_LOW_US);
        DS18B20_GPIO_RELEASE(dev);
        DS18B20_DELAY_US(DS18B20_WRITE_0_FINISH_US);
    }
    DS18B20_CRITICAL_EXIT();
}

static uint8_t ds18b20_read_bit(const ds18b20_t *dev)
{
    uint8_t bit;

    DS18B20_CRITICAL_ENTER();
    DS18B20_GPIO_LOW(dev);
    DS18B20_DELAY_US(DS18B20_READ_LOW_US);
    DS18B20_GPIO_RELEASE(dev);
    DS18B20_DELAY_US(DS18B20_READ_SAMPLE_US);
    bit = DS18B20_GPIO_GET(dev) ? 1 : 0;
    DS18B20_DELAY_US(DS18B20_READ_FINISH_US);
    DS18B20_CRITICAL_EXIT();

    return bit;
}

static void ds18b20_write_byte(const ds18b20_t *dev, uint8_t data)
{
    uint8_t bix;

    for (bix = 0; bix < 8; bix++)
    {
        ds18b20_write_bit(dev, data & 0x01);
        data >>= 1;
    }
}

static uint8_t ds18b20_read_byte(const ds18b20_t *dev)
{
    uint8_t bix;
    uint8_t data = 0;

    for (bix = 0; bix < 8; bix++)
    {
        data >>= 1;
        if (ds18b20_read_bit(dev))
            data |= 0x80;
    }

    return data;
}

static void ds18b20_select_rom(const ds18b20_t *dev, const uint8_t *rom)
{
    uint8_t i;

    if (rom)
    {
        ds18b20_write_byte(dev, DS18B20_CMD_MATCH_ROM);
        for (i = 0; i < DS18B20_ROM_SIZE; i++)
            ds18b20_write_byte(dev, rom[i]);
    }
    else
    {
        ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    }
}

int ds18b20_start_conversion(const ds18b20_t *dev, const uint8_t *rom)
{
    int res;

    if (!dev)
        return ERR_DS18B20_ARG;

    res = ds18b20_reset(dev);
    if (res)
        return res;

    ds18b20_select_rom(dev, rom);
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);

    if (dev->strong_pullup)
        DS18B20_GPIO_HIGH(dev);
    else
        DS18B20_GPIO_RELEASE(dev);

    return 0;
}

int ds18b20_conversion_done(const ds18b20_t *dev, uint8_t *done)
{
    if (!dev || !done)
        return ERR_DS18B20_ARG;

    *done = ds18b20_read_bit(dev) ? 1 : 0;

    return 0;
}

uint16_t ds18b20_conversion_time_ms(ds18b20_resolution_t resolution)
{
    switch (resolution)
    {
        case DS18B20_RESOLUTION_9BIT:
            return 94;
        case DS18B20_RESOLUTION_10BIT:
            return 188;
        case DS18B20_RESOLUTION_11BIT:
            return 375;
        case DS18B20_RESOLUTION_12BIT:
        default:
            return 750;
    }
}

int ds18b20_read_scratchpad(const ds18b20_t *dev, const uint8_t *rom, uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE])
{
    int res;
    uint8_t i;

    if (!dev || !scratchpad)
        return ERR_DS18B20_ARG;

    res = ds18b20_reset(dev);
    if (res)
        return res;

    ds18b20_select_rom(dev, rom);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    for (i = 0; i < DS18B20_SCRATCHPAD_SIZE; i++)
        scratchpad[i] = ds18b20_read_byte(dev);

    if (ds18b20_crc8(scratchpad, DS18B20_SCRATCHPAD_SIZE) != 0)
        return ERR_DS18B20_CRC;

    return 0;
}

int ds18b20_read_temp_raw(const ds18b20_t *dev, const uint8_t *rom, int16_t *raw)
{
    int res;
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];

    if (!raw)
        return ERR_DS18B20_ARG;

    res = ds18b20_read_scratchpad(dev, rom, scratchpad);
    if (res)
        return res;

    *raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);

    return 0;
}

int ds18b20_read_temp_mdeg(const ds18b20_t *dev, const uint8_t *rom, int32_t *mdeg)
{
    int res;
    int16_t raw;

    if (!mdeg)
        return ERR_DS18B20_ARG;

    res = ds18b20_read_temp_raw(dev, rom, &raw);
    if (res)
        return res;

    *mdeg = ds18b20_raw_to_mdeg(raw);

    return 0;
}

int ds18b20_set_resolution(const ds18b20_t *dev, const uint8_t *rom, ds18b20_resolution_t resolution)
{
    int res;
    uint8_t scratchpad[DS18B20_SCRATCHPAD_SIZE];

    if (!dev)
        return ERR_DS18B20_ARG;

    if (resolution != DS18B20_RESOLUTION_9BIT &&
        resolution != DS18B20_RESOLUTION_10BIT &&
        resolution != DS18B20_RESOLUTION_11BIT &&
        resolution != DS18B20_RESOLUTION_12BIT)
    {
        return ERR_DS18B20_ARG;
    }

    res = ds18b20_read_scratchpad(dev, rom, scratchpad);
    if (res)
        return res;

    res = ds18b20_reset(dev);
    if (res)
        return res;

    ds18b20_select_rom(dev, rom);
    ds18b20_write_byte(dev, DS18B20_CMD_WRITE_SCRATCHPAD);
    ds18b20_write_byte(dev, scratchpad[2]);
    ds18b20_write_byte(dev, scratchpad[3]);
    ds18b20_write_byte(dev, resolution);

    return 0;
}

int ds18b20_read_power_supply(const ds18b20_t *dev, const uint8_t *rom, uint8_t *externally_powered)
{
    int res;

    if (!dev || !externally_powered)
        return ERR_DS18B20_ARG;

    res = ds18b20_reset(dev);
    if (res)
        return res;

    ds18b20_select_rom(dev, rom);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_POWER_SUPPLY);
    *externally_powered = ds18b20_read_bit(dev) ? 1 : 0;

    return 0;
}

int32_t ds18b20_raw_to_mdeg(int16_t raw)
{
    int32_t mdeg = (int32_t)raw * 1000;

    if (mdeg >= 0)
        return (mdeg + 8) / 16;

    return (mdeg - 8) / 16;
}

uint8_t ds18b20_crc8_update(uint8_t crc, uint8_t data)
{
    uint8_t bix;

    for (bix = 0; bix < 8; bix++)
    {
        uint8_t mix = (crc ^ data) & 0x01;
        crc >>= 1;
        if (mix)
            crc ^= 0x8c;
        data >>= 1;
    }

    return crc;
}

uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;

    if (!data)
        return 0xff;

    while (len-- > 0)
        crc = ds18b20_crc8_update(crc, *data++);

    return crc;
}
