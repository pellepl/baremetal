#include "targets.h"
#include "bma400_def.h"
#include "gpio_driver.h"
#include "spi_prodtest.h"
#include "eventqueue.h"
#include "minio.h"
#include "accelerometer_bma400.h"

#define FIFO_SIZE   512

static struct {
    bool init;
    bool debug;
    uint8_t id;
    volatile uint32_t irqs;
    uint8_t fifo_buf[FIFO_SIZE];
    int samples;
    int16_t sample_buf[FIFO_SIZE/sizeof(int16_t)];
} me;


/**
 * Decode BMA400 frame data to proper signed 16bit int16_t[3] x,y,z accelerometer data, 
 * with 12 bits of information.
 * Beware that if encoded_data and raw_data is the same buffer, it will only work if the
 * BMA400 is configured in 12-bit mode, sampling all three axes. Otherwise the raw data
 * buffer size will exceed that of the encoded_data and we will overwrite encoded data 
 * before reading it.
 * See https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bma400-ds000.pdf p 31
 * @param encoded_data 		the fifo buffer as retrieved by the BMA400
 * @param encoded_data_len	nbr of encoded bytes
 * @param raw_samples 		where to put signed 16 bit samples, in x y z format
 * @return number of 16 bit samples, will always be a factor of 3 or negative on error
 */
static int bma400_decode_fifo(const uint8_t *encoded_data, const uint16_t encoded_data_len, int16_t *raw_samples)
{
    int ret = 0;
	uint16_t data_ix = 0;
	while (data_ix < encoded_data_len) {
		uint8_t header = encoded_data[data_ix];
		data_ix += 1;
		// only interested in sensor data frames. Drop all time, control, and empty frames.

		if ((header & 0b11100000) == 0b10000000) {
			// data frame, 1-6 data bytes follows

			if ((header & 0b00001110) != 0) { // non-empty data frame
				// 8 or 12 bit samples
				uint8_t spl_size_in_bytes = (header & 0b00010000) == 0 ? 1 : 2;
				for (int axis = 0; axis < 3; axis++) {
					int16_t raw_sample = 0;
					// see what axes are enabled (0=x, 1=y, 2=z)
					if ((header & (0b00000010 << axis)) != 0) {
						if (data_ix + spl_size_in_bytes > encoded_data_len) {
							return -EIO;
						}
						uint8_t data_msb;
						uint8_t data_lsb = 0;
						if (spl_size_in_bytes == 2) {
							// 12-bit format
							data_lsb = encoded_data[data_ix++];
						}
						data_msb = encoded_data[data_ix++];
						raw_sample = (int16_t)(((uint16_t)(data_msb << 4)) | data_lsb);
						if (raw_sample > 2047) {
							raw_sample -= 4096;
						}
					}
					raw_samples[axis] = raw_sample;
				} // per axis

				raw_samples += 3;
				ret += 3;
			} else {
				// a so called empty data frame, 1 zero byte follows which we promptly ignore
				if (data_ix + 1 > encoded_data_len) {
					return -EIO;
				}
				data_ix += 1;
			}

		} else if ((header & 0b11100000) == 0b10100000) {
			// time frame, 3 time bytes follows

			if (data_ix + 3 > encoded_data_len) {
				return -EIO;
			}
			data_ix += 3;

		} else if (header == 0b01001000) {
			// control frame, one control byte follows

			if (data_ix + 1 > encoded_data_len) {
				return -EIO;
			}
			data_ix += 1;

		} else {
			// unknown frame header, the encoded data is corrupt
			return -EPROTO;
		}
	}

	return ret;
}

static int acc_reg_write(uint8_t reg, const uint8_t *data, uint16_t len)
{
	// burst writes is a no-no, must write register per register
	int err;
	uint8_t spi_data[2];
	while (len-- > 0) {
		spi_data[0] = reg++;
		spi_data[1] = *data++;
		err = spi_transceive_blocking(SPI_BUS_GENERIC, spi_data, 2, NULL, 0);
		if (err != ERROR_OK)
			break;
	}
	return err;
}

static int acc_reg_read(uint8_t reg, uint8_t *data, uint16_t len)
{
	int err;
	uint8_t cmd = reg | BMA400_SPI_REGISTER_READ_FLAG;
	uint8_t temp_buff[len + 2];
	err = spi_transceive_blocking(SPI_BUS_GENERIC, &cmd, 1, temp_buff, sizeof(temp_buff));
	memcpy(data, temp_buff + 2, len);
	return err;
}

