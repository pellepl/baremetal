/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include <stdarg.h>
#include <math.h>

#include "board.h"
#include "gpio_driver.h"
#include "uart_driver.h"
#include "minio.h"
#include "cli.h"

#include "disp.h"
#include "nrf.h"
#include "nrf52840.h"
#include "nrf52840_peripherals.h"


#define NUMARGS(...) (sizeof((int[]){0, ##__VA_ARGS__}) / sizeof(int) - 1)
#define DISP_CMD_BLOCKING(cmd, ...) disp_write_cmd_blocking(cmd, NUMARGS(__VA_ARGS__), ##__VA_ARGS__)
#define DCX_DATA 1
#define DCX_COMM 0
#define DISP_W 388
#define DISP_H 390
#define BYTES_PER_PIXEL (0.5f)
#define BYTES_PER_COL (uint32_t)(DISP_W * BYTES_PER_PIXEL)
#define MAX_SPI_CHUNK 0x7fff
#define G_BUF_SIZE (int)(DISP_W * DISP_H * BYTES_PER_PIXEL)

const uint16_t sineLookupTable[] = {
0x8000, 0x8324, 0x8647, 0x896a, 0x8c8b, 0x8fab, 0x92c7, 0x95e1,
0x98f8, 0x9c0b, 0x9f19, 0xa223, 0xa527, 0xa826, 0xab1f, 0xae10,
0xb0fb, 0xb3de, 0xb6b9, 0xb98c, 0xbc56, 0xbf17, 0xc1cd, 0xc47a,
0xc71c, 0xc9b3, 0xcc3f, 0xcebf, 0xd133, 0xd39a, 0xd5f5, 0xd842,
0xda82, 0xdcb3, 0xded7, 0xe0eb, 0xe2f1, 0xe4e8, 0xe6cf, 0xe8a6,
0xea6d, 0xec23, 0xedc9, 0xef5e, 0xf0e2, 0xf254, 0xf3b5, 0xf504,
0xf641, 0xf76b, 0xf884, 0xf989, 0xfa7c, 0xfb5c, 0xfc29, 0xfce3,
0xfd89, 0xfe1d, 0xfe9c, 0xff09, 0xff61, 0xffa6, 0xffd8, 0xfff5,
0xffff, 0xfff5, 0xffd8, 0xffa6, 0xff61, 0xff09, 0xfe9c, 0xfe1d,
0xfd89, 0xfce3, 0xfc29, 0xfb5c, 0xfa7c, 0xf989, 0xf884, 0xf76b,
0xf641, 0xf504, 0xf3b5, 0xf254, 0xf0e2, 0xef5e, 0xedc9, 0xec23,
0xea6d, 0xe8a6, 0xe6cf, 0xe4e8, 0xe2f1, 0xe0eb, 0xded7, 0xdcb3,
0xda82, 0xd842, 0xd5f5, 0xd39a, 0xd133, 0xcebf, 0xcc3f, 0xc9b3,
0xc71c, 0xc47a, 0xc1cd, 0xbf17, 0xbc56, 0xb98c, 0xb6b9, 0xb3de,
0xb0fb, 0xae10, 0xab1f, 0xa826, 0xa527, 0xa223, 0x9f19, 0x9c0b,
0x98f8, 0x95e1, 0x92c7, 0x8fab, 0x8c8b, 0x896a, 0x8647, 0x8324,
0x8000, 0x7cdb, 0x79b8, 0x7695, 0x7374, 0x7054, 0x6d38, 0x6a1e,
0x6707, 0x63f4, 0x60e6, 0x5ddc, 0x5ad8, 0x57d9, 0x54e0, 0x51ef,
0x4f04, 0x4c21, 0x4946, 0x4673, 0x43a9, 0x40e8, 0x3e32, 0x3b85,
0x38e3, 0x364c, 0x33c0, 0x3140, 0x2ecc, 0x2c65, 0x2a0a, 0x27bd,
0x257d, 0x234c, 0x2128, 0x1f14, 0x1d0e, 0x1b17, 0x1930, 0x1759,
0x1592, 0x13dc, 0x1236, 0x10a1, 0xf1d, 0xdab, 0xc4a, 0xafb,
0x9be, 0x894, 0x77b, 0x676, 0x583, 0x4a3, 0x3d6, 0x31c,
0x276, 0x1e2, 0x163, 0xf6, 0x9e, 0x59, 0x27, 0x0a,
0x00, 0x0a, 0x27, 0x59, 0x9e, 0xf6, 0x163, 0x1e2,
0x276, 0x31c, 0x3d6, 0x4a3, 0x583, 0x676, 0x77b, 0x894,
0x9be, 0xafb, 0xc4a, 0xdab, 0xf1d, 0x10a1, 0x1236, 0x13dc,
0x1592, 0x1759, 0x1930, 0x1b17, 0x1d0e, 0x1f14, 0x2128, 0x234c,
0x257d, 0x27bd, 0x2a0a, 0x2c65, 0x2ecc, 0x3140, 0x33c0, 0x364c,
0x38e3, 0x3b85, 0x3e32, 0x40e8, 0x43a9, 0x4673, 0x4946, 0x4c21,
0x4f04, 0x51ef, 0x54e0, 0x57d9, 0x5ad8, 0x5ddc, 0x60e6, 0x63f4,
0x6707, 0x6a1e, 0x6d38, 0x7054, 0x7374, 0x7695, 0x79b8, 0x7cdb};

