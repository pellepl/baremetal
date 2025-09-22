#include <errno.h>
#include <stdbool.h>
#include "common_util.h"
#include "disp_ch3613.h"
#include "disp_ch3613_registers.h"
#include "minio.h"
#include "spi_prodtest.h"
#include "targets.h"

extern uint32_t _RAM_START;
extern uint32_t _RAM_SIZE;

#define DISP_WIDTH 390
#define DISP_HEIGHT 390
#define DISP_CUTOUT_WIDTH 354
#define DISP_CUTOUT_HEIGHT 142
#define DISP_CUTOUT_X1 18
#define DISP_CUTOUT_Y1 25
#define BYTES_PER_PIXEL 0.5f
#define GBUF_SIZE (size_t)(DISP_WIDTH * DISP_HEIGHT * BYTES_PER_PIXEL)
#define NUMARGS(...) (sizeof((int[]){0, ##__VA_ARGS__}) / sizeof(int) - 1)
#define DISP_CMD_BLOCKING(cmd, ...)                                          \
    err = disp_write_cmd_blocking(cmd, NUMARGS(__VA_ARGS__), ##__VA_ARGS__); \
    if (err)                                                                 \
        goto end;
#define DCX_DATA 1
#define DCX_COMM 0

#define COL_BIPIXEL(c) ((((c) & 0xf) << 4) | ((c) & 0xf))
#define COL_L_PIXEL(c) (((c) & 0xf) << 4)
#define COL_R_PIXEL(c) ((c) & 0xf)
#define COL_L_MASK (0x0f)
#define COL_R_MASK (0xf0)
#define STRIDE_Y(y) ((DISP_HEIGHT + (y - 1)) * -(int)(DISP_WIDTH * BYTES_PER_PIXEL))

// The screen is not centered so we must tweak the coords a bit
#define CASET_X_OFFSET 6

static const uint32_t PALETTE_STANDARD[16] = {
    0x000000, 0x444444, 0xaaaaaa, 0xffffff,
    0x004400, 0x008800, 0x00aa00, 0x00ff00,
    0x440000, 0x880000, 0xaa0000, 0xff0000,
    0x000044, 0x000088, 0x0000aa, 0x0000ff};

static struct
{
    bool init;
    uint8_t *gbuf;
} me;

static void dcx(int data_else_comm)
{
    const target_t *target = target_get();
    gpio_set(target->display.pin_dc, data_else_comm);
}

static void disp_power(bool enable)
{
    const target_t *target = target_get();
    if (enable)
        gpio_set(target->display.pin_vdd, target->display.pin_vdd_active_high ? 1 : 0);
    else
        gpio_set(target->display.pin_vdd, target->display.pin_vdd_active_high ? 0 : 1);
}

static void disp_reset(bool enable)
{
    const target_t *target = target_get();
    if (enable)
        gpio_set(target->display.pin_reset, target->display.pin_reset_active_high ? 1 : 0);
    else
        gpio_set(target->display.pin_reset, target->display.pin_reset_active_high ? 0 : 1);
}

static int disp_write_cmd_blocking_arr(const uint8_t *cmd_arr, uint32_t arr_len)
{
    int err;
    dcx(DCX_COMM);
    err = spi_transceive_blocking(SPI_BUS_FAST, cmd_arr, 1, NULL, 0);
    if (err)
        goto end;
    dcx(DCX_DATA);

    if (arr_len > 1)
        err = spi_transceive_blocking(SPI_BUS_FAST, cmd_arr + 1, arr_len - 1, NULL, 0);
end:
    dcx(DCX_DATA);
    return err;
}

static int disp_write_cmd_blocking(uint8_t cmd, uint32_t args_count, ...)
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
        if (arg_nbr >= sizeof(spi_cmd_buffer) - 1)
            return -E2BIG;
    }
    return disp_write_cmd_blocking_arr(spi_cmd_buffer, args_count + 1);
}

