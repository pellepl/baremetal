#include <stdint.h>
#include "targets.h"
#include "gpio_driver.h"
#include "spi_prodtest.h"
#include "eventqueue.h"
#include "minio.h"
#include "ppg_afe4410.h"

#define ppg_order(x) ((((x) & 0xff) << 16) | ((x) & 0xff00) | ((x) & 0xff0000) << 16)

typedef union
{
    uint32_t raw;
    struct
    {
        uint8_t reg_raw;
        uint8_t value_raw[3];
    };
    struct __attribute__((packed))
    {
        uint8_t reg : 8;
        uint32_t value : 24;
    };
} afe4410_reg_t;

static struct
{
    bool init;
} me;

static void adc_ready_irq_handle(uint32_t event, void *arg)
{
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
    if (target->ppg.pin_adc_ready != BOARD_PIN_UNDEF)
    {
        gpio_irq_callback(target->ppg.pin_adc_ready, NULL);
        target_reset_pin(target->ppg.pin_adc_ready);
    }
    if (target->ppg.pin_reset != BOARD_PIN_UNDEF)
    {
        gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 0 : 1);
        target_reset_pin(target->ppg.pin_reset);
    }
    if (target->ppg.pin_interface_on != BOARD_PIN_UNDEF)
    {
        gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 0 : 1);
        target_reset_pin(target->ppg.pin_interface_on);
    }
    if (target->ppg.pin_vdd != BOARD_PIN_UNDEF)
    {
        gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 0 : 1);
        target_reset_pin(target->ppg.pin_vdd);
    }
}

static void enable_spi_ifc(bool enable)
{
    const target_t *target = target_get();
    if (!target->ppg.routed || target->ppg.pin_interface_on == BOARD_PIN_UNDEF)
    {
        return;
    }
    if (enable)
    {
        gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 1 : 0);
    }
    else
    {
        gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 0 : 1);
    }
}

static int write_registers(const afe4410_reg_t *registers, uint8_t count)
{
    int err;
    enable_spi_ifc(true);
    afe4410_reg_t cmd_write_reg = {.reg = 0x00, .value = ppg_order(0x000000)};
    err = spi_transceive_blocking(SPI_BUS_GENERIC, (const uint8_t *)&cmd_write_reg.raw, sizeof(afe4410_reg_t), NULL, 0);
    if (err)
        goto end;
    for (uint8_t ix = 0; err == 0 && ix < count; ix++)
    {
        err = spi_transceive_blocking(SPI_BUS_GENERIC, (const uint8_t *)&registers[ix].raw, sizeof(afe4410_reg_t), NULL, 0);
    }
end:
    enable_spi_ifc(false);
    return err;
}

static int read_registers(afe4410_reg_t *registers, uint8_t count)
{
    int err;
    enable_spi_ifc(true);
    afe4410_reg_t cmd_read_reg = {.reg = 0x00, .value = ppg_order(0x000001)};
    err = spi_transceive_blocking(SPI_BUS_GENERIC, (const uint8_t *)&cmd_read_reg.raw, sizeof(afe4410_reg_t), NULL, 0);
    if (err)
        goto end;
    for (uint8_t ix = 0; err == 0 && ix < count; ix++)
    {
        err = spi_transceive_blocking(SPI_BUS_GENERIC, &registers[ix].reg_raw, 1, registers[ix].value_raw, 3);
    }
end:
    enable_spi_ifc(false);
    return err;
}

int ppg_afe4410_init(void)
{
    const target_t *target = target_get();

    if (me.init)
    {
        return -EBADF;
    }

    if (!target->ppg.routed || target->ppg.type != PPG_TYPE_AFE4410)
    {
        return -ENOTSUP;
    }

    if (target->ppg.bus.bus_type != BUS_TYPE_SPI)
    {
        // only spi support implemented for now
        return -ENOSYS;
    }

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

    if (target->ppg.pin_vdd != BOARD_PIN_UNDEF)
    {
        gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 0 : 1);
        gpio_config(target->ppg.pin_vdd, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }

    if (target->ppg.pin_reset != BOARD_PIN_UNDEF)
    {
        gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 0 : 1);
        gpio_config(target->ppg.pin_reset, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }

    if (target->ppg.pin_interface_on != BOARD_PIN_UNDEF)
    {
        gpio_set(target->ppg.pin_interface_on, target->ppg.pin_interface_on_active_high ? 0 : 1);
        gpio_config(target->ppg.pin_interface_on, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }

    if (target->ppg.pin_adc_ready != BOARD_PIN_UNDEF)
    {
        gpio_config(target->ppg.pin_adc_ready, GPIO_DIRECTION_INPUT, GPIO_PULL_DOWN);
        gpio_irq_callback(target->ppg.pin_adc_ready, adc_ready_irq);
    }

    (void)ppg_afe4410_power(true);
    cpu_halt(10);
    (void)ppg_afe4410_reset(false);
    cpu_halt(10);

    me.init = true;

    return ERROR_OK;
}

int ppg_afe4410_power(bool enable)
{
    const target_t *target = target_get();
    if (!target->ppg.routed || target->ppg.pin_vdd == BOARD_PIN_UNDEF)
    {
        return -ENOTSUP;
    }
    if (enable)
    {
        gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 1 : 0);
    }
    else
    {
        gpio_set(target->ppg.pin_vdd, target->ppg.pin_vdd_active_high ? 0 : 1);
    }

    return ERROR_OK;
}

int ppg_afe4410_reset(bool enable)
{
    const target_t *target = target_get();
    if (!target->ppg.routed || target->ppg.pin_reset == BOARD_PIN_UNDEF)
    {
        return -ENOTSUP;
    }
    if (enable)
    {
        gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 1 : 0);
    }
    else
    {
        gpio_set(target->ppg.pin_reset, target->ppg.pin_reset_active_high ? 0 : 1);
    }

    return ERROR_OK;
}

void ppg_afe4410_deinit(void)
{
    if (!me.init)
    {
        return;
    }

    deinit();

    me.init = false;
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

static int cli_afe4410_read(int argc, const char **argv)
{
	if (argc < 1)
		return ERR_CLI_EINVAL;
    afe4410_reg_t reg = {
        .reg = strtol(argv[0], NULL, 0)
    };
	int err = read_registers(&reg, 1);
	printf("reg %02x == %06x\r\n", reg.reg, reg.value);
	return err;
}
CLI_FUNCTION(cli_afe4410_read, "afe4410_read", "read AFE4410 reg: <reg>");

static int cli_afe4410_write(int argc, const char **argv)
{
	if (argc < 2)
		return ERR_CLI_EINVAL;
    afe4410_reg_t reg = {
        .reg = strtol(argv[0], NULL, 0), .value = strtol(argv[1], NULL, 0)
    };
	int err = write_registers(&reg, 1);
	printf("reg %02x => %06x\r\n", reg.reg, reg.value);
	return err;
}
CLI_FUNCTION(cli_afe4410_write, "afe4410_write", "read AFE4410 reg: <reg> <val>");


#endif