static void acc_irq_handle(uint32_t event, void *arg) {
	// read all three int status registers to clear them
	uint8_t stat_raw_buf[3];
	(void)acc_reg_read(BMA400_REG_INT_STAT0, stat_raw_buf, 3);
	bma400_reg_INT_STAT0_t int_stat0;
	int_stat0.raw = stat_raw_buf[0];

	if (int_stat0.fwm_int_stat != 0) { // fifo watermark interrupt
		// read FIFO data
		uint8_t fifo_len_raw[2];
		bma400_reg_FIFO_PWR_CONFIG_t fifo_pwr_cfg;

		// get number of bytes in fifo
		(void)acc_reg_read(BMA400_REG_FIFO_LENGTH0, fifo_len_raw, 2);
		uint16_t fifo_bytes = ((bma400_reg_FIFO_LENGTH0_t)fifo_len_raw[0]).fifo_bytes_cnt_7_0 |
						(((bma400_reg_FIFO_LENGTH1_t)fifo_len_raw[1]).fifo_bytes_cnt_10_8 << 8);
		if (fifo_bytes > sizeof(me.fifo_buf)) {
			fifo_bytes = sizeof(me.fifo_buf);
		}
		// enable reading fifo
		fifo_pwr_cfg.fifo_read_disable = 0;
		(void)acc_reg_write(BMA400_REG_FIFO_PWR_CONFIG, (uint8_t[1]){fifo_pwr_cfg.raw}, 1);
		// datasheet claims a 50us wait between enabling readout and actual readout
		cpu_halt_us(50);
		// read
		(void)acc_reg_read(BMA400_REG_FIFO_DATA, (uint8_t *)me.fifo_buf, fifo_bytes);
		// disable reading fifo (saves 0.1uA)
		fifo_pwr_cfg.fifo_read_disable = 1;
		(void)acc_reg_write(BMA400_REG_FIFO_PWR_CONFIG, (uint8_t[1]){fifo_pwr_cfg.raw}, 1);

		me.samples = bma400_decode_fifo(me.fifo_buf, fifo_bytes, me.sample_buf);

		if (me.debug && me.samples > 0)
		{
			for (int i = 0; i < me.samples; i += 3)
			{
				printf("ACC:\t%d\t%d\t%d\r\n", me.sample_buf[i], me.sample_buf[i + 1], me.sample_buf[i + 2]);
			}
		}
		else if (me.samples < 0)
		{
			printf("ACC: error decoding samples %d\r\n", me.samples);
		}
	}
}

static void acc_irq(uint16_t pin, uint8_t state)
{
	if (!me.init)
		return;
	me.irqs++;
	eventq_add(0, 0, acc_irq_handle);
}

static void deinit(void)
{
	const target_t *target = target_get();
	if (!target->accelerometer.routed)
		return;

	if (target->accelerometer.bus.bus_type == BUS_TYPE_SPI)
	{
		spi_deinit(SPI_BUS_GENERIC);
	}
	if (target->accelerometer.pin_int != BOARD_PIN_UNDEF)
	{
		gpio_irq_callback(target->accelerometer.pin_int, NULL);
		target_reset_pin(target->accelerometer.pin_int);
	}
	if (target->accelerometer.pin_vdd != BOARD_PIN_UNDEF)
	{
		gpio_set(target->accelerometer.pin_vdd, target->accelerometer.pin_vdd_active_high ? 0 : 1);
		target_reset_pin(target->accelerometer.pin_vdd);
	}
}

