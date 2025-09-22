#include <stdint.h>
#include "targets.h"
#include "gpio_driver.h"
#include "spi_prodtest.h"
#include "eventqueue.h"
#include "minio.h"
#include "ppg_afe4410.h"

static struct
{
    bool init;
    uint32_t irq_count;
} me;

static inline uint32_t bits(uint32_t x, uint8_t num_bits, uint8_t shift)
{
    uint32_t mask = (num_bits >= 32 ? 0xffffffff : ((1u << num_bits) - 1u)) << shift;
    return (x & mask) >> shift;
}

// Sequence sniffed from CSEM. Commented by ChatGPT.
static const afe4410_reg_t csem_setup_sequence[] = {
    // Global/timing, interface, misc control
    {.reg = 0x1e, .uval = 0x000102},
    {.reg = 0x23, .uval = 0x12c218},
    {.reg = 0x29, .uval = 0x000000},
    {.reg = 0x42, .uval = 0x000000},
    {.reg = 0x51, .uval = 0x000000},
    {.reg = 0x31, .uval = 0x000020},
    {.reg = 0x4b, .uval = 0x000000},
    {.reg = 0x39, .uval = 0x000000},

    // TIA gain/CF and filter bandwidth
    {.reg = 0x1f, .uval = 0x001111},
    {.reg = 0x20, .uval = 0x008011},
    {.reg = 0x21, .uval = 0x000011},

    // Offset DACs (photodiode dark/light cancellation)
    {.reg = 0x3a, .uval = 0x100000},
    {.reg = 0x3e, .uval = 0x000000},
    {.reg = 0x22, .uval = 0x000000},
    {.reg = 0x4e, .uval = 0x000000},

    // FIFO / watermark / alerts
    {.reg = 0x3d, .uval = 0x000024},
    {.reg = 0x72, .uval = 0x0000f0},
    {.reg = 0x50, .uval = 0x000018},
    {.reg = 0x6c, .uval = 0x000800},

    // Timing engine: LED windows & ADC conversion windows (period builder)
    {.reg = 0x01, .uval = 0x00002c}, // LED2STC      start time for LED2 sample window
    {.reg = 0x02, .uval = 0x000032}, // LED2ENDC     end time for LED2 sample window
    {.reg = 0x03, .uval = 0x000026}, // LED2LEDSTC   turn LED2 ON at this count
    {.reg = 0x04, .uval = 0x000032}, // LED2LEDENDC  turn LED2 OFF here
    {.reg = 0x05, .uval = 0x000065}, // ALED2STC     start Ambient2 sample
    {.reg = 0x06, .uval = 0x00006b}, // ALED2ENDC    end Ambient2 sample
    {.reg = 0x07, .uval = 0x00009e}, // LED1STC      start LED1 sample
    {.reg = 0x08, .uval = 0x0000a4}, // LED1ENDC     end LED1 sample
    {.reg = 0x09, .uval = 0x000098}, // LED1LEDSTC   LED1 ON start
    {.reg = 0x0a, .uval = 0x0000a4}, // LED1LEDENDC  LED1 ON end
    {.reg = 0x0b, .uval = 0x0000d7}, // ALED1STC     Ambient1 sample start
    {.reg = 0x0c, .uval = 0x0000dd}, // ALED1ENDC    Ambient1 sample end
    {.reg = 0x0d, .uval = 0x000039}, // LED2CONVST   ADC conversion start for LED2 slot
    {.reg = 0x0e, .uval = 0x00005f}, // LED2CONVEND  ADC conversion end for LED2 slot
    {.reg = 0x0f, .uval = 0x000072}, // ALED2CONVST  ADC conversion start for Ambient2 slot
    {.reg = 0x10, .uval = 0x000098}, // ALED2CONVEND ADC conversion end for Ambient2 slot
    {.reg = 0x11, .uval = 0x0000ab}, // LED1CONVST   ADC conversion start for LED1 slot
    {.reg = 0x12, .uval = 0x0000d1}, // LED1CONVEND  ADC conversion end for LED1 slot
    {.reg = 0x13, .uval = 0x0000e4}, // ALED1CONVST  ADC conversion start for Ambient1 slot
    {.reg = 0x14, .uval = 0x00010a}, // ALED1CONVEND ADC conversion end for Ambient1 slot
    {.reg = 0x1d, .uval = 0x0004ff}, // PRPCT        period count — total frame length, sets repetition

    // LED3 extra timing window (none)
    {.reg = 0x34, .uval = 0x00ffff},
    {.reg = 0x35, .uval = 0x00ffff},
    {.reg = 0x36, .uval = 0x0000d1},
    {.reg = 0x37, .uval = 0x0000dd},

    // Target/trigger windows for threshold mode
    {.reg = 0x43, .uval = 0x00005f},
    {.reg = 0x44, .uval = 0x00006b},
    {.reg = 0x45, .uval = 0x000026},
    {.reg = 0x46, .uval = 0x0000dd},
    {.reg = 0x47, .uval = 0x00ffff},
    {.reg = 0x48, .uval = 0x00ffff},
    {.reg = 0x49, .uval = 0x00ffff},
    {.reg = 0x4a, .uval = 0x00ffff},

    // Data-ready window (when DATA_RDY asserts)
    {.reg = 0x52, .uval = 0x000110},
    {.reg = 0x53, .uval = 0x000113},

    // Programmable interrupt 1 (optional heartbeat pulse each period)
    {.reg = 0x57, .uval = 0x00c350},
    {.reg = 0x58, .uval = 0x00c350},

    // Dynamic reconfig windows (when analog settings latch)
    {.reg = 0x64, .uval = 0x000000},
    {.reg = 0x65, .uval = 0x000118},
    {.reg = 0x66, .uval = 0x000000},
    {.reg = 0x67, .uval = 0x000118},
    {.reg = 0x68, .uval = 0x000000},
    {.reg = 0x69, .uval = 0x000118},
    {.reg = 0x6a, .uval = 0x000118},
    {.reg = 0x6b, .uval = 0x0004e6},

    // LED4 current & alt configs (not used here)
    {.reg = 0x24, .uval = 0x000000},

    // Re-program phase-separate TIA again for final profile
    {.reg = 0x1f, .uval = 0x003131},
    {.reg = 0x20, .uval = 0x008031},
    {.reg = 0x21, .uval = 0x000031},

    // LED driver path selection reset back to DRV1
    {.reg = 0x72, .uval = 0x000000},

};