#define INDEXED_IMAGE_W 256
#define INDEXED_IMAGE_H 256

extern const uint8_t indexed_image[INDEXED_IMAGE_W * INDEXED_IMAGE_H];
extern const uint32_t indexed_image_palette[16];
extern unsigned const char font[256][8];

static uint8_t g_buffer_a[G_BUF_SIZE];
static uint8_t g_buffer_b[G_BUF_SIZE];
static uint8_t *g_buffer = g_buffer_a;

static struct {
    volatile uint32_t running;
    volatile uint32_t txed;
    volatile uint8_t *g_buffer;
    volatile uint32_t gbuf_ix;
    volatile uint32_t gbuf_chunk_len;
    volatile uint32_t target_gbuf_ix;
} spi_status;

static volatile int32_t vsync_last_tick = 0;
static volatile int32_t vsync_last_delta_tick = 0;
static volatile float last_vsync_freq = 0;
static volatile int vsync = 0;

static void cli_cb(const char *func_name, int res);
static void spi_dispatch_gbuf_chunk(void);
static void disp_write_cmd_blocking(uint8_t cmd, uint32_t args_count, ...);

static uint32_t lfsr_a, lfsr_b, lfsr_c;

static void prand_seed(uint32_t seed) {
	lfsr_a = seed ^ 0x20070515;
	lfsr_b = seed ^ 0x20090129;
	lfsr_c = seed ^ 0x20140318;
}

static uint32_t _prand_bit(void) {
	// https://www.schneier.com/academic/archives/1994/09/pseudo-random_sequen.html
	lfsr_a = (((((lfsr_a >> 31) ^ (lfsr_a >> 6) ^ (lfsr_a >> 4) ^ (lfsr_a >> 2) ^ (lfsr_a << 1) ^ lfsr_a) & 1) << 31) |
			  lfsr_a >> 1);
	lfsr_b = ((((lfsr_b >> 30) ^ (lfsr_b >> 2)) & 1) << 30) | (lfsr_b >> 1);
	lfsr_c = ((((lfsr_c >> 28) ^ (lfsr_c >> 1)) & 1) << 28) | (lfsr_c >> 1);
	return (uint32_t)(lfsr_a ^ lfsr_b ^ lfsr_c) & 1;
}

static uint32_t prand(uint8_t bits) {
	uint32_t r = 0;
	while (bits--) {
		r = (r << 1) | _prand_bit();
	}
	return r;
}

static float sine(float x) {
    if (x < 0) x = -x;
    uint32_t ix = (uint32_t)(x / 3.14f * 256.f);
    ix &= 0xff;
    return ((uint32_t)sineLookupTable[ix]) / 32768.f -1.f;
}

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
        if (spi_status.target_gbuf_ix > 0) {
            spi_dispatch_gbuf_chunk();
        }
    }
    if (BOARD_DISP_SPI_BUS->EVENTS_END) {
        BOARD_DISP_SPI_BUS->EVENTS_END = 0;
        spi_status.running = 0;
        if (spi_status.gbuf_chunk_len == 0) { // gbuf_chunk_len = 0 => end of transfer
            spi_status.target_gbuf_ix = 0;
        }
    }
    if (BOARD_DISP_SPI_BUS->EVENTS_STOPPED) {
        BOARD_DISP_SPI_BUS->EVENTS_STOPPED = 0;
        spi_status.running = 0;
        if (spi_status.gbuf_chunk_len == 0) {
            spi_status.target_gbuf_ix = 0;
        }
    }
}

