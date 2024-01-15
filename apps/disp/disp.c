/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include <stdarg.h>

#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "minio.h"
#include "cli.h"

#include "disp.h"
#include "nrf.h"
#include "nrf52840.h"
#include "nrf52840_peripherals.h"

static void cli_cb(const char *func_name, int res);

#define NUMARGS(...) (sizeof((int[]){0, ##__VA_ARGS__}) / sizeof(int) - 1)
#define DISP_CMD_BLOCKING(cmd, ...) disp_write_cmd_blocking(cmd, NUMARGS(__VA_ARGS__), ##__VA_ARGS__)
#define DCX_DATA 1
#define DCX_COMM 0
#define DISP_W 390
#define DISP_H 390
#define BYTES_PER_PIXEL 2

static uint8_t gbuf[(uint32_t)(DISP_W * DISP_H * BYTES_PER_PIXEL / 2)];

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

static NRF_GPIO_Type *port_for_pin(uint16_t pin)
{
    return pin < 32 ? NRF_P0 : NRF_P1;
}

static void disp_ifc_init(void)
{
    const disp_cfg_t *cfg = disp_get_cfg();

    gpio_set(cfg->pins.resetn, 0);
    gpio_set(cfg->pins.spi_csn, 1);
    gpio_set(cfg->pins.spi_sck, 0);
    gpio_config(cfg->pins.resetn, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.spi_csn, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.dc, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.spi_mosi, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.spi_sck, GPIO_DIRECTION_FUNCTION_OUT, GPIO_PULL_NONE);
    gpio_config(cfg->pins.tearing, GPIO_DIRECTION_INPUT, GPIO_PULL_UP);
    NRF_GPIO_Type *port = port_for_pin(cfg->pins.spi_sck);
    port->PIN_CNF[cfg->pins.spi_sck & 0x1f] |= GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos;

    BOARD_DISP_SPI_BUS->PSEL.SCK = cfg->pins.spi_sck;
    BOARD_DISP_SPI_BUS->PSEL.MOSI = cfg->pins.spi_mosi;
    BOARD_DISP_SPI_BUS->PSEL.CSN = -1;
    BOARD_DISP_SPI_BUS->PSEL.MISO = -1;
    BOARD_DISP_SPI_BUS->PSELDCX = -1;

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

static void vsync_init(void)
{
}

static uint32_t timer_read(int reset)
{
    NRF_TIMER0->TASKS_CAPTURE[0] = 1;
    if (reset) NRF_TIMER0->TASKS_CLEAR = 1;
    return NRF_TIMER0->CC[0];
}

static void cs(int c)
{
    const disp_cfg_t *cfg = disp_get_cfg();
    gpio_set(cfg->pins.spi_csn, c);
    cpu_halt(0);
}

static void dcx(int data_else_comm)
{
    const disp_cfg_t *cfg = disp_get_cfg();
    gpio_set(cfg->pins.dc, data_else_comm);
}

static void disp_write_cmd_blocking_arr(const uint8_t *cmd_arr, uint32_t arr_len)
{
    while (spi_status.running)
        ;

    cs(0);
    dcx(DCX_COMM);
    spi_status.txed = 0;
    BOARD_DISP_SPI_BUS->TXD.PTR = (uint32_t)(intptr_t)cmd_arr;
    BOARD_DISP_SPI_BUS->TXD.MAXCNT = 1;
    BOARD_DISP_SPI_BUS->DCXCNT = 1;
    BOARD_DISP_SPI_BUS->TASKS_START = 1;

    while (!spi_status.txed)
        ;
    while (spi_status.running)
        ;
    cs(1);
    dcx(DCX_DATA);

    if (arr_len > 1)
    {
        cs(0);
        spi_status.txed = 0;
        BOARD_DISP_SPI_BUS->TXD.PTR = (uint32_t)(intptr_t)cmd_arr + 1;
        BOARD_DISP_SPI_BUS->TXD.MAXCNT = arr_len - 1;
        BOARD_DISP_SPI_BUS->DCXCNT = 0;
        BOARD_DISP_SPI_BUS->TASKS_START = 1;

        while (!spi_status.txed)
            ;
        cs(1);
    }
}

static void disp_write_cmd_blocking(uint8_t cmd, uint32_t args_count, ...)
{
    static uint8_t spi_cmd_buffer[32];
    va_list ap;

    spi_cmd_buffer[0] = cmd;
    va_start(ap, args_count);
    uint32_t arg_nbr = 0;
    while (arg_nbr < args_count)
    {
        spi_cmd_buffer[arg_nbr + 1] = (uint8_t)va_arg(ap, int);
        arg_nbr++;
    }
    // fprint_mem(0, spi_cmd_buffer, args_count + 1);
    disp_write_cmd_blocking_arr(spi_cmd_buffer, args_count + 1);
}

static void disp_init(void) 
{
    // power on sequence - See datasheet CH13613 SPEC V0.08, page 218.
    const disp_cfg_t *cfg = disp_get_cfg();
    // vdd always on
    // wait more than 10 ms before setting pin_reset high
    gpio_set(cfg->pins.resetn, 0);
    cpu_halt(50);
    gpio_set(cfg->pins.resetn, 1);
    cpu_halt(15);
    gpio_set(cfg->pins.resetn, 0);
    cpu_halt(5);
    gpio_set(cfg->pins.resetn, 1);
    cpu_halt(15);

    DISP_CMD_BLOCKING(CH13613_M_MAUCCTR, 0x50);
    DISP_CMD_BLOCKING(CH13613_MP0_IFCTR2, 0x78, 0x70);
    DISP_CMD_BLOCKING(CH13613_M_MAUCCTR, 0x51);
    DISP_CMD_BLOCKING(CH13613_MP1_ELVSSCMD, 0x02, 0x02, 0x02);
    DISP_CMD_BLOCKING(CH13613_MP1_SWIRECTR, 0x01, 0xf3);
    DISP_CMD_BLOCKING(CH13613_M_MAUCCTR, 0x50);
    DISP_CMD_BLOCKING(CH13613_USER_CASET, 0x00, 0x06, 0x01, 0x8b);
    DISP_CMD_BLOCKING(CH13613_USER_RASET, 0x00, 0x00, 0x01, 0x85);
    DISP_CMD_BLOCKING(CH13613_USER_TEON, 0x00);
    DISP_CMD_BLOCKING(CH13613_USER_SDC, 0xc0);
    DISP_CMD_BLOCKING(CH13613_USER_IPF, 0x05);
    DISP_CMD_BLOCKING(CH13613_USER_SLPOUT);
    cpu_halt(200);
    DISP_CMD_BLOCKING(CH13613_USER_DISPON);
    cpu_halt(100);
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
    disp_init();
    vsync_init();

    printf("init done\n");

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

static int cli_disp_init(int argc, const char **argv)
{
    disp_init();
    return 0;
}
CLI_FUNCTION(cli_disp_init, "init", "initialize display");

static int cli_disp_cmd(int argc, const char **argv)
{
    if (argc == 0)
        return ERR_CLI_EINVAL;
    static uint8_t cmd[32];
    for (int i = 0; i < argc; i++)
    {
        cmd[i] = (uint8_t)strtol(argv[i], 0, 0);
    }
    disp_write_cmd_blocking_arr(cmd, argc);
    return 0;
}
CLI_FUNCTION(cli_disp_cmd, "cmd", "<cmd> (<data> *): send command to display");

static int cli_timer(int argc, const char **argv)
{
    printf("%08x\n", timer_read(argc > 0));
    return 0;
}
CLI_FUNCTION(cli_timer, "timer", "(0): return timer (and reset)");

static int cli_box(int argc, const char **argv)
{
    int x1 = DISP_W / 2 - 10;
    int y1 = DISP_H / 2 - 10;
    int w = 20;
    int h = 20;
    uint8_t col = 0xff;
    if (argc >= 1)
        x1 = strtol(argv[0], 0, 0);
    if (argc >= 2)
        y1 = strtol(argv[1], 0, 0);
    if (argc >= 3)
        w = strtol(argv[2], 0, 0);
    if (argc >= 4)
        h = strtol(argv[3], 0, 0);
    if (argc >= 5)
        col = strtol(argv[4], 0, 0);

    int x2 = x1 + w + 1;
    int y2 = y1 + h + 1;

    uint32_t len_to_tx = (uint32_t)(w * h * BYTES_PER_PIXEL);

    memset(gbuf, col, sizeof(gbuf));

    DISP_CMD_BLOCKING(CH13613_USER_CASET, x1 >> 8, x1, x2 >> 8, x2);
    DISP_CMD_BLOCKING(CH13613_USER_RASET, y1 >> 8, y1, y2 >> 8, y2);
    DISP_CMD_BLOCKING(CH13613_USER_RAMW);
    cs(0);
    dcx(DCX_DATA);

    (void)timer_read(1);
    spi_status.txed = 0;

    BOARD_DISP_SPI_BUS->TXD.PTR = (uint32_t)(intptr_t)gbuf;
    BOARD_DISP_SPI_BUS->TXD.MAXCNT = len_to_tx;
    BOARD_DISP_SPI_BUS->DCXCNT = 0;
    BOARD_DISP_SPI_BUS->TASKS_START = 1;

    while (!spi_status.txed)
        ;
    uint32_t ticks = timer_read(1);
    float speed = 8.f * (float)len_to_tx * 16000000.f / (float)ticks;
    printf("box ticks %d ticks, %f bps, %d bytes\n", ticks, speed, len_to_tx);
    cs(1);
    return 0;
}
CLI_FUNCTION(cli_box, "box", "(x(,y(,w(,h(,col)))))): draw box");

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