static void adc_ready_irq_handle(uint32_t event, void *arg)
{
    me.irq_count++;
}

static void adc_ready_irq(uint16_t pin, uint8_t state)
{
    if (!me.init)
        return;
    eventq_add(0, 0, adc_ready_irq_handle);
}

static void deinit(void)
{
    const target_t *target = target_get();
    if (!target->ppg.routed)
        return;

    if (target->ppg.bus.bus_type == BUS_TYPE_SPI)
    {
        spi_deinit(SPI_BUS_GENERIC);
    }
    gpio_irq_callback(target->ppg.pin_adc_ready, NULL);
    target_reset_pin(target->ppg.pin_adc_ready);
    gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 1 : 0);
    target_reset_pin(target->ppg.pin_reset);
    gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 0 : 1);
    target_reset_pin(target->ppg.pin_interface_on);
    gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 0 : 1);
    target_reset_pin(target->ppg.pin_vdd);
}

static void enable_spi_ifc(bool enable)
{
    const target_t *target = target_get();
    if (!target->ppg.routed || target->ppg.pin_interface_on == BOARD_PIN_UNDEF)
        return;
    if (enable)
        gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 1 : 0);
    else
        gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 0 : 1);
}

static int spi_write_reg(afe4410_reg_t reg)
{
    uint8_t tx[4] = {reg.reg,
                     reg.uval >> 16,
                     reg.uval >> 8,
                     reg.uval};
    return spi_transceive_blocking(SPI_BUS_GENERIC, tx, 4, NULL, 0);
}

static int spi_read_reg(afe4410_reg_t *reg)
{
    uint8_t tx[1] = {reg->reg};
    uint8_t rx[4] = {0x00, 0xef, 0xef, 0xef};
    int err = spi_transceive_blocking(SPI_BUS_GENERIC, tx, 1, rx, 4);
    if (!err)
        reg->uval = (rx[1] << 16) | (rx[2] << 8) | (rx[3]);
    return err;
}