static void timer_init(void)
{
    NRF_TIMER0->MODE = 0;    // timer
    NRF_TIMER0->BITMODE = 3; // 32 bit
    NRF_TIMER0->PRESCALER = 0; // div by 1 => 16 MHz
    NRF_TIMER0->TASKS_START = 1;
}

static void vsync_init(void)
{
    const disp_cfg_t *cfg = disp_get_cfg();
    const uint16_t pin_tearing = cfg->pins.tearing;

    // Timer1, timer mode, 32 bit, 16 MHz
    NRF_TIMER1->MODE = 0;
    NRF_TIMER1->BITMODE = 3;
    NRF_TIMER1->PRESCALER = 0;

    // Use GPIOTE channel 0 to trigger on tearing pin on up flank
    NRF_GPIOTE->CONFIG[0] = 
        (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
        ((pin_tearing & 0x1f) << GPIOTE_CONFIG_PSEL_Pos) |
        ((pin_tearing >> 5) << GPIOTE_CONFIG_PORT_Pos) |
        (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos) |
        0;

    // PPI: tie TIMER_START and CAPTURE to GPIOTE pin_tearing high flank event 
    NRF_PPI->CH[0].EEP = (uint32_t)(intptr_t)&NRF_GPIOTE->EVENTS_IN[0];
    NRF_PPI->CH[0].TEP = (uint32_t)(intptr_t)&NRF_TIMER1->TASKS_START;
    NRF_PPI->FORK[0].TEP = (uint32_t)(intptr_t)&NRF_TIMER1->TASKS_CAPTURE[0];
    NRF_PPI->CHEN = PPI_CHENSET_CH0_Enabled << PPI_CHENSET_CH0_Pos;

    // Enable interrupt for high flank on pin_tearing
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);
    NVIC_EnableIRQ(GPIOTE_IRQn);

    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos;
}

static void vsync_halt(void) {
    vsync = 0;
    while (!vsync);
    vsync = 0;
}

static uint32_t vsync_elased(void) {
    NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    return (int32_t)NRF_TIMER1->CC[1] - (int32_t)vsync_last_tick;
}