int acc_bma400_config(bool current_testing, bool debug)
{
	if (!me.init)
	{
		return -EBADF;
	}

	me.debug = debug;

	uint8_t range = BMA400_ACCCFG1_ACC_RANGE_2G;

	// perform a soft reset
	bma400_reg_CMD_t cmd = {.cmd = BMA400_CMD_SOFT_RESET};
	(void)acc_reg_write(BMA400_REG_CMD, (uint8_t[1]){cmd.raw}, 1);

	// config accelerometer
	// 25Hz ODR, range 2g, start in sleep state
	bma400_reg_ACC_CONFIG0_t acc_cfg0 = {.power_mode_conf = BMA400_ACCCFG0_POWER_MODE_SLEEP,
										 .osr_lp = 0,
										 .filt_bw = BMA400_ACCCFG0_FILT_BW_0_4xODR};
	bma400_reg_ACC_CONFIG1_t acc_cfg1 = {.acc_odr = BMA400_ACCCFG1_ACC_ODR_25HZ, .osr = 0, .acc_range = range};
	if (current_testing)
	{
		acc_cfg1.acc_odr = BMA400_ACCCFG1_ACC_ODR_12HZ5; // help hw measure accelerometer, use 12.5Hz ODR
	}
	bma400_reg_ACC_CONFIG2_t acc_cfg2 = {.data_src_reg = BMA400_ACCCFG2_DATA_SRC_ACC_FILT1_VAR};
	(void)acc_reg_write(BMA400_REG_ACC_CONFIG0, (uint8_t[3]){acc_cfg0.raw, acc_cfg1.raw, acc_cfg2.raw}, 3);

	// config fifo
	bma400_reg_FIFO_CONFIG0_t fifo_cfg0 = {.raw = 0};
	if (!current_testing)
	{
		// enable fifo - select all axes, select source
		fifo_cfg0.fifo_x_en = 1;
		fifo_cfg0.fifo_y_en = 1;
		fifo_cfg0.fifo_z_en = 1;
		fifo_cfg0.fifo_data_src = BMA400_FIFOCFG0_FIFO_DATA_SRC_FILT1_VAR;
	}

	// set watermark level
	uint16_t fwm = 6 * 3 * 2; // six sets of samples, one set is three samples (xyz), one sample is two bytes

	bma400_reg_FIFO_CONFIG1_t fifo_cfg1 = {.fifo_watermark_7_0 = (fwm) & 0xff};
	bma400_reg_FIFO_CONFIG2_t fifo_cfg2 = {.fifo_watermark_8_10 = (fwm) >> 8};
	(void)acc_reg_write(BMA400_REG_FIFO_CONFIG0, (uint8_t[3]){fifo_cfg0.raw, fifo_cfg1.raw, fifo_cfg2.raw}, 3);
	bma400_reg_FIFO_PWR_CONFIG_t fifo_pwr_cfg = {.fifo_read_disable = 1}; // saves 0.1uA
	(void)acc_reg_write(BMA400_REG_FIFO_CONFIG0,
						(uint8_t[4]){fifo_cfg0.raw, fifo_cfg1.raw, fifo_cfg2.raw, fifo_pwr_cfg.raw}, 4);

	// configure external interrupt - interrupts are only available in normal mode
	// map to interrupt 1
	bma400_reg_INT1_MAP_t int1_map = {.raw = 0};
	if (!current_testing)
	{
		int1_map.ffull_int1 = 1;
		int1_map.fwm_int1 = 1;
	}
	bma400_reg_INT2_MAP_t int2_map = {.raw = 0};
	bma400_reg_INT12_MAP_t int12_map = {.raw = 0};
	(void)acc_reg_write(BMA400_REG_INT1_MAP, (uint8_t[3]){int1_map.raw, int2_map.raw, int12_map.raw}, 3);

	// set interrupt 1 pin to active high, push pull
	bma400_reg_INT12_IO_CTRL_t int12_io_ctrl = {.int1_lvl = BMA400_INT12IOCTL_INT1_LVL_ACTIVE_HIGH,
												.int1_od = BMA400_INT12IOCTL_INT1_PUSHPULL,
												.int2_od = BMA400_INT12IOCTL_INT2_OPENDRAIN};
	(void)acc_reg_write(BMA400_REG_INT12_IO_CTRL, (uint8_t[1]){int12_io_ctrl.raw}, 1);

	// enable interrupts, disable latched interrupts
	bma400_reg_INT_CONFIG0_t int_cfg0 = {.gen1_int_en = 1};
	if (!current_testing)
	{
		int_cfg0.ffull_int_en = 1;
		int_cfg0.fwm_int_en = 1;
	}
	bma400_reg_INT_CONFIG1_t int_cfg1 = {.latch_int = 0};

	(void)acc_reg_write(BMA400_REG_INT_CONFIG0, (uint8_t[2]){int_cfg0.raw, int_cfg1.raw}, 2);

	// start accelerometer
	acc_cfg0.power_mode_conf = BMA400_ACCCFG0_POWER_MODE_NORMAL;
	(void)acc_reg_write(BMA400_REG_ACC_CONFIG0, (uint8_t[1]){acc_cfg0.raw}, 1);

	return ERROR_OK;
}