static int write_registers(const afe4410_reg_t *registers, uint8_t count)
{
    int err;
    enable_spi_ifc(true);
    err = spi_write_reg((afe4410_reg_t){.reg = 0x00, .uval = 0x000020});
    if (err)
        goto end;
    for (uint8_t ix = 0; err == 0 && ix < count; ix++)
        err = spi_write_reg(registers[ix]);
    if (err)
        goto end;
    err = spi_write_reg((afe4410_reg_t){.reg = 0x00, .uval = 0x000021});
end:
    enable_spi_ifc(false);
    return err;
}

static int read_registers(afe4410_reg_t *registers, uint8_t count)
{
    int err;
    enable_spi_ifc(true);
    err = spi_write_reg((afe4410_reg_t){.reg = 0x00, .uval = 0x000021});
    if (err)
        goto end;
    for (uint8_t ix = 0; err == 0 && ix < count; ix++)
        err = spi_read_reg(&registers[ix]);
end:
    enable_spi_ifc(false);
    return err;
}

static int read_led_currents(afe4410_led_currents_t *res)
{
    afe4410_reg_t reg = {.reg = 0x22};
    int err = read_registers(&reg, 1);
    if (err)
        return err;
    res->iled1 = (bits(reg.uval, 6, 0) << 2) | bits(reg.uval, 2, 18);
    res->iled2 = (bits(reg.uval, 6, 6) << 2) | bits(reg.uval, 2, 20);
    res->iled3 = (bits(reg.uval, 6, 12) << 2) | bits(reg.uval, 2, 22);
    return err;
}

static int write_led_currents(const afe4410_led_currents_t *curr)
{
    uint32_t val = (bits(curr->iled1, 6, 2) << 0) | (bits(curr->iled1, 2, 6) << 18) |
                   (bits(curr->iled2, 6, 2) << 6) | (bits(curr->iled2, 2, 6) << 20) |
                   (bits(curr->iled3, 6, 2) << 12) | (bits(curr->iled3, 2, 6) << 22);
    afe4410_reg_t reg = {.reg = 0x22, .uval = val};
    return write_registers(&reg, 1);
}

static int read_adc(afe4410_adc_values_t *res)
{
    afe4410_reg_t regs[] = {{.reg = 0x2c}, {.reg = 0x2d}, {.reg = 0x2a}, {.reg = 0x2b}, {.reg = 0x40}, {.reg = 0x3f}};
    int err = read_registers(regs, 6);
    if (err)
        return err;
    res->led1 = regs[0].sval;
    res->led1_ambient = regs[1].sval;
    res->led2 = regs[2].sval;
    res->led2_ambient = regs[3].sval;
    res->avg_delta_led_amb_1 = regs[4].sval;
    res->avg_delta_led_amb_2 = regs[5].sval;
    return err;
}

static int read_dac(afe4410_dac_t *res)
{
    afe4410_reg_t regs[] = {{.reg = 0x3a}, {.reg = 0x3e}, {.reg = 0x21}};
    int err = read_registers(regs, 3);
    if (err)
        return err;
    uint32_t r_3a = regs[0].uval;
    uint32_t r_3e = regs[1].uval;
    res->early_offset = (r_3a & (1 << 20)) != 0;
    res->multiplier = bits(regs[2].uval, 3, 12);
    // thanks TI, really!
    res->led1 = (bits(r_3a, 1, 9) << 7) | // sign bit
                (bits(r_3e, 1, 3) << 6) | // msb
                (bits(r_3a, 4, 5) << 2) | // mid bits
                (bits(r_3e, 1, 2) << 1) | // lsb
                (bits(r_3e, 1, 9) << 0);  // real lsb :D
    res->amb1 = (bits(r_3a, 1, 14) << 7) |
                (bits(r_3e, 1, 5) << 6) |
                (bits(r_3a, 4, 10) << 2) |
                (bits(r_3e, 1, 4) << 1) |
                (bits(r_3e, 1, 10) << 0);
    res->led2 = (bits(r_3a, 1, 19) << 7) |
                (bits(r_3e, 1, 7) << 6) |
                (bits(r_3a, 4, 15) << 2) |
                (bits(r_3e, 1, 6) << 1) |
                (bits(r_3e, 1, 11) << 0);
    res->amb2 = (bits(r_3a, 1, 4) << 7) |
                (bits(r_3e, 1, 1) << 6) |
                (bits(r_3a, 4, 0) << 2) |
                (bits(r_3e, 1, 0) << 1) |
                (bits(r_3e, 1, 8) << 0);
    return err;
}