static uint32_t vsync_ticks_left(void) {
{
    NRF_TIMER1->TASKS_CAPTURE[1] = 1;
    uint32_t tval = NRF_TIMER1->CC[1];
    uint32_t local_last_tick = vsync_last_tick;
    uint32_t local_last_delta = vsync_last_delta_tick;
    uint32_t delta = tval - local_last_tick;
    if (delta > local_last_delta) delta = local_last_delta;
    return local_last_delta - delta;
}

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

static void spi_dispatch_gbuf_chunk(void) {
    if (spi_status.gbuf_chunk_len == 0) {
        // first chunk
        spi_status.gbuf_ix = 0;
        cs(0);
        dcx(DCX_DATA);
    } else {
        spi_status.gbuf_ix += spi_status.gbuf_chunk_len;
    }
    if (spi_status.gbuf_ix >= spi_status.target_gbuf_ix) {
         // done, indicate by setting chunk_len to 0
        spi_status.gbuf_chunk_len = 0;
        cs(1);
    } else {
        // first / next chunk
        uint32_t remaining = spi_status.target_gbuf_ix - spi_status.gbuf_ix;
        uint32_t chunk_len = (remaining > MAX_SPI_CHUNK) ? MAX_SPI_CHUNK : remaining;
        spi_status.gbuf_chunk_len = chunk_len;
        BOARD_DISP_SPI_BUS->TXD.PTR = (uint32_t)(intptr_t)spi_status.g_buffer + spi_status.gbuf_ix;
        BOARD_DISP_SPI_BUS->TXD.MAXCNT = chunk_len;
        BOARD_DISP_SPI_BUS->DCXCNT = 0;
        BOARD_DISP_SPI_BUS->TASKS_START = 1;
    }
}

static void disp_send_gbuf(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h) {
    while (spi_status.target_gbuf_ix != 0);
    
    uint32_t len_to_tx = (uint32_t)(w * h * BYTES_PER_PIXEL);
    
    
    uint16_t x2 = x1 + w-1;
    uint16_t y2 = y1 + h-1;

    DISP_CMD_BLOCKING(CH13613_USER_CASET, x1 >> 8, x1, x2 >> 8, x2);
    DISP_CMD_BLOCKING(CH13613_USER_RASET, y1 >> 8, y1, y2 >> 8, y2);
    DISP_CMD_BLOCKING(CH13613_USER_RAMW);
    if (len_to_tx > G_BUF_SIZE)
        len_to_tx = G_BUF_SIZE;
    spi_status.target_gbuf_ix = len_to_tx;
    spi_status.gbuf_chunk_len = 0;
    spi_status.g_buffer = g_buffer;
    g_buffer = (g_buffer == g_buffer_a ? g_buffer_b : g_buffer_a);
    spi_dispatch_gbuf_chunk();
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

const uint32_t palette_balls[16] = {
    0x000000, 0x003377, 0x0066bb, 0x0099ff,
    0x666666, 0x007700, 0x00bb00, 0x00ff00,
    0xaaaaaa, 0x770000, 0xbb0000, 0xbb0000,
    0xffffff, 0x007777, 0x00bbbb, 0x00bbbb
};
const uint32_t palette_bw[16] = {
    0x000000, 0x111111, 0x222222, 0x333333,
    0x444444, 0x555555, 0x666666, 0x777777,
    0x888888, 0x999999, 0xaaaaaa, 0xbbbbbb,
    0xcccccc, 0xdddddd, 0xeeeeee, 0xffffff
};
const uint8_t balls_gfx[3][8*16] = {
     {
        0x00, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x11, 0x22, 0x22, 0x11, 0x00, 0x00,
        0x00, 0x01, 0x22, 0x33, 0x32, 0x11, 0x10, 0x00,
        0x00, 0x12, 0xcc, 0xcc, 0x32, 0x11, 0x11, 0x00,
        0x01, 0x2c, 0xcc, 0xcc, 0x33, 0x21, 0x11, 0x10,
        0x01, 0x2c, 0xcc, 0xcc, 0x33, 0x21, 0x11, 0x10,
        0x12, 0x23, 0x33, 0x33, 0x33, 0x21, 0x11, 0x11,
        0x12, 0x23, 0x33, 0x33, 0x33, 0x22, 0x11, 0x11,
        0x12, 0x33, 0x33, 0x33, 0x32, 0x22, 0x11, 0x11,
        0x12, 0x33, 0x33, 0x33, 0x32, 0x22, 0x11, 0x11,
        0x01, 0x23, 0x33, 0x33, 0x32, 0x22, 0x11, 0x10,
        0x01, 0x23, 0x33, 0x32, 0x22, 0x21, 0x11, 0x10,
        0x00, 0x12, 0x33, 0x22, 0x22, 0x21, 0x11, 0x00,
        0x00, 0x01, 0x22, 0x22, 0x22, 0x11, 0x10, 0x00,
        0x00, 0x00, 0x11, 0x22, 0x22, 0x11, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x00,
     },
     {
        0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x55, 0x66, 0x66, 0x55, 0x00, 0x00,
        0x00, 0x05, 0x66, 0x77, 0x76, 0x55, 0x50, 0x00,
        0x00, 0x56, 0xcc, 0xcc, 0x76, 0x55, 0x55, 0x00,
        0x05, 0x6c, 0xcc, 0xcc, 0x77, 0x65, 0x55, 0x50,
        0x05, 0x6c, 0xcc, 0xcc, 0x77, 0x65, 0x55, 0x50,
        0x56, 0x67, 0x77, 0x77, 0x77, 0x65, 0x55, 0x55,
        0x56, 0x67, 0x77, 0x77, 0x77, 0x66, 0x55, 0x55,
        0x56, 0x77, 0x77, 0x77, 0x76, 0x66, 0x55, 0x55,
        0x56, 0x77, 0x77, 0x77, 0x76, 0x66, 0x55, 0x55,
        0x05, 0x67, 0x77, 0x77, 0x76, 0x66, 0x55, 0x50,
        0x05, 0x67, 0x77, 0x76, 0x66, 0x65, 0x55, 0x50,
        0x00, 0x56, 0x77, 0x66, 0x66, 0x65, 0x55, 0x00,
        0x00, 0x05, 0x66, 0x66, 0x66, 0x55, 0x50, 0x00,
        0x00, 0x00, 0x55, 0x66, 0x66, 0x55, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x00,
     },
     {
        0x00, 0x00, 0x00, 0x99, 0x99, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x99, 0xaa, 0xaa, 0x99, 0x00, 0x00,
        0x00, 0x09, 0xaa, 0xbb, 0xba, 0x99, 0x90, 0x00,
        0x00, 0x9a, 0xcc, 0xcc, 0xba, 0x99, 0x99, 0x00,
        0x09, 0xac, 0xcc, 0xcc, 0xbb, 0xa9, 0x99, 0x90,
        0x09, 0xac, 0xcc, 0xcc, 0xbb, 0xa9, 0x99, 0x90,
        0x9a, 0xab, 0xbb, 0xbb, 0xbb, 0xa9, 0x99, 0x99,
        0x9a, 0xab, 0xbb, 0xbb, 0xbb, 0xaa, 0x99, 0x99,
        0x9a, 0xbb, 0xbb, 0xbb, 0xba, 0xaa, 0x99, 0x99,
        0x9a, 0xbb, 0xbb, 0xbb, 0xba, 0xaa, 0x99, 0x99,
        0x09, 0xab, 0xbb, 0xbb, 0xba, 0xaa, 0x99, 0x90,
        0x09, 0xab, 0xbb, 0xba, 0xaa, 0xa9, 0x99, 0x90,
        0x00, 0x9a, 0xbb, 0xaa, 0xaa, 0xa9, 0x99, 0x00,
        0x00, 0x09, 0xaa, 0xaa, 0xaa, 0x99, 0x90, 0x00,
        0x00, 0x00, 0x99, 0xaa, 0xaa, 0x99, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x99, 0x99, 0x00, 0x00, 0x00,
     },
};

typedef struct {
    int x;
    int y;
    int dx; int dy;
    uint8_t color;
} ball_t;

static void disp_set_palette(const uint32_t *p) {
    // send palette in four batches, first three commands take 5 colours, last takes one colour (5+5+5+1)
    uint8_t cmd[1 + 3*5];
    for (int command = CH13613_MP0_COLMS1; command < CH13613_MP0_COLMS4; command++) {
        cmd[0] = command;
        for (uint32_t i = 0; i < 5; i++) {
            uint32_t col = *p++;
            cmd[1 + i * 3] = (col >> 16) & 0xff;
            cmd[2 + i * 3] = (col >> 8) & 0xff;
            cmd[3 + i * 3] = (col >> 0) & 0xff;
        }
        disp_write_cmd_blocking_arr(cmd, sizeof(cmd));
    }
    cmd[0] = CH13613_MP0_COLMS4;
    uint32_t col = *p++;
    cmd[1] = (col >> 16) & 0xff;
    cmd[2] = (col >> 8) & 0xff;
    cmd[3] = (col >> 0) & 0xff;
    disp_write_cmd_blocking_arr(cmd, 1+3);
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

    uint8_t SPI111_OPT = 0 << 2;
    uint8_t SPI4BIT_EN = 1 << 3;

    DISP_CMD_BLOCKING(CH13613_M_MAUCCTR, 0x50);
    DISP_CMD_BLOCKING(CH13613_MP0_IFCTR2, 0x78, 0x00 | SPI111_OPT | SPI4BIT_EN);
    // CH13613_MP0_IFCTR2[0]: 0x78
    //  7    RAMKP          0
    //  6    DE_RAMKP       1
    //  5    QSPI_MIPI_EN   1
    //  4    SDA_EN         1
    //  3    SPI_WRAM       1
    //  2-1  DSPI_CFG       00
    //  0    DSPI_EN        0
    // CH13613_MP0_IFCTR2[1]: 0x70
    //  7    fcomp_rdecomp  0
    //  6-4  QSPI_GRAY256   000
    //  3    SPI4bit_en     0
    //  2    SPI111_opt     0
    //  1-0  IM_SEL         00
    //
    DISP_CMD_BLOCKING(CH13613_M_MAUCCTR, 0x51);
    DISP_CMD_BLOCKING(CH13613_MP1_ELVSSCMD, 0x02, 0x02, 0x02);
    DISP_CMD_BLOCKING(CH13613_MP1_SWIRECTR, 0x01, 0xf3);
    DISP_CMD_BLOCKING(CH13613_M_MAUCCTR, 0x50);
    DISP_CMD_BLOCKING(CH13613_USER_CASET, 0x00, 0x06, 0x01, 0x8b);
    DISP_CMD_BLOCKING(CH13613_USER_RASET, 0x00, 0x00, 0x01, 0x85);
    DISP_CMD_BLOCKING(CH13613_USER_TEON, 0x00);
    DISP_CMD_BLOCKING(CH13613_USER_SDC, 0xc0);
    DISP_CMD_BLOCKING(CH13613_USER_IPF, 0x01); // SPI111  // 0x05); 16 bit
    DISP_CMD_BLOCKING(CH13613_USER_SLPOUT);
    cpu_halt(200);
    DISP_CMD_BLOCKING(CH13613_USER_DISPON);
    cpu_halt(100);
}


static void disp_pattern(float dh_x, float dh_y, float dv_x, float dv_y) {
    float ix = DISP_W / 2 - (dh_x + dv_y) * INDEXED_IMAGE_W / 2;
    float iy = DISP_H / 2 - (dv_x + dh_y) * INDEXED_IMAGE_H / 2;
    for (uint32_t y = 0; y < DISP_H; y++, ix += dv_x, iy += dv_y) {
        float _ix = ix;
        float _iy = iy;
        for (uint32_t x = 0; x < BYTES_PER_COL; x++) {
            float __ix;
            float __iy;
            uint32_t _ix_i, _iy_i;

            __ix = _ix < 0 ? -_ix : _ix;
            __iy = _iy < 0 ? -_iy : _iy;
            
            _ix_i = ((uint32_t)__ix) & 0xff;
            _iy_i = ((uint32_t)__iy) & 0xff;
            uint32_t col1 = indexed_image[_ix_i + (_iy_i<<8)];

            _ix += dh_x;
            _iy += dh_y;

            _ix_i = ((uint32_t)__ix) & 0xff;
            _iy_i = ((uint32_t)__iy) & 0xff;
            uint32_t col2 = indexed_image[_ix_i + (_iy_i<<8)];

            _ix += dh_x;
            _iy += dh_y;

            g_buffer[x+y*BYTES_PER_COL] = (col1) | (col2 << 4);
        }
    }
}

static void disp_str(const char *str, uint32_t x, uint32_t y) {
    char c;
    while ((c = *str++) != 0) {
        if (x > BYTES_PER_COL) break;
        if (y > DISP_H) break;
        for (int fy = 0; fy < 8; fy++) {
            uint8_t bitmap = font[(uint8_t)c][fy];
            if (y  + fy*2 > DISP_H) break;
            for (int fx = 0; fx < 8; fx++) {
                if (x + fx*2 > BYTES_PER_COL) continue;
                if (bitmap & (1<<(7-fx))) {
                    g_buffer[x+fx*2+(fy*4+y)*BYTES_PER_COL] = 0x11;
                    g_buffer[x+fx*2+(fy*4+1+y)*BYTES_PER_COL] = 0x11;
                    g_buffer[x+fx*2+(fy*4+2+y)*BYTES_PER_COL] = 0x11;
                    g_buffer[x+fx*2+(fy*4+3+y)*BYTES_PER_COL] = 0x11;
                    g_buffer[x+fx*2+1+(fy*4+y)*BYTES_PER_COL] = 0x11;
                    g_buffer[x+fx*2+1+(fy*4+1+y)*BYTES_PER_COL] = 0x11;
                    g_buffer[x+fx*2+1+(fy*4+2+y)*BYTES_PER_COL] = 0x11;
                    g_buffer[x+fx*2+1+(fy*4+3+y)*BYTES_PER_COL] = 0x11;
                }
            }
        }
        x += 2*8;
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
    disp_init();
    vsync_init();

    prand_seed(0xfedebeda);

    printf("init done\n");

    disp_set_palette(indexed_image_palette);
    disp_pattern(1.f, 0, 0, 1.f);
    disp_send_gbuf(0,0,DISP_W, DISP_H);
        
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

static int cli_last_vsync(int argc, const char **argv)
{
    printf("%f Hz\n", last_vsync_freq);
    return 0;
}
CLI_FUNCTION(cli_last_vsync, "vsync", "return last vsync frequency");

static int cli_pal(int argc, const char **argv)
{
    uint32_t pal[16] = {
    0x000000, 0x111111, 0x222222, 0x333333,
    0x444444, 0x555555, 0x666666, 0x777777,
    0x888888, 0x999999, 0xaaaaaa, 0xbbbbbb,
    0xcccccc, 0xdddddd, 0xeeeeee, 0xffffff
    };
    for (int i = 0; i < argc; i++) {
        pal[i] = strtol(argv[i],0,0);
    }
    disp_set_palette(pal);
    return 0;
}
CLI_FUNCTION(cli_pal, "pal", "(colors)*: set palette");

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

    __builtin_memset(g_buffer, col, G_BUF_SIZE);

    DISP_CMD_BLOCKING(CH13613_USER_CASET, x1 >> 8, x1, x2 >> 8, x2);
    DISP_CMD_BLOCKING(CH13613_USER_RASET, y1 >> 8, y1, y2 >> 8, y2);
    DISP_CMD_BLOCKING(CH13613_USER_RAMW);
    cs(0);
    dcx(DCX_DATA);

    (void)timer_read(1);
    spi_status.txed = 0;

    BOARD_DISP_SPI_BUS->TXD.PTR = (uint32_t)(intptr_t)g_buffer;
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
CLI_FUNCTION(cli_box, "box", "(x(,y(,w(,h(,fill)))))): draw box");

static int cli_pat(int argc, const char **argv)
{
    float dx = (float)strtol(argv[0], 0, 0) / 100.f;
    float dy = (float)strtol(argv[1], 0, 0) / 100.f;
    (void)timer_read(1);
    disp_pattern(dx, dy, -dy, dx);
    uint32_t ticks = timer_read(1);
    printf("ticks %d ticks, %f Hz\n", ticks, 16000000.f / (float)ticks);
    disp_send_gbuf(0,0,DISP_W, DISP_H);

    return 0;
}
CLI_FUNCTION(cli_pat, "pat", "dx/100, dy/100: pattern");

static int cli_demo_bunny(int argc, const char **argv)
{
    int runs = 300;
    if (argc > 0) runs = strtol(argv[0], 0, 0);
    disp_set_palette(indexed_image_palette);
    if (argc > 1) {
        uint32_t rev_pal[16];
        uint8_t permutation = strtol(argv[1], 0, 0) % 6;
        uint8_t rs = 16;
        uint8_t gs = 8;
        uint8_t bs = 0;
        switch (permutation) {
            case 0:
            break;
            case 1: rs = 16; gs = 0; bs = 8;
            break;
            case 2: rs =  8; gs = 16;  bs = 0;
            break; 
            case 3: rs =  8; gs = 0;  bs = 16;
            break;
            case 4: rs =  0; gs = 8;  bs = 16;
            break;
            default:
            case 5: rs =  0; gs = 16;  bs = 8;
            break;
        }
        for (int i = 0; i < 16; i++) {
            uint32_t c = indexed_image_palette[i];
            uint8_t r = (c >> 16) & 0xff;
            uint8_t g = (c >> 8) & 0xff;
            uint8_t b = (c >> 0) & 0xff;

            rev_pal[i] = (r << rs) | (g << gs) | (b << bs);
        }
        disp_set_palette(rev_pal);
    }
    (void)timer_read(1);
    while (--runs) {
        float dx1 = sine((float)runs * 0.07f)*1.7f;
        float dy1 = sine((float)runs * 0.08f + 0.5f)*1.7f;
        disp_pattern(dx1, dy1, -dy1, dx1);
        disp_send_gbuf(0,0,DISP_W, DISP_H);
        uint32_t ticks = timer_read(1);
        if ((runs % 20) == 0) printf("fps: %f\n", 16000000.f/(float)ticks);
    }

    disp_pattern(1.f, 0, 0, 1.f);
    disp_send_gbuf(0,0,DISP_W, DISP_H);


    return 0;
}
CLI_FUNCTION(cli_demo_bunny, "demo_bunny", "(<cycles> (bw)): demo_bunny");

#define POS(x) ((x) < 0 ? -(x):(x))
#define NEG(x) ((x) >= 0 ? -(x):(x))
static int cli_demo_balls(int argc, const char **argv)
{
    int runs = 300;
    int ball_count = 350;
    if (argc > 0) runs = strtol(argv[0], 0, 0);
    if (argc > 1) ball_count = strtol(argv[1], 0, 0);
    disp_set_palette(palette_balls);
    ball_t balls[ball_count];
    prand_seed(0x123);
    for (int b = 0; b < ball_count; b++) {
        balls[b].color = b % 3;
        balls[b].x = prand(32) % (DISP_W-16);
        balls[b].y = prand(32) % (DISP_H-16);
        balls[b].dx = (prand(32) % 9) - 5;
        if (balls[b].dx >= 0) balls[b].dx ++;
        balls[b].dy = (prand(32) % 9) - 5;
        if (balls[b].dy >= 0) balls[b].dy ++;
    }
    (void)timer_read(1);
    while (--runs) {
        vsync_halt();
        __builtin_memset(g_buffer, 0, G_BUF_SIZE);
        for (int b = 0; b < ball_count; b++) {
            int xx = balls[b].x;
            int yy = balls[b].y;
            balls[b].x += balls[b].dx;
            balls[b].y += balls[b].dy;
            if (balls[b].x >= DISP_W-16) {
                balls[b].x = DISP_W-16;
                balls[b].dx = NEG(balls[b].dx);
            } else if (balls[b].x < 0) {
                balls[b].x = 0;
                balls[b].dx = POS(balls[b].dx);
            }
            if (balls[b].y >= DISP_W-16) {
                balls[b].y = DISP_W-16;
                balls[b].dy = NEG(balls[b].dy);
            } else if (balls[b].y < 0) {
                balls[b].y = 0;
                balls[b].dy = POS(balls[b].dy);
            }
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 8; x++) {
                    g_buffer[xx/2 + (yy + y) * BYTES_PER_COL + x] |= balls_gfx[balls[b].color][y*8 + x];
                }
            }
        }
        disp_send_gbuf(0,0,DISP_W, DISP_H);
        uint32_t ticks = timer_read(1);
        if ((runs % 40) == 0) printf("fps: %f\n", 16000000.f/(float)ticks);
    }

    return 0;
}
CLI_FUNCTION(cli_demo_balls, "demo_balls", "(cycles (balls)): demo_balls");

static int cli_clr(int argc, const char **argv) {
    memset(g_buffer_a, 0, G_BUF_SIZE);
    memset(g_buffer_b, 0, G_BUF_SIZE);
    return 0;
}
CLI_FUNCTION(cli_clr, "clr", "clear");

static int cli_str(int argc, const char **argv) {
    char str[256];
    __builtin_memset(str, 0, 256);
    for (int i = 2; i < argc; i++) {
        sprintf(str + strlen(str), "%s ", argv[i]);
    }
    disp_str(str, strtol(argv[0],0,0), strtol(argv[1],0,0));
    return 0;
}
CLI_FUNCTION(cli_str, "str", "x y str: draw string");

static int cli_flush(int argc, const char **argv) {
    disp_send_gbuf(0,0,DISP_W, DISP_H);
    return 0;
}
CLI_FUNCTION(cli_flush, "flush", "flush gbuffer");


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

void GPIOTE_IRQHandler(void);
void GPIOTE_IRQHandler(void)
{
    NRF_GPIOTE->EVENTS_IN[0] = 0;
    int32_t vsync_now_tick = NRF_TIMER1->CC[0];
    vsync_last_delta_tick = vsync_now_tick - vsync_last_tick;
    last_vsync_freq = 16000000.f / (float)vsync_last_delta_tick;
    if (vsync_last_tick == 0 && last_vsync_freq != 0.f && vsync_last_delta_tick > 0) {
        printf("got vsync %f Hz, delta ticks:%d\n", last_vsync_freq, vsync_last_delta_tick);
    }
    vsync_last_tick = vsync_now_tick;
    vsync = 1;
}
