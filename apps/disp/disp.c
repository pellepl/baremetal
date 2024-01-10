/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include <stdarg.h>

#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "minio.h"
#include "cli.h"

#include "disp.h"

#include "nrf52840.h"

static void cli_cb(const char *func_name, int res);

static struct {
    volatile uint32_t running;
    volatile uint32_t txed;
} spi_status;

const disp_cfg_t *disp_get_cfg(void)
{
    static const disp_cfg_t cfg = {
        .pins.dc = BOARD_DISP_DATACOM_PIN,
        .pins.resetn = BOARD_DISP_RESETN_PIN,
        .pins.spi_csn = BOARD_DISP_SPI_CSN_PIN,
        .pins.spi_mosi = BOARD_DISP_SPI_MOSI_PIN,
        .pins.spi_sck = BOARD_DISP_SPI_SCK_PIN,
        .pins.tearing = BOARD_DISP_TEARING_PIN};
    return &cfg;
}

static void disp_ifc_init(void)
{
    const disp_cfg_t *cfg = disp_get_cfg();

    gpio_set(cfg->pins.resetn, 0);
    gpio_set(cfg->pins.spi_csn, 1);
    gpio_config(cfg->pins.resetn, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.spi_csn, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.dc, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.spi_mosi, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.spi_sck, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.tearing, GPIO_DIRECTION_INPUT, GPIO_PULL_UP);

    BOARD_DISP_SPI_BUS->PSEL.SCK = cfg->pins.spi_sck;
    BOARD_DISP_SPI_BUS->PSEL.MOSI = cfg->pins.spi_mosi;
    BOARD_DISP_SPI_BUS->PSEL.CSN = -1;
    BOARD_DISP_SPI_BUS->PSEL.MISO = -1;
    BOARD_DISP_SPI_BUS->PSELDCX = cfg->pins.dc;

    BOARD_DISP_SPI_BUS->INTENSET = 
        (1<<1) | // stopped
        (0<<4) | // endrx
        (1<<6) | // end
        (1<<8) | // endtx
        (1<<19) | // started
        0;

    BOARD_DISP_SPI_BUS->CONFIG = BOARD_DISP_SPI_BUS_CFG;
    BOARD_DISP_SPI_BUS->FREQUENCY = BOARD_DISP_SPI_BUS_FREQ;

    NVIC_ClearPendingIRQ(SPIM3_IRQn);
    NVIC_EnableIRQ(SPIM3_IRQn);

    BOARD_DISP_SPI_BUS->ENABLE = 7;

    memset(&spi_status, 0, sizeof(spi_status));
}

static void spi_handle_irq(void)
{
    if (BOARD_DISP_SPI_BUS->EVENTS_STARTED) {
        BOARD_DISP_SPI_BUS->EVENTS_STARTED = 0;
        spi_status.running = 1;
    }
    if (BOARD_DISP_SPI_BUS->EVENTS_ENDTX) {
        BOARD_DISP_SPI_BUS->EVENTS_ENDTX = 0;
        spi_status.txed = 1;

    }
    if (BOARD_DISP_SPI_BUS->EVENTS_END) {
        BOARD_DISP_SPI_BUS->EVENTS_END = 0;
        spi_status.running = 0;
    }
    if (BOARD_DISP_SPI_BUS->EVENTS_STOPPED) {
        BOARD_DISP_SPI_BUS->EVENTS_STOPPED = 0;
        spi_status.running = 0;
    }
}

static void timer_init(void)
{
    NRF_TIMER0->MODE = 0;    // timer
    NRF_TIMER0->BITMODE = 3; // 32 bit0
    NRF_TIMER0->PRESCALER = 0; // div by 1 => 16 MHz
    NRF_TIMER0->TASKS_START = 1;
}

static uint32_t timer_read(int reset)
{
    NRF_TIMER0->TASKS_CAPTURE[0] = 1;
    if (reset) NRF_TIMER0->TASKS_CLEAR = 1;
    return NRF_TIMER0->CC[0];
}

static void disp_write_cmd_blocking(uint8_t cmd, uint32_t args_count, ...) {
    static uint8_t spi_cmd_buffer[32];
    va_list ap;
    const disp_cfg_t *cfg = disp_get_cfg();

    while (spi_status.running);
    
    gpio_set(cfg->pins.spi_csn, 0);
    spi_cmd_buffer[0] = cmd;
    va_start(ap, args_count);
    uint32_t arg_nbr = 0;
    while (arg_nbr < args_count) {
        spi_cmd_buffer[arg_nbr+1] = (uint8_t)va_arg(ap, int);
        arg_nbr++;
    }

    fprint_mem(0, spi_cmd_buffer, args_count+1);

    spi_status.txed = 0;

    BOARD_DISP_SPI_BUS->TXD.PTR = (uint32_t)(intptr_t)spi_cmd_buffer;
    BOARD_DISP_SPI_BUS->TXD.MAXCNT = 1 + args_count;
    BOARD_DISP_SPI_BUS->DCXCNT = 1;
    BOARD_DISP_SPI_BUS->TASKS_START = 1;

    while(!spi_status.txed);
    gpio_set(cfg->pins.spi_csn, 1);

}