static int write_dac(const afe4410_dac_t *dac)
{
    uint32_t r_3a =
        (dac->early_offset ? (1 << 20) : 0) |
        (bits(dac->led1, 1, 7) << 9) |
        (bits(dac->led1, 4, 2) << 5) |
        (bits(dac->amb1, 1, 7) << 14) |
        (bits(dac->amb1, 4, 2) << 10) |
        (bits(dac->led2, 1, 7) << 19) |
        (bits(dac->led2, 4, 2) << 15) |
        (bits(dac->amb2, 1, 7) << 4) |
        (bits(dac->amb2, 4, 2) << 0);
    uint32_t r_3e =
        (bits(dac->led1, 1, 6) << 3) |
        (bits(dac->led1, 1, 1) << 2) |
        (bits(dac->led1, 1, 0) << 9) |
        (bits(dac->amb1, 1, 6) << 5) |
        (bits(dac->amb1, 1, 1) << 4) |
        (bits(dac->amb1, 1, 0) << 10) |
        (bits(dac->led2, 1, 6) << 7) |
        (bits(dac->led2, 1, 1) << 6) |
        (bits(dac->led2, 1, 0) << 11) |
        (bits(dac->amb2, 1, 6) << 1) |
        (bits(dac->amb2, 1, 1) << 0) |
        (bits(dac->amb2, 1, 0) << 8);
    afe4410_reg_t regs[] = {{.reg = 0x3a, .uval = r_3a}, {.reg = 0x3e, .uval = r_3e}};
    int err = write_registers(regs, 2);
    // TODO register 0x21?
    return err;
}

int ppg_afe4410_init(void)
{
    const target_t *target = target_get();

    if (me.init)
        return -EBADF;

    if (!target->ppg.routed || target->ppg.type != PPG_TYPE_AFE4410)
        return -ENOTSUP;

    if (target->ppg.bus.bus_type != BUS_TYPE_SPI)
        // only spi support implemented for now
        return -ENOSYS;

    memset(&me, 0, sizeof(me));

    spi_config_t spi_cfg = {
        .pins.clk = target->ppg.bus.spi.pin_clk,
        .pins.csn = target->ppg.bus.spi.pin_cs,
        .pins.miso = target->ppg.bus.spi.pin_miso,
        .pins.mosi = target->ppg.bus.spi.pin_mosi,
        .clk_freq = target->ppg.bus.bus_speed_hz,
        .mode = target->ppg.bus.spi.mode};

    int err = spi_init(SPI_BUS_GENERIC, &spi_cfg);
    if (err)
    {
        deinit();
        return err;
    }

    gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 0 : 1);
    gpio_config(target->ppg.pin_vdd, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 0 : 1);
    gpio_config(target->ppg.pin_reset, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 0 : 1);
    gpio_config(target->ppg.pin_interface_on, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    gpio_config(target->ppg.pin_adc_ready, GPIO_DIRECTION_INPUT, GPIO_PULL_DOWN);
    gpio_irq_callback(target->ppg.pin_adc_ready, adc_ready_irq);

    (void)ppg_afe4410_power(true);
    cpu_halt(10);
    (void)ppg_afe4410_reset(false);
    cpu_halt(10);

    afe4410_reg_t reg_wr = {.reg = 0x01, .uval = 0x00abcd};
    afe4410_reg_t reg_rd = {.reg = 0x01, .uval = 0x123456};
    err = write_registers(&reg_wr, 1);
    if (!err)
        err = read_registers(&reg_rd, 1);
    if (!err && reg_wr.uval != reg_rd.uval)
        err = -ENOLINK;
    if (err)
        deinit();
    else
    {
        reg_wr.uval = 0; // reset uval
        write_registers(&reg_wr, 1);
        me.init = true;
    }

    return err;
}

