#include <stddef.h>
#include "minikiln.h"
#include "board.h"
#include "settings.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "flash_driver.h"
#include "events.h"
#include "minio.h"
#include "cli.h"
#include "nvmtnv.h"
#include "flash_driver.h"
#include "timer.h"
#include "ringbuffer.h"
#include "assert.h"
#include "rtc.h"
#include "gfx.h"
#include "ui.h"
#include "input.h"

#define _str(x) __str(x)
#define __str(x) #x

static int cli_help(int argc, const char **argv)
{
    int len = cli_entry_list_length();
    cli_entry_t **list = cli_entry_list_start();
    for (int i = 0; i < len; i++)
    {
        printf("\t%s\n", list[i]->name);
    }
    return 0;
}
CLI_FUNCTION(cli_help, "help", ": lists commands");

static int cli_reset(int argc, const char **argv)
{
    sys_reset();
    return 0;
}
CLI_FUNCTION(cli_reset, "reset", ": resets target");

static int cli_hang(int argc, const char **argv)
{
    while (1)
        ;
    return 0;
}
CLI_FUNCTION(cli_hang, "hang", ": hangs system, test watchdog");

static int cli_crash(int argc, const char **argv)
{
    sys_force_dump();
    return 0;
}
CLI_FUNCTION(cli_crash, "crash", ": crashes system");

static void cli_cb(const char *func_name, int res)
{
    switch (res)
    {
    case 0:
        printf("OK\n");
        break;
    case ERR_CLI_NO_ENTRY:
        cli_help(0, (void *)0);
        printf("ERROR \"%s\" not found\n", func_name);
        break;
    case ERR_CLI_INPUT_OVERFLOW:
        printf("ERROR too long input\n");
        break;
    case ERR_CLI_ARGUMENT_OVERFLOW:
        printf("ERROR too many arguments\n");
        break;
    case ERR_CLI_SILENT:
        break;
    default:
        printf("ERROR (%d)\n", res);
        break;
    }
}

static void nvm_init(void)
{
    uint8_t buf[strlen(_str(BUILD_INFO_TARGET_NAME))];
    int res;
    uint32_t sector, sector_cnt;
    flash_get_sectors_for_type(FLASH_TYPE_CODE_BANK0, &sector, &sector_cnt);
    res = nvmtnv_mount(sector + sector_cnt - 5);
    for (int i = 0; i < 2; i++)
    {
        if (res >= 0)
            res = nvmtnv_fix();
        if (res >= 0)
            res = nvmtnv_read(0, buf, sizeof(buf));
        if (res >= 0)
            res = (memcmp(_str(BUILD_INFO_TARGET_NAME), buf, sizeof(buf)) ? -1 : 0);
        if (i == 0 && res < 0)
        {
            printf("NVM: cannot validate storage, formatting @ sector %08x\n", sector + sector_cnt - 5);
            res = nvmtnv_format(sector + sector_cnt - 5, 5, flash_get_sector_size(sector) / 32);
            if (res >= 0)
                res = nvmtnv_mount(sector + sector_cnt - 5);
            if (res >= 0)
                res = nvmtnv_write(0, (const uint8_t *)_str(BUILD_INFO_TARGET_NAME), strlen(_str(BUILD_INFO_TARGET_NAME)));
            if (res < 0)
                printf("NVM: WARNING: could not format storage, %d\n", res);
        }
    }
    if (res >= 0)
    {
        printf("NVM ok\n");
    }
    else
    {
        printf("NVM failed, %d\n", res);
    }
}

// CONFIG_UART_STM32_RX_INTERRUPT
static ringbuffer_t uart_ringbuffer;
static uint8_t uart_rx[512];
void uart_irq_rxchar_stm32f1(unsigned int hdl, char x);
void uart_irq_rxchar_stm32f1(unsigned int hdl, char x)
{
    ringbuffer_putc(&uart_ringbuffer, (uint8_t)x);
}
// CONFIG_UART_STM32_RX_INTERRUPT

static uint32_t sys_up_second = 0;

static uint64_t rtc_alarm_tick;
static void rtc_alarm_cb(void)
{
    static event_t second_event;
    rtc_alarm_tick += RTC_S_TO_TICK(1);
    rtc_set_alarm_tick(rtc_alarm_tick);
    sys_up_second++;
    event_add(&second_event, EVENT_SECOND_TICK, (void *)sys_get_up_second());
}

extern uint32_t _estack[];
extern uint32_t __nobss_end[];
static uint32_t ___used_stack;

