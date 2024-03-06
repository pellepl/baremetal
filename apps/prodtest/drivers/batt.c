#include "batt.h"
#include "targets.h"
#include "gpio_driver.h"

#define BATT_CHANNEL 0
#define ADC_TO_MILLIVOLT(__x) (uint16_t)((__x) * 600 * 6 / 1024)

int batt_single_measurement(bool load, int16_t *millivolt)
{
    int err;
    const target_t *t = target_get();

    if (millivolt == NULL) {
        return -EINVAL;
    }

    NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_10bit <<  SAADC_RESOLUTION_VAL_Pos;
    NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass << SAADC_OVERSAMPLE_OVERSAMPLE_Pos;
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;
    
    NRF_SAADC->CH[BATT_CHANNEL].CONFIG = 
        (SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos) |
        (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos) |
        (SAADC_CH_CONFIG_TACQ_10us << SAADC_CH_CONFIG_TACQ_Pos) |
        (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
        (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos) |
        (SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos) |
        (SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos);
    NRF_SAADC->CH[BATT_CHANNEL].PSELN = SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos;
    if (t->battery.routed && t->battery.pin_measure != BOARD_PIN_UNDEF) {
        gpio_config(t->battery.pin_measure, GPIO_DIRECTION_ANALOG, GPIO_PULL_NONE);
        NRF_SAADC->CH[BATT_CHANNEL].PSELP = t->battery.pin_measure << SAADC_CH_PSELP_PSELP_Pos;
    } else {
        NRF_SAADC->CH[BATT_CHANNEL].PSELP = SAADC_CH_PSELP_PSELP_VDD << SAADC_CH_PSELP_PSELP_Pos;
    }

    if (t->battery.routed && t->battery.pin_measure_enable != BOARD_PIN_UNDEF) {
        gpio_set(t->battery.pin_measure_enable, t->battery.pin_measure_enable_active_high ? 1 : 0);
        gpio_config(t->battery.pin_measure_enable, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }

    if (load && t->battery.routed && t->battery.pin_load != BOARD_PIN_UNDEF) {
        gpio_set(t->battery.pin_load, t->battery.pin_measure_enable_active_high ? 1 : 0);
        gpio_config(t->battery.pin_load, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
        cpu_halt(t->battery.load_delay_ms);
    }

    int16_t spl;

    NRF_SAADC->RESULT.PTR = (uint32_t)&spl;
    NRF_SAADC->RESULT.MAXCNT = 1;

    NRF_SAADC->EVENTS_RESULTDONE = 0;
    NRF_SAADC->TASKS_START = 1;
    NRF_SAADC->TASKS_SAMPLE = 1;
    volatile int spoonguard = cpu_core_clock_freq() / 8;
    while (NRF_SAADC->EVENTS_RESULTDONE == 0 && --spoonguard);
    NRF_SAADC->TASKS_STOP = 1;

    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos;

    if (t->battery.routed)
    {
        if (load && t->battery.pin_load != BOARD_PIN_UNDEF)
        {
            gpio_set(t->battery.pin_load, t->battery.pin_measure_enable_active_high ? 0 : 1);
        }

        if (t->battery.pin_measure_enable != BOARD_PIN_UNDEF)
        {
            gpio_set(t->battery.pin_measure_enable, t->battery.pin_measure_enable_active_high ? 0 : 1);
        }

        if (t->battery.pin_load != BOARD_PIN_UNDEF)
        {
            target_reset_pin(t->battery.pin_load);
        }

        if (t->battery.pin_measure_enable != BOARD_PIN_UNDEF)
        {
            target_reset_pin(t->battery.pin_measure_enable);
        }

        if (t->battery.pin_measure != BOARD_PIN_UNDEF)
        {
            target_reset_pin(t->battery.pin_measure);
        }
    }
    err = spoonguard == 0 ? -ETIME : ERROR_OK;

    if (err == ERROR_OK) {
        *millivolt = ADC_TO_MILLIVOLT(spl);
    }

    return err;
}

#if NO_EXTRA_CLI == 0

#include "cli.h"
#include "minio.h"

static int cli_batt_vdd(int argc, const char **argv) {
    int16_t mv;
    int err = batt_single_measurement(false, &mv);
    printf("BATT: %d mV\r\n", mv);
    return err;
}
CLI_FUNCTION(cli_batt_vdd, "batt_vdd", "Sample VDD");

#endif