static void deinit(void)
{
    const target_t *target = target_get();
    if (!target->display.routed)
        return;

    if (target->display.bus.bus_type == BUS_TYPE_SPI)
        spi_deinit(SPI_BUS_FAST);
    gpio_irq_callback(target->display.pin_tearing, NULL);
    target_reset_pin(target->display.pin_tearing);
    disp_reset(true);
    target_reset_pin(target->display.pin_reset);
    disp_power(false);
    target_reset_pin(target->display.pin_vdd);
    target_reset_pin(target->display.pin_dc);
}

int disp_ch3613_init(void)
{
    const target_t *target = target_get();

    if (me.init)
        return -EBADF;

    if (!target->display.routed || target->display.type != DISPLAY_TYPE_CH3613)
        return -ENOTSUP;

    if (target->display.bus.bus_type != BUS_TYPE_SPI)
        return -ENOSYS;

    memset(&me, 0, sizeof(me));

    gpio_set(target->display.pin_vdd, target->display.pin_vdd_active_high ? 0 : 1);
    gpio_set(target->display.pin_reset, target->display.pin_reset_active_high ? 1 : 0);
    gpio_set(target->display.bus.spi.pin_cs, target->display.bus.spi.pin_cs_active_high ? 0 : 1);
    gpio_config(target->display.pin_reset, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(target->display.pin_dc, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(target->display.bus.spi.pin_cs, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(target->display.pin_tearing, GPIO_DIRECTION_INPUT, GPIO_PULL_UP);
    gpio_config(target->display.pin_vdd, GPIO_DIRECTION_OUTPUT, GPIO_PULL_UP);

    spi_config_t spi_cfg = {
        .pins.clk = target->display.bus.spi.pin_clk,
        .pins.csn = target->display.bus.spi.pin_cs,
        .pins.miso = target->display.bus.spi.pin_miso,
        .pins.mosi = target->display.bus.spi.pin_mosi,
        .clk_freq = target->display.bus.bus_speed_hz,
        .mode = target->display.bus.spi.mode};

    int err = spi_init(SPI_BUS_FAST, &spi_cfg);
    if (err)
    {
        deinit();
        return err;
    }

    uint32_t ram_start = (uint32_t)(intptr_t)&_RAM_START;
    uint32_t ram_size = (uint32_t)(intptr_t)&_RAM_SIZE;
    // place display gbuffer directly after stack
    me.gbuf = (void *)(intptr_t)((ram_start + ram_size + 3) & ~3);
    me.init = true;
    return 0;
}

void disp_ch3613_deinit(void)
{
    if (!me.init)
        return;

    deinit();

    me.init = false;
}

int disp_ch3613_setup(void)
{
    if (!me.init)
        return -EBADF;

    // wait more than 10 ms before releasing reset
    disp_reset(true);
    disp_power(true);
    cpu_halt(50);
    disp_reset(false);
    cpu_halt(15);
    disp_reset(true);
    cpu_halt(5);
    disp_reset(false);
    cpu_halt(15);

    uint8_t SPI111_OPT = 0 << 2;
    uint8_t SPI4BIT_EN = 1 << 3;
    int err;
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
    DISP_CMD_BLOCKING(CH13613_USER_SDC, 0x02); // Flip screen horizontally
    DISP_CMD_BLOCKING(CH13613_USER_IPF, 0x01); // SPI111  // 0x05); 16 bit
    DISP_CMD_BLOCKING(CH13613_USER_SLPOUT);
    cpu_halt(70);
    DISP_CMD_BLOCKING(CH13613_USER_WDB, 0xff);
    DISP_CMD_BLOCKING(CH13613_USER_DISPON);
    disp_ch3613_palette(PALETTE_STANDARD);

end:
    return err;
}

int disp_ch3613_palette(const uint32_t *p)
{
    if (!me.init)
        return -EBADF;
    // send palette in four batches, first three commands take 5 colours, last takes one colour (5+5+5+1)
    uint8_t cmd[1 + 3 * 5];
    int err = 0;
    for (int command = CH13613_MP0_COLMS1; err == 0 && command < CH13613_MP0_COLMS4; command++)
    {
        cmd[0] = command;
        for (uint32_t i = 0; i < 5; i++)
        {
            uint32_t col = *p++;
            cmd[1 + i * 3] = (col >> 16) & 0xff;
            cmd[2 + i * 3] = (col >> 8) & 0xff;
            cmd[3 + i * 3] = (col >> 0) & 0xff;
        }
        err = disp_write_cmd_blocking_arr(cmd, sizeof(cmd));
    }
    cmd[0] = CH13613_MP0_COLMS4;
    uint32_t col = *p++;
    cmd[1] = (col >> 16) & 0xff;
    cmd[2] = (col >> 8) & 0xff;
    cmd[3] = (col >> 0) & 0xff;
    err = disp_write_cmd_blocking_arr(cmd, 1 + 3);
    return err;
}

int disp_ch3613_clear(uint8_t col)
{
    return disp_ch3613_fill(0, 0, DISP_WIDTH - 1, DISP_HEIGHT - 1, col);
}

int disp_ch3613_fill(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t col)
{
    if (x1 > x2 || y2 > y1)
        return 0;
    x1 = clamp(x1, 0, DISP_WIDTH - 1);
    x2 = clamp(x2, 0, DISP_WIDTH - 1);
    y1 = clamp(y1, 0, DISP_HEIGHT - 1);
    y2 = clamp(y2, 0, DISP_HEIGHT - 1);
    int w = x2 - x1 + 1;
    uint8_t *gram;

    for (int y = y1; y <= y2; y++)
    {
        int w_rem = w;
        gram = me.gbuf + STRIDE_Y(y) + (int)(x1 * BYTES_PER_PIXEL);
        if (x1 & 1)
        {
            *gram = (*gram & COL_R_MASK) | COL_R_PIXEL(col);
            gram++;
            w_rem--;
        }
        if (w_rem >= 2)
        {
            memset(gram, COL_BIPIXEL(col), (int)(w_rem * BYTES_PER_PIXEL));
            gram += (int)(w_rem * BYTES_PER_PIXEL);
        }
        if ((x2 & 1) == 0)
        {
            *gram = (*gram & COL_L_MASK) | COL_L_PIXEL(col);
        }
    }

    return -ENOTSUP;
}

int disp_ch3613_flush(void)
{
    return -ENOTSUP;
}

int disp_ch3613_string(const char *str, int16_t x, int16_t y, uint8_t colour)
{
}

#if NO_EXTRA_CLI == 0

#include "cli.h"

static int cli_disp_init(int argc, const char **argv)
{
    int err = disp_ch3613_init();
    return err;
}
CLI_FUNCTION(cli_disp_init, "ch3613_init", "initialize CH3613 display driver");

static int cli_disp_setup(int argc, const char **argv)
{
    int err = disp_ch3613_setup();
    return err;
}
CLI_FUNCTION(cli_disp_setup, "ch3613_setup", "setup CH3613 display");

static int cli_disp_command(int argc, const char **argv)
{
    if (argc <= 0)
        return ERR_CLI_EINVAL;
    if (!me.init) return -EBADF;
    uint8_t data[argc];
    for (int i = 0; i < argc; i++)
        data[i] = strtol(argv[i], NULL, 0);
    return disp_write_cmd_blocking_arr(data, argc);
}
CLI_FUNCTION(cli_disp_command, "ch3613_cmd", "send CH3613 command: <x>*");

static int cli_disp_deinit(int argc, const char **argv)
{
    disp_ch3613_deinit();
    return 0;
}
CLI_FUNCTION(cli_disp_deinit, "ch3613_deinit", "deinitialize CH3613 display driver");

#endif