int acc_bma400_init(void) {
    const target_t *target = target_get();

    if (me.init) {
        return -EBADF;
    }

    if (!target->accelerometer.routed || target->accelerometer.type != ACC_TYPE_BMA400) {
        return -ENOTSUP;
    }

    if (target->accelerometer.bus.bus_type != BUS_TYPE_SPI) {
        // only spi support implemented for now
        return -ENOSYS;
    }

    memset(&me, 0, sizeof(me));

    spi_config_t spi_cfg = {
        .pins.clk = target->accelerometer.bus.spi.pin_clk,
        .pins.csn = target->accelerometer.bus.spi.pin_cs,
        .pins.miso = target->accelerometer.bus.spi.pin_miso,
        .pins.mosi = target->accelerometer.bus.spi.pin_mosi,
        .clk_freq = target->accelerometer.bus.bus_speed_hz,
        .mode = target->accelerometer.bus.spi.mode
    };

    int err = spi_init(SPI_BUS_GENERIC, &spi_cfg);
    if (err) {
        deinit();
        return err;
    }

    if (target->accelerometer.pin_vdd != BOARD_PIN_UNDEF) {
        gpio_set(target->accelerometer.pin_vdd, target->accelerometer.pin_vdd_active_high ? 0 : 1);
        gpio_config(target->accelerometer.pin_vdd, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
    }

    if (target->accelerometer.pin_int != BOARD_PIN_UNDEF) {
		gpio_config(target->accelerometer.pin_int, GPIO_DIRECTION_INPUT, GPIO_PULL_DOWN);
		gpio_irq_callback(target->accelerometer.pin_int, acc_irq);
	}

	(void)acc_bma400_power(false);
	cpu_halt(200);
	(void)acc_bma400_power(true);
	cpu_halt(10);

	bma400_reg_CHIPID_t chip_id;
	// extra dummy read for SPI mode
	acc_reg_read(BMA400_REG_CHIPID, &chip_id.raw, 1);
	acc_reg_read(BMA400_REG_CHIPID, &chip_id.raw, 1);
	me.id = chip_id.chipid;

	if (me.id != BMA400_CHIP_ID) {
	    deinit();
	    return -ENXIO;
	}

	me.init = true;

	return ERROR_OK;
}

int acc_bma400_power(bool enable)
{
	const target_t *target = target_get();
	if (!target->accelerometer.routed || target->accelerometer.pin_vdd == BOARD_PIN_UNDEF)
	{
		return -ENOTSUP;
	}
	if (enable) {
		gpio_set(target->accelerometer.pin_vdd, target->accelerometer.pin_vdd_active_high ? 1 : 0);
	} else {
		gpio_set(target->accelerometer.pin_vdd, target->accelerometer.pin_vdd_active_high ? 0 : 1);
	}

	return ERROR_OK;
}

void acc_bma400_deinit(void) {
    if (!me.init) {
        return;
    }

	deinit();

    me.init = false;
}

uint8_t acc_bma400_get_id(void) {
	return me.id;
}

uint32_t acc_bma400_get_irq_count(void) {
	return me.irqs;
}

uint32_t acc_bma400_get_sample_count(void) {
	return me.samples;
}

const int16_t *acc_bma400_get_samples(void) {
	return me.sample_buf;
}

#if NO_EXTRA_CLI == 0

#include "cli.h"

static int cli_bma400_init(int argc, const char **argv) {
    int err = acc_bma400_init();
    if (err == -ENXIO || err == ERROR_OK) printf("ID:0x%02x\r\n", me.id);
    return err;
}
CLI_FUNCTION(cli_bma400_init, "bma400_init", "initialize BMA400 driver");

static int cli_bma400_read(int argc, const char **argv)
{
	if (argc < 1)
		return ERR_CLI_EINVAL;
	uint8_t reg = strtol(argv[0], NULL, 0);
	uint8_t len = argc > 1 ? strtol(argv[1], NULL, 0) : 1;
	uint8_t reg_data[len];
	int err = acc_reg_read(reg, reg_data, sizeof(reg_data));
	printf("reg %02x = ", reg);
	for (uint8_t i = 0; i < len; i++)
	{
		printf("%02x ", reg_data[i]);
	}
	printf("\r\n");
	return err;
}
CLI_FUNCTION(cli_bma400_read, "bma400_read", "read BMA400 reg: <reg> (<length>)");

static int cli_bma400_config(int argc, const char **argv) {
    bool current_testing = false;
    bool debug = false;
    for (int i = 0; i < argc; i++) {
        switch (argv[i][0]) {
		case 'c':
			current_testing = true;
			break;
		case 'd':
			debug = true;
			break;
		}
	}
	int err = acc_bma400_config(current_testing, debug);
	return err;
}
CLI_FUNCTION(cli_bma400_config, "bma400_config", "config BMA400 for testing: (debug|current)*");

static int cli_bma400_power(int argc, const char **argv)
{
	bool enable = strtol(argv[0], NULL, 0);
	return acc_bma400_power(enable);
}
CLI_FUNCTION(cli_bma400_power, "bma400_power", "BMA400 power on/off: 0|1");

static int cli_bma400_deinit(int argc, const char **argv) {
    acc_bma400_deinit();
    return ERROR_OK;
}
CLI_FUNCTION(cli_bma400_deinit, "bma400_deinit", "stop BMA400 driver");

#endif
