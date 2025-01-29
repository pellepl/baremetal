#include "thermo.h"
#include "minio.h"
#include "timer.h"
#include "spi_sw.h"
#include "gpio_driver.h"
#include "kiln.h"
#include "assert.h"
#include "ui/ui.h"
#include "cli.h"
#include "settings.h"
#include "dbg_kiln_model.h"

static timer_t thermo_timer;

static volatile int16_t temp;
static volatile int temp_junction;
static volatile int temp_is_new;
static volatile uint8_t temp_err;

static volatile tick_t last_therm_spled;
static volatile tick_t last_therm_read;
static ui_event_t thermo_event;
static int thermo_multi = 1;
static int thermo_offs = 0;

static void thermo_timer_cb(timer_t *t)
{
#if defined(MAX6675)
    uint8_t buf[2];
    gpio_set(PIN_THERMOC_CS, 0);
    spi_sw_txrx(1, (void *)0, buf, 2);
    gpio_set(PIN_THERMOC_CS, 1);
    uint16_t v = (buf[0] << 8) | buf[1];
    temp_err = (((v & 0b100) != 0) || ((v & 0b10) == 0)) ? 0x80;
    temp_junction = 0;
    if (!thermo_error)
    {
        temp = v >> 5;
    }
#elif defined(MAX31855)
    uint8_t buf[4];
    gpio_set(PIN_THERMOC_CS, 0);
    spi_sw_txrx(1, (void *)0, buf, 4);
    gpio_set(PIN_THERMOC_CS, 1);
    int16_t v = (buf[0] << 8) | buf[1];
    temp_err = (buf[1] & 1) ? (buf[3] & 0b111) | 0x80 : 0;
    if (!temp_err)
    {
        temp = v >> (2 + 2);
        temp_junction = ((buf[2] << 8) | buf[3]) >> 8;
    }
#endif
    tick_t now = timer_now();
    last_therm_spled = now;
    if (settings_get_val_for_id(SETTING_KILN_SIMULATION))
    {
        temp = model_temp();
        temp_err = 0;
    }

    temp_is_new = 1;

    thermo_event.value = temp * thermo_multi + thermo_offs;
    thermo_event.user = (void *)(intptr_t)temp_err;
    ui_post_event(&thermo_event);
    tick_t dt;
    if (now < last_therm_read)
    {
        //        printf("ther now %08x%08x\n", (uint32_t)(now>>32), (uint32_t)now);
        //        printf("ther ltr %08x%08x\n", (uint32_t)(last_therm_read>>32), (uint32_t)last_therm_read);
        now = timer_now();
        printf("ther now'%08x%08x\n", (uint32_t)(now >> 32), (uint32_t)now);
    }

    if ((dt = now - last_therm_read) > (tick_t)TIMER_MS_TO_TICKS(4000))
    {
        // make sure someone reads the temperature at least each 4000th ms
        //        printf("ther dt %08x%08x\n", (uint32_t)(dt>>32), (uint32_t)dt);
        ASSERT(0);
    }
}

void thermoc_init(void)
{
    gpio_config(PIN_THERMOC_CLK, GPIO_DIRECTION_OUTPUT, GPIO_PULL_UP);
    gpio_config(PIN_THERMOC_CS, GPIO_DIRECTION_OUTPUT, GPIO_PULL_UP);
    gpio_config(PIN_THERMOC_MISO, GPIO_DIRECTION_INPUT, GPIO_PULL_UP);
    gpio_set(PIN_THERMOC_CS, 1);
    spi_sw_init(1, SPI_MODE_3,
                SPI_TRANSMISSION_MSB_FIRST, SPI_TRANSMISSION_MSB_FIRST,
                -1, PIN_THERMOC_MISO, PIN_THERMOC_CLK, 0);
    timer_start(&thermo_timer, thermo_timer_cb, TIMER_MS_TO_TICKS(THERMOCOUPLE_SAMPLE_PERIOD_MS), TIMER_REPETITIVE);
    thermo_event.type = EVENT_THERMO;
    thermo_event.is_static = 1;
    thermo_multi = 1;
    thermo_offs = 0;
}

tick_t thermoc_ticks_since_sample(void)
{
    tick_t spl = last_therm_spled;
    tick_t now = timer_now();
    if (now < spl)
    {
        // got a sample while executing this function, get a new now again
        now = timer_now();
    }
    return now - spl;
}

void thermoc_temp(thermo_result_t *r)
{
    ASSERT(r);
    last_therm_read = timer_now();
    r->is_new = temp_is_new;
    temp_is_new = 0;
    r->temperature = temp * thermo_multi + thermo_offs;
    r->temperature_junction = temp_junction;
    r->error = temp_err;
}

void thermoc_set_multiplier(int m)
{
    thermo_multi = m;
}
void thermoc_set_offset(int a)
{
    thermo_offs = a;
}

static int cli_thermo_set_multi(int argc, const char **argv)
{
    thermoc_set_multiplier(atoi(argv[0]));
    printf("thermo multiplier %d\n", thermo_multi);
    return 0;
}
CLI_FUNCTION(cli_thermo_set_multi, "tmult", "<mul>: set thermo multiplier");
static int cli_thermo_set_offs(int argc, const char **argv)
{
    thermoc_set_offset(atoi(argv[0]));
    printf("thermo offset %d\n", thermo_offs);
    return 0;
}
CLI_FUNCTION(cli_thermo_set_offs, "toffs", "<offs>: set thermo offset");