uint32_t sys_get_up_second(void) {
    return sys_up_second;
}

uint32_t __get_used_stack(void)
{
    return ___used_stack;
}

static void event_handler(uint32_t event, void *arg)
{
    //printf("ev %d\t%08x\n", event, arg);
    const event_func_t *ev_fns = EVENT_HANDLERS_START;
    const event_func_t *ev_fns_end = EVENT_HANDLERS_END;
    while (ev_fns < ev_fns_end) {
        ((event_func_t)(*ev_fns))(event, arg);
        ev_fns++;
    }
}

static event_t event_scroll;

int main(void)
{
    cpu_init();
    for (uint32_t i = 0; i < (uint32_t)_estack - (uint32_t)__nobss_end; i++)
    {
        if (((uint8_t *)__nobss_end)[i] != 0xa5)
        {
            ___used_stack = ((uint32_t)_estack - (uint32_t)__nobss_end) - i;
            break;
        }
    }
    memset((uint8_t *)__nobss_end, 0xa5, (uint32_t)_estack - (uint32_t)__nobss_end);
    board_init();
    gpio_init();
    flash_init();
    ringbuffer_init(&uart_ringbuffer, uart_rx, sizeof(uart_rx));
    disp_init();
    uart_config_t uart_cfg = {
        .baudrate = UART_BAUDRATE,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE};
    uart_init(0, &uart_cfg);

    printf("\n%s    [%s %s on %s]\n%s/%s/%s@%s\n%s@%s %s-%s %s\n%s %s\nmain freq: %d MHz\n",
           _str(BUILD_INFO_TARGET_NAME),
           _str(BUILD_INFO_GIT_COMMIT), _str(BUILD_INFO_GIT_TAG), _str(BUILD_INFO_GIT_BRANCH),
           _str(BUILD_INFO_TARGET_ARCH), _str(BUILD_INFO_TARGET_FAMILY), _str(BUILD_INFO_TARGET_PROC),
           _str(BUILD_INFO_TARGET_BOARD),
           _str(BUILD_INFO_HOST_WHO), _str(BUILD_INFO_HOST_NAME),
           _str(BUILD_INFO_HOST_WHEN_DATE), _str(BUILD_INFO_HOST_WHEN_TIME),
           _str(BUILD_INFO_HOST_WHEN_EPOCH),
           _str(BUILD_INFO_CC_MACHINE),
           _str(BUILD_INFO_CC_VERSION),
           cpu_core_clock_freq());

    sys_init();

    cli_init(cli_cb, "\r\n;", " ,", "", "");

    nvm_init();
    settings_init();
    rtc_init(rtc_alarm_cb);
    rtc_alarm_tick = rtc_get_tick() + RTC_S_TO_TICK(1);
    rtc_set_alarm_tick(rtc_alarm_tick);
    sys_watchdog_start(2000);
    timer_init();
    input_init();
    event_init(event_handler);
    gfx_init();
    disp_set_enabled(true, NULL);
    ui_init();
    ui_trigger_update();
    uint32_t tick = 0;
    while (1)
    {
        uint8_t *rx;
        int rx_len;
        if ((rx_len = ringbuffer_available_linear(&uart_ringbuffer, &rx)) > 0)
        {
            cli_parse((char *)rx, rx_len);
            ringbuffer_get(&uart_ringbuffer, 0, rx_len);
        }

        static int16_t rotation_prev = 0;
        static int acc_rot = 0;
        int16_t rotation = input_rot_read();
        acc_rot += rotation - rotation_prev;
        rotation_prev = rotation;
        if (acc_rot / INPUT_ROTARY_DIVISOR != 0) {
            event_add(&event_scroll, EVENT_UI_SCRL, (void *)(int)(acc_rot / INPUT_ROTARY_DIVISOR));
            acc_rot -= INPUT_ROTARY_DIVISOR * (acc_rot / INPUT_ROTARY_DIVISOR);
        }

        while (event_execute_one())
            ;

        tick++;
        sys_watchdog_feed();
    }
}

void app_shutdown_from_hardfault(void);
void app_shutdown_from_hardfault(void)
{
}

void app_shutdown_from_hardfault_done(void);
void app_shutdown_from_hardfault_done(void)
{
    for (int i = 0; i < BOARD_LED_COUNT; i++)
    {
        gpio_set(BOARD_LED_GPIO_PIN[i], BOARD_LED_GPIO_ACTIVE[i]);
    }
}
