#include "board.h"
#include "events.h"
#include "thermo.h"
#include "minio.h"
#include "timer.h"
#include "spi_sw.h"
#include "gpio_driver.h"
#include "assert.h"
#include "cli.h"

static struct
{
    timer_t thermo_timer;
    volatile int16_t temp;
    volatile int temp_junction;
    volatile int temp_is_new;
    volatile uint8_t temp_err;
    volatile tick_t last_therm_spled;
    volatile tick_t last_therm_read;
    event_t thermo_event;
    int thermo_multi;
    int thermo_offs;
} me = {
    .thermo_multi = 1,
};

static void thermo_timer_cb(timer_t *t)
{
#if defined(MAX6675)
    uint8_t buf[2];
    gpio_set(BOARD_PIN_THERMOC_CS, 0);
    spi_sw_txrx(1, (void *)0, buf, 2);
    gpio_set(BOARD_PIN_THERMOC_CS, 1);
    uint16_t v = (buf[0] << 8) | buf[1];
    me.temp_err = (((v & 0b100) != 0) || ((v & 0b10) == 0)) ? 0x80;
    me.temp_junction = 0;
    if (!me.thermo_error)
    {
        me.temp = v >> 5;
    }
#elif defined(MAX31855)
    uint8_t buf[4];
    gpio_set(BOARD_PIN_THERMOC_CS, 0);
    spi_sw_txrx(1, (void *)0, buf, 4);
    gpio_set(BOARD_PIN_THERMOC_CS, 1);
    int16_t v = (buf[0] << 8) | buf[1];
    me.temp_err = (buf[1] & 1) ? (buf[3] & 0b111) | 0x80 : 0;
    if (!me.temp_err)
    {
        me.temp = v >> (2 + 2);
        me.temp_junction = ((buf[2] << 8) | buf[3]) >> 8;
    }
#endif
    tick_t now = timer_now();
    me.last_therm_spled = now;

    me.temp_is_new = 1;

    event_add(&me.thermo_event, EVENT_THERMO, (void *)(me.temp * me.thermo_multi + me.thermo_offs));
    tick_t dt;
    if (now < me.last_therm_read)
    {
        now = timer_now();
    }

    if ((dt = now - me.last_therm_read) > (tick_t)TIMER_MS_TO_TICKS(4000))
    {
        // make sure someone reads the temperature at least each 4000th ms
        ASSERT(0);
    }
}

void thermoc_init(void)
{
    gpio_config(BOARD_PIN_THERMOC_CLK, GPIO_DIRECTION_OUTPUT, GPIO_PULL_UP);
    gpio_config(BOARD_PIN_THERMOC_CS, GPIO_DIRECTION_OUTPUT, GPIO_PULL_UP);
    gpio_config(BOARD_PIN_THERMOC_MISO, GPIO_DIRECTION_INPUT, GPIO_PULL_UP);
    gpio_set(BOARD_PIN_THERMOC_CS, 1);
    spi_sw_init(1, SPI_MODE_3,
                SPI_TRANSMISSION_MSB_FIRST, SPI_TRANSMISSION_MSB_FIRST,
                -1, BOARD_PIN_THERMOC_MISO, BOARD_PIN_THERMOC_CLK, 0);
    timer_start(&me.thermo_timer, thermo_timer_cb, TIMER_MS_TO_TICKS(THERMOCOUPLE_SAMPLE_PERIOD_MS), TIMER_REPETITIVE);
    me.thermo_multi = 1;
    me.thermo_offs = 0;
    me.last_therm_spled = timer_now();
}

tick_t thermoc_ticks_since_sample(void)
{
    tick_t spl = me.last_therm_spled;
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
    me.last_therm_read = timer_now();
    r->is_new = me.temp_is_new;
    me.temp_is_new = 0;
    r->temperature = me.temp * me.thermo_multi + me.thermo_offs;
    r->temperature_junction = me.temp_junction;
    r->error = me.temp_err;
}

void thermoc_set_multiplier(int m)
{
    me.thermo_multi = m;
}
void thermoc_set_offset(int a)
{
    me.thermo_offs = a;
}

static int cli_thermo_set_multi(int argc, const char **argv)
{
    thermoc_set_multiplier(atoi(argv[0]));
    printf("thermo multiplier %d\n", me.thermo_multi);
    return 0;
}
CLI_FUNCTION(cli_thermo_set_multi, "tmult", "<mul>: set thermo multiplier");
static int cli_thermo_set_offs(int argc, const char **argv)
{
    thermoc_set_offset(atoi(argv[0]));
    printf("thermo offset %d\n", me.thermo_offs);
    return 0;
}
CLI_FUNCTION(cli_thermo_set_offs, "toffs", "<offs>: set thermo offset");
