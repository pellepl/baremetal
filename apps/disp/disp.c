/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "minio.h"
#include "cli.h"

#include "disp.h"

#include "nrf52840.h"

static void cli_cb(const char *func_name, int res);

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

static void disp_init(void)
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
        (0<<1) | // stopped
        (0<<4) | // endrx
        (0<<6) | // end
        (1<<8) | // endtx
        (1<<19) | // started
        0;

    BOARD_DISP_SPI_BUS->CONFIG = BOARD_DISP_SPI_BUS_CFG;
    BOARD_DISP_SPI_BUS->FREQUENCY = BOARD_DISP_SPI_BUS_FREQ;

    NVIC_ClearPendingIRQ(SPIM3_IRQn);
    NVIC_EnableIRQ(SPIM3_IRQn);

    BOARD_DISP_SPI_BUS->ENABLE = 7;
}

static void spi_handle_irq(void)
{
    if (BOARD_DISP_SPI_BUS->EVENTS_STARTED) {
        BOARD_DISP_SPI_BUS->EVENTS_STARTED = 0;
        printf("spim3 irq STARTED\n");
    }
    if (BOARD_DISP_SPI_BUS->EVENTS_ENDTX) {
        BOARD_DISP_SPI_BUS->EVENTS_ENDTX = 0;
        printf("spim3 irq ENDTX\n");
    }
    if (BOARD_DISP_SPI_BUS->EVENTS_END) {
        BOARD_DISP_SPI_BUS->EVENTS_END = 0;
        printf("spim3 irq END\n");
    }
    if (BOARD_DISP_SPI_BUS->EVENTS_STOPPED) {
        BOARD_DISP_SPI_BUS->EVENTS_STOPPED = 0;
        printf("spim3 irq STOPPED\n");
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

int main(void)
{
    cpu_init();
    board_init();
    gpio_init();
    disp_init();
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
    BOARD_DISP_SPI_BUS->TXD.PTR = 0x20000000;
    BOARD_DISP_SPI_BUS->TXD.MAXCNT = 0x1000;
    BOARD_DISP_SPI_BUS->TASKS_START = 1;
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