int ppg_afe4410_power(bool enable)
{
    const target_t *target = target_get();
    if (!target->ppg.routed || target->ppg.pin_vdd == BOARD_PIN_UNDEF)
        return -ENOTSUP;
    if (enable)
        gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 1 : 0);
    else
        gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 0 : 1);

    return ERROR_OK;
}

int ppg_afe4410_reset(bool enable)
{
    const target_t *target = target_get();
    if (!target->ppg.routed || target->ppg.pin_reset == BOARD_PIN_UNDEF)
        return -ENOTSUP;
    if (enable)
        gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 1 : 0);
    else
        gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 0 : 1);

    return ERROR_OK;
}

void ppg_afe4410_deinit(void)
{
    if (!me.init)
        return;

    deinit();

    me.init = false;
}

int ppg_afe4410_write(const afe4410_reg_t *reg)
{
    if (!me.init)
        return -EBADF;
    return write_registers(reg, 1);
}
int ppg_afe4410_read(afe4410_reg_t *reg)
{
    if (!me.init)
        return -EBADF;
    return read_registers(reg, 1);
}

int ppg_afe4410_write_dac(const afe4410_dac_t *dac)
{
    if (!me.init)
        return -EBADF;
    return write_dac(dac);
}

int ppg_afe4410_read_dac(afe4410_dac_t *dac)
{
    if (!me.init)
        return -EBADF;
    return read_dac(dac);
}

int ppg_afe4410_write_iled(const afe4410_led_currents_t *led)
{
    if (!me.init)
        return -EBADF;
    return write_led_currents(led);
}

int ppg_afe4410_read_iled(afe4410_led_currents_t *led)
{
    if (!me.init)
        return -EBADF;
    return read_led_currents(led);
}

int ppg_afe4410_read_adc(afe4410_adc_values_t *adc)
{
    if (!me.init)
        return -EBADF;
    return read_adc(adc);
}

int ppg_afe4410_setup(void)
{
    if (!me.init)
        return -EBADF;
    int err;
    err = write_registers(csem_setup_sequence, sizeof(csem_setup_sequence) / sizeof(csem_setup_sequence[0]));
    if (err)
        return err;
    afe4410_reg_t reg_wr = {.reg = 0x22, .uval = 0x280fff};
    err = write_registers(&reg_wr, 1);

    return err;
}

#if NO_EXTRA_CLI == 0

#include "cli.h"

static int cli_afe4410_init(int argc, const char **argv)
{
    int err = ppg_afe4410_init();
    return err;
}
CLI_FUNCTION(cli_afe4410_init, "afe4410_init", "initialize AFE4410 driver");

static int cli_afe4410_power(int argc, const char **argv)
{
    bool enable = strtol(argv[0], NULL, 0);
    return ppg_afe4410_power(enable);
}
CLI_FUNCTION(cli_afe4410_power, "afe4410_power", "AFE4410 power on/off: 0|1");

static int cli_afe4410_reset(int argc, const char **argv)
{
    bool enable = strtol(argv[0], NULL, 0);
    return ppg_afe4410_reset(enable);
}
CLI_FUNCTION(cli_afe4410_reset, "afe4410_reset", "AFE4410 reset on/off: 0|1");

static int cli_afe4410_deinit(int argc, const char **argv)
{
    ppg_afe4410_deinit();
    return ERROR_OK;
}
CLI_FUNCTION(cli_afe4410_deinit, "afe4410_deinit", "stop AFE4410 driver");

static int cli_afe4410_setup(int argc, const char **argv)
{
    return ppg_afe4410_setup();
}
CLI_FUNCTION(cli_afe4410_setup, "afe4410_setup", "setup AFE4410");

static int cli_afe4410_read(int argc, const char **argv)
{
    if (argc < 1)
        return ERR_CLI_EINVAL;
    afe4410_reg_t reg = {
        .reg = strtol(argv[0], NULL, 0)};
    int err = ppg_afe4410_read(&reg);
    printf("reg %02x == %06x\r\n", reg.reg, reg.uval);
    return err;
}
CLI_FUNCTION(cli_afe4410_read, "afe4410_read", "read AFE4410 reg: <reg>");

