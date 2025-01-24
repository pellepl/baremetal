/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

/*
Safeguards:
x watchdog
x check that the thermocouple has been sampled from main thread
x check that the thermocouple sample has be read from thermo thread
x check that thermocouple works
x max temp
x max full power time
x max on time
on reboot, check if power to kiln was recently turned on - if so, probably power failure, short circuit or something
  - when kiln is started, set a flag in nvm which is cleared after ~5 seconds
  - if flag is set on reboot, probably main power safety switch
check that temp increases when power is on
verify thermocouple reading against control box reading when power is off
*/

#include "kiln.h"
#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "flash_driver.h"
#include "minio.h"
#include "cli.h"
#include "disp.h"
#include "touch.h"
#include "nvmtnv.h"
#include "flash_driver.h"
#include "timer.h"
#include "thermo.h"
#include "fonts/fonts.h"
#include "ui/ui.h"
#include "sys.h"
#include "settings.h"
#include "crystal_test.h"
#include "ringbuffer.h"
#include "assert.h"
#include "rtc.h"
#include "controller.h"
#include "dbg_kiln_model.h"

#define _str(x) __str(x)
#define __str(x) #x

static struct {
    enum {
        SCREEN_NONE = 0,
        SCREEN_GUI,
        SCREEN_SLEEP_INACTIVE,
        SCREEN_SLEEP_ACTIVE,
    } disp_screen;
    touch_sys_t touch;
} kiln;

static int cli_help(int argc, const char **argv) {
    int len = cli_entry_list_length();
    cli_entry_t **list = cli_entry_list_start();
    for (int i = 0; i < len; i++) {
        printf("\t%s\n", list[i]->name);
    }
    return 0;
}
CLI_FUNCTION(cli_help, "help", ": lists commands");

static int cli_reset(int argc, const char **argv) {
    sys_reset();
    return 0;
}
CLI_FUNCTION(cli_reset, "reset", ": resets target");

static int cli_hang(int argc, const char **argv) {
    while(1);
    return 0;
}
CLI_FUNCTION(cli_hang, "hang", ": hangs system, test watchdog");

static int cli_crash(int argc, const char **argv) {
    sys_force_dump();
    return 0;
}
CLI_FUNCTION(cli_crash, "crash", ": crashes system");

