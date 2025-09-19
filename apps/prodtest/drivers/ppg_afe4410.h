#pragma once
#include <stdbool.h>

typedef union
{
    uint32_t raw;
    struct __attribute__((packed))
    {
        uint8_t reg : 8;
        union
        {
            uint32_t uval : 24;
            int32_t sval : 24;
        };
    };
} afe4410_reg_t;

typedef struct
{
    bool early_offset;
    int8_t led1;
    int8_t amb1;
    int8_t led2;
    int8_t amb2;
    enum
    {
        DAC_X1 = 0b000, // +-15.875uA
        DAC_X2 = 0b011, // +-31.75uA
        DAC_X4 = 0b101, // +-63.5uA
        DAC_X8 = 0b111  // +-127uA
    } multiplier;
} afe4410_dac_t;

typedef struct
{
    uint8_t iled1;
    uint8_t iled2;
    uint8_t iled3;
} afe4410_led_currents_t;

typedef struct
{
    int32_t led1;
    union
    {
        int32_t led1_ambient;
        int32_t led4;
    };
    int led2;
    union
    {
        int32_t led2_ambient;
        int32_t led3;
    };
    int32_t avg_delta_led_amb_1;
    int32_t avg_delta_led_amb_2;
} afe4410_adc_values_t;

int ppg_afe4410_init(void);
int ppg_afe4410_setup(void);
void ppg_afe4410_deinit(void);
int ppg_afe4410_power(bool enable);
int ppg_afe4410_reset(bool enable);
int ppg_afe4410_write(const afe4410_reg_t *reg);
int ppg_afe4410_read(afe4410_reg_t *reg);
int ppg_afe4410_write_dac(const afe4410_dac_t *dac);
int ppg_afe4410_read_dac(afe4410_dac_t *dac);
int ppg_afe4410_write_iled(const afe4410_led_currents_t *led);
int ppg_afe4410_read_iled(afe4410_led_currents_t *led);
int ppg_afe4410_read_adc(afe4410_adc_values_t *adc);