static int cli_afe4410_write(int argc, const char **argv)
{
    if (argc < 2)
        return ERR_CLI_EINVAL;
    afe4410_reg_t reg = {
        .reg = strtol(argv[0], NULL, 0), .uval = strtol(argv[1], NULL, 0)};
    int err = ppg_afe4410_write(&reg);
    printf("reg %02x => %06x\r\n", reg.reg, reg.uval);
    return err;
}
CLI_FUNCTION(cli_afe4410_write, "afe4410_write", "read AFE4410 reg: <reg> <val>");

static int cli_afe4410_read_adc(int argc, const char **argv)
{
    bool hex = argc > 0 && argv[0][0] == 'h';
    afe4410_adc_values_t adc = {0};
    int err = ppg_afe4410_read_adc(&adc);
    if (hex)
    {
        printf("led1:%06x\tamb1:%06x\tled2:%06x\tamb2:%06x\tavg[led1-amb1]:%06x\tavg[led2-amb2]:%06x\r\n",
               adc.led1, adc.led1_ambient, adc.led2, adc.led2_ambient, adc.avg_delta_led_amb_1, adc.avg_delta_led_amb_2);
    }
    else
    {
        printf("led1:%d\tamb1:%d\tled2:%d\tamb2:%d\tavg[led1-amb1]:%d\tavg[led2-amb2]:%d\r\n",
               adc.led1, adc.led1_ambient, adc.led2, adc.led2_ambient, adc.avg_delta_led_amb_1, adc.avg_delta_led_amb_2);
    }
    return err;
}
CLI_FUNCTION(cli_afe4410_read_adc, "afe4410_adc", "read AFE4410 adc values: (hex)");

static int cli_afe4410_dac(int argc, const char **argv)
{
    afe4410_dac_t dac;
    int err = ppg_afe4410_read_dac(&dac);
    if (err)
        return err;
    for (int i = 0; i < argc; i += 2)
    {
        if (0 == strcmp("led1", argv[i]))
            dac.led1 = strtol(argv[i + 1], NULL, 0);
        else if (0 == strcmp("led2", argv[i]))
            dac.led2 = strtol(argv[i + 1], NULL, 0);
        else if (0 == strcmp("amb1", argv[i]) || 0 == strcmp("led3", argv[i]))
            dac.amb1 = strtol(argv[i + 1], NULL, 0);
        else if (0 == strcmp("amb2", argv[i]))
            dac.amb2 = strtol(argv[i + 1], NULL, 0);
        else if (0 == strcmp("eo", argv[i]))
            dac.early_offset = strtol(argv[i + 1], NULL, 0);
        else
            return -ERR_CLI_EINVAL;
    }
    if (argc > 0)
        err = write_dac(&dac);
    printf("DAC\tled1:%+3d\tamb1:%+3d\tled2:%+3d\tamb2:%+3d\tearly_offs:%d\tmul:%dX\n",
           dac.led1, dac.amb1, dac.led2, dac.amb2, dac.early_offset, (int[]){1, 0, 0, 2, 0, 4, 0, 8}[dac.multiplier]);
    return err;
}
CLI_FUNCTION(cli_afe4410_dac, "afe4410_dac", "read/write dac: (led1|led2|amb1|amb2|eo <value>)*");

static int cli_afe4410_led(int argc, const char **argv)
{
    afe4410_led_currents_t led;
    int err = ppg_afe4410_read_iled(&led);
    if (err)
        return err;
    for (int i = 0; i < argc; i += 2)
    {
        if (0 == strcmp("led1", argv[i]))
            led.iled1 = strtol(argv[i + 1], NULL, 0);
        else if (0 == strcmp("led2", argv[i]))
            led.iled2 = strtol(argv[i + 1], NULL, 0);
        else if (0 == strcmp("led3", argv[i]))
            led.iled3 = strtol(argv[i + 1], NULL, 0);
        else
            return -ERR_CLI_EINVAL;
    }
    if (argc > 0)
        err = write_led_currents(&led);
    printf("CURR\tled1:%3d\tled2:%3d\tled3:%3d\n", led.iled1, led.iled2, led.iled3);
    return err;
}
CLI_FUNCTION(cli_afe4410_led, "afe4410_led", "read/write led current: (led1|led2|led3 <value>)*");

#endif