static void cli_cb(const char *func_name, int res) {
    switch (res) {
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

static void nvm_init(void) {
    uint8_t buf[strlen(_str(BUILD_INFO_TARGET_NAME))];
    int res;
    uint32_t sector, sector_cnt;
    flash_get_sectors_for_type(FLASH_TYPE_CODE_BANK0, &sector, &sector_cnt);
    res = nvmtnv_mount(sector+sector_cnt-5);
    for (int i = 0; i < 2; i++) {
        if (res >= 0) res = nvmtnv_fix();
        if (res >= 0) res = nvmtnv_read(SETTING_ID, buf, sizeof(buf));
        if (res >= 0) res = (memcmp(_str(BUILD_INFO_TARGET_NAME), buf, sizeof(buf)) ? -1 : 0);
        if (i == 0 && res < 0) {
            printf("NVM: cannot validate storage, formatting @ sector %08x\n", sector+sector_cnt-5);
            res = nvmtnv_format(sector+sector_cnt-5, 5, flash_get_sector_size(sector)/32);
            if (res >= 0) res = nvmtnv_mount(sector+sector_cnt-5);
            if (res >= 0) res = nvmtnv_write(SETTING_ID, (const uint8_t *)_str(BUILD_INFO_TARGET_NAME), strlen(_str(BUILD_INFO_TARGET_NAME)));
            if (res < 0) printf("NVM: WARNING: could not format storage, %d\n", res);
        }
    }
    if (res >= 0) {
        printf("NVM ok\n");
    }  else {
        printf("NVM failed, %d\n", res);
    }
}

static void display_init(void) {
    kiln.touch.needs_calibration = 0;
    disp_init();
    if (gpio_read(BOARD_BUTTON_GPIO_PIN[0]) == BOARD_BUTTON_GPIO_ACTIVE[0]) {
        printf("DISP force calibration\n");
        kiln.touch.needs_calibration = 1;
    }
    if (nvmtnv_read(SETTING_TOUCH_CALIB, (uint8_t *)&kiln.touch.limits, sizeof(kiln.touch.limits)) < 0) {
        printf("DISP not calibrated\n");
        kiln.touch.needs_calibration = 1;
    }
    disp_clear(0x000080);
}

#define DISP_CALIB_CURSOR_SCREEN_PART          6
static void disp_calibration(uint32_t tick) {
    uint16_t dw = disp_width();
    uint16_t dh = disp_height();
    uint16_t dmin = dw < dh ? dw : dh;
    uint32_t dtick = tick & 0x3ffff;
    uint16_t x,y,wh,offs;
    offs = dtick < 0x20000 ? dmin/20: 0;
    x = y = 1;

    wh = dmin/DISP_CALIB_CURSOR_SCREEN_PART - offs*2;    
    if (kiln.touch.needs_calibration == 1) {
        x = y = offs;
    } else if (kiln.touch.needs_calibration == 2) {
        x = dw - wh - offs;
        y = dh - wh - offs;
    } else if (kiln.touch.needs_calibration == 3) {
        x = offs;
        y = dh - wh - offs;
    } else if (kiln.touch.needs_calibration == 4) {
        x = dw - wh - offs;
        y = offs;
    } else if (kiln.touch.needs_calibration == 5) {
        x = (dw - wh) / 2;
        y = (dh - wh) / 2;
    } else {
        kiln.touch.needs_calibration = 0;
    }
    if (kiln.touch.needs_calibration) {
        if (dtick == 0 || dtick == 0x20000) {
            disp_clear(0x80);
            disp_fill(x,y,wh,wh,0xffff66);
        }
    } else {
        uint16_t minx[2];   
        uint16_t maxx[2];
        uint16_t miny[2];
        uint16_t maxy[2];
        minx[0] = kiln.touch.calib[0].x;
        minx[1] = kiln.touch.calib[2].x;
        maxx[0] = kiln.touch.calib[1].x;
        maxx[1] = kiln.touch.calib[3].x;
        miny[0] = kiln.touch.calib[0].y;
        miny[1] = kiln.touch.calib[3].y;
        maxy[0] = kiln.touch.calib[1].y;
        maxy[1] = kiln.touch.calib[2].y;

        // pick extreme values
        kiln.touch.limits.minx = minx[0] < minx[1] ? minx[0] : minx[1];
        kiln.touch.limits.maxx = maxx[0] > maxx[1] ? maxx[0] : maxx[1];
        kiln.touch.limits.miny = miny[0] < miny[1] ? miny[0] : miny[1];
        kiln.touch.limits.maxy = maxy[0] > maxy[1] ? maxy[0] : maxy[1];
        // get error for midpoint
        int errx = (kiln.touch.limits.maxx + kiln.touch.limits.minx)/2 - kiln.touch.calib[4].x;
        int erry = (kiln.touch.limits.maxy + kiln.touch.limits.miny)/2 - kiln.touch.calib[4].y;
        // adjust to midpoint
        kiln.touch.limits.minx -= errx/2;
        kiln.touch.limits.maxx -= errx/2;
        kiln.touch.limits.miny -= erry/2;
        kiln.touch.limits.maxy -= erry/2;

        // bloat DISP_CALIB_CURSOR_SCREEN_PART parts
        int dt = dw < dh ? (kiln.touch.limits.maxx - kiln.touch.limits.minx) : (kiln.touch.limits.maxy - kiln.touch.limits.miny);
        dt /= DISP_CALIB_CURSOR_SCREEN_PART;
        dt /= 2;
        if (dt < 0) dt = -dt;
        if (kiln.touch.limits.minx < kiln.touch.limits.maxx) {
            kiln.touch.limits.minx -= dt;
            kiln.touch.limits.maxx += dt;
        } else {
            kiln.touch.limits.minx += dt;
            kiln.touch.limits.maxx -= dt;
        }
        if (kiln.touch.limits.miny < kiln.touch.limits.maxy) {
            kiln.touch.limits.miny -= dt;
            kiln.touch.limits.maxy += dt;
        } else {
            kiln.touch.limits.miny += dt;
            kiln.touch.limits.maxy -= dt;
        }
        printf("xraw: %d..%d  yraw: %d..%d\n", kiln.touch.limits.minx, kiln.touch.limits.maxx, kiln.touch.limits.miny, kiln.touch.limits.maxy);
        printf("DISP calibrated\n");
        nvmtnv_write(SETTING_TOUCH_CALIB, (uint8_t *)&kiln.touch.limits, sizeof(kiln.touch.limits));
        disp_clear(0x0);
    }
}

// CONFIG_UART_STM32_RX_INTERRUPT
static ringbuffer_t uart_ringbuffer;
static uint8_t uart_rx[512];
void uart_irq_rxchar_stm32f1(unsigned int hdl, char x);
void uart_irq_rxchar_stm32f1(unsigned int hdl, char x) {
    ringbuffer_putc(&uart_ringbuffer, (uint8_t)x);
}
// CONFIG_UART_STM32_RX_INTERRUPT

static ui_event_t event_second = {
    .type = EVENT_SECOND,
    .is_static = 1
};
static uint32_t rtc_second = 0;
static uint64_t rtc_alarm_tick;
static void rtc_alarm_cb(void) {
    event_second.time = rtc_second;
    rtc_alarm_tick += RTC_S_TO_TICK(1);
    rtc_set_alarm_tick(rtc_alarm_tick);
    ui_post_event(&event_second);
    rtc_second++;
}

extern uint32_t _estack[];
extern uint32_t __nobss_end[];
static uint32_t ___used_stack;

uint32_t __get_used_stack(void) {
    return ___used_stack;
}

int main(void) {
    cpu_init();
    for (uint32_t i = 0; i <  (uint32_t)_estack - (uint32_t)__nobss_end; i++) {
        if (((uint8_t *)__nobss_end)[i] != 0xa5) {
            ___used_stack = ((uint32_t)_estack - (uint32_t)__nobss_end) - i;
            break;
        }
    }
    memset((uint8_t *)__nobss_end, 0xa5, (uint32_t)_estack - (uint32_t)__nobss_end);
    board_init();
    gpio_init();
    for (int i = 0; i < BOARD_LED_COUNT; i++) {
        gpio_config(BOARD_LED_GPIO_PIN[i], GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
        gpio_set(BOARD_LED_GPIO_PIN[i], !BOARD_LED_GPIO_ACTIVE[i]);
    }
    for (int i = 0; i < BOARD_BUTTON_COUNT; i++) {
        gpio_config(BOARD_BUTTON_GPIO_PIN[i], GPIO_DIRECTION_INPUT, GPIO_PULL_UP);
    }
    gpio_config(BOARD_BUZZER_PIN, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    flash_init();
    ringbuffer_init(&uart_ringbuffer, uart_rx, sizeof(uart_rx));
    uart_config_t uart_cfg = {
        .baudrate = UART_BAUDRATE,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE
    };
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


    // TODO test the LFCLK crystal as this is the one keeping kiln time
    //crystal_test(LFCLK_OSC_XTAL);

    nvm_init();

    rtc_init(rtc_alarm_cb);
    rtc_alarm_tick = rtc_get_tick() + RTC_S_TO_TICK(1);
    rtc_set_alarm_tick(rtc_alarm_tick);
    sys_watchdog_start(2000);
    timer_init();
    settings_init();
    model_init();
    ctrl_init();
    thermoc_init();
    spi_sw_t spi_cfg = {
        .mosi_pin = PIN_TOUCH_MOSI,
        .miso_pin = PIN_TOUCH_MISO,
        .clk_pin = PIN_TOUCH_CLK,
        .delay = 0
    };
    touch_init(0, &spi_cfg, PIN_TOUCH_CS, PIN_TOUCH_IRQ, &kiln.touch);
    display_init();

    uint32_t tick = 0;
    while(1) {
        uint8_t *rx;
        int rx_len;
        if ((rx_len = ringbuffer_available_linear(&uart_ringbuffer, &rx)) > 0) {
            cli_parse((char *)rx, rx_len);
            ringbuffer_get(&uart_ringbuffer, 0, rx_len);
        }
        tick_t dt;
        if ((dt = thermoc_ticks_since_sample()) > (tick_t)TIMER_MS_TO_TICKS(4000)) {
            // make sure the thermocouple is sampled at least each 4000th ms
            printf("main dt %08x%08x\n", (uint32_t)(dt>>32), (uint32_t)dt);
            ASSERT(0);
        }
        touch_process();
        if (kiln.touch.needs_calibration) disp_calibration(tick);
        else if (kiln.disp_screen == SCREEN_NONE) {
            kiln.disp_screen = SCREEN_GUI;
            gui_init();
        } else {
            gui_loop();
        }
        tick++;
        sys_watchdog_feed();
    }
}

void app_shutdown_from_hardfault(void);
void app_shutdown_from_hardfault(void) {
    // shut off kiln 
    ctrl_elements_off();
    // disable buzzer
    gui_silence();
}

void app_shutdown_from_hardfault_done(void);
void app_shutdown_from_hardfault_done(void) {
    for (int i = 0; i < BOARD_LED_COUNT; i++) {
        gpio_set(BOARD_LED_GPIO_PIN[i], BOARD_LED_GPIO_ACTIVE[i]);
    }
}