#define NUMARGS(...)  (sizeof((int[]){0, ##__VA_ARGS__})/sizeof(int)-1)
#define DISP_CMD_BLOCKING(cmd, ...) disp_write_cmd_blocking(cmd, NUMARGS(__VA_ARGS__), ## __VA_ARGS__)
static void disp_init(void) 
{
    // power on sequence - See datasheet CH13613 SPEC V0.08, page 218.
    const disp_cfg_t *cfg = disp_get_cfg();
    // vdd always on
    // wait more than 10 ms before setting pin_reset high
    //cpu_halt(15);
    gpio_set(cfg->pins.resetn, 1);
    cpu_halt(15);
    gpio_set(cfg->pins.resetn, 0);
    cpu_halt(5);
    gpio_set(cfg->pins.resetn, 1);
    cpu_halt(15);

    DISP_CMD_BLOCKING(0xf0, 0x50);
    DISP_CMD_BLOCKING(0xb1, 0x78, 0x70);
    DISP_CMD_BLOCKING(0xf0, 0xf1);
    DISP_CMD_BLOCKING(0xc1, 0x02, 0x02, 0x02);
    DISP_CMD_BLOCKING(0xc3, 0x01, 0xf3);
    DISP_CMD_BLOCKING(0xf0, 0x50);
    DISP_CMD_BLOCKING(0x2a, 0x00, 0x06, 0x01, 0x8b);
    DISP_CMD_BLOCKING(0x2b, 0x00, 0x00, 0x01, 0x85);
    DISP_CMD_BLOCKING(0x35, 0x00);
    DISP_CMD_BLOCKING(0x36, 0xc0);
    DISP_CMD_BLOCKING(0x3a, 0x05);
    DISP_CMD_BLOCKING(0x11);
    cpu_halt(200);
    DISP_CMD_BLOCKING(0x29);
    cpu_halt(100);
    DISP_CMD_BLOCKING(0x38);


    int x1 = 20, y1 = 20, x2 = 40, y2 = 40;
    DISP_CMD_BLOCKING(0x2a, x1 >> 8, x1, x2 >> 8, x2);
    DISP_CMD_BLOCKING(0x2b, y1 >> 8, y1, y2 >> 8, y2);
    {
        gpio_set(cfg->pins.spi_csn, 0);

        static uint8_t gbuffer[20*20*2];
        memset(gbuffer, 0xff, sizeof(gbuffer));

        spi_status.txed = 0;

        BOARD_DISP_SPI_BUS->TXD.PTR = (uint32_t)(intptr_t)gbuffer;
        BOARD_DISP_SPI_BUS->TXD.MAXCNT = sizeof(gbuffer);
        BOARD_DISP_SPI_BUS->DCXCNT = 0;
        BOARD_DISP_SPI_BUS->TASKS_START = 1;

        while(!spi_status.txed);
        gpio_set(cfg->pins.spi_csn, 1);
    }
    
}

int main(void)
{
    cpu_init();
    board_init();
    gpio_init();
    disp_ifc_init();
    uart_config_t cfg = {
        .baudrate = UART_BAUDRATE_115200,
        .parity = UART_PARITY_NONE,
        .stopbits = UART_STOPBITS_1,
        .flowcontrol = UART_FLOWCONTROL_NONE};
    uart_init(0, &cfg);

    printf("\ndisp cli\n");
    cli_init(cli_cb, "\r\n;", " ,", "", "");

    timer_init();

    cpu_interrupt_enable();

    while (1)
    {
        int rx = uart_pollchar(UART_STD);
        if (rx > 0)
        {
            char c = (char)rx;
            cli_parse(&c, 1);
        }
    }
}

///////////////////////////// CLI STUFF //////////////////////////////////////

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
CLI_FUNCTION(cli_help, "help", "display help");

static int cli_reset(int argc, const char **argv)
{
    cpu_reset();
    return 0;
}
CLI_FUNCTION(cli_reset, "reset", "reset");

static int cli_test(int argc, const char **argv)
{
    disp_init();
    return 0;
}
CLI_FUNCTION(cli_test, "test", "test");

static int cli_timer(int argc, const char **argv)
{
    printf("%08x\n", timer_read(argc > 0));
    return 0;
}
CLI_FUNCTION(cli_timer, "timer", "(0): return timer (and reset)");

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
    default:
        printf("ERROR (%d)\n", res);
        break;
    }
}

void BOARD_DISP_SPI_BUS_IRQ_HANDLER_FUNC(void);
void BOARD_DISP_SPI_BUS_IRQ_HANDLER_FUNC(void)
{
    spi_handle_irq();
}