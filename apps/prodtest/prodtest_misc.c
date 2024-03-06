#include <math.h>
#include "prodtest.h"
#include "gpio_driver.h"
#include "cli.h"
#include "minio.h"
#include "cpu.h"
#include "batt.h"
#include "accelerometer_bma400.h" // once we have more accelerometers, make this a common api instead
#include "uart_prodtest.h"
#include "rf.h"

typedef enum {
	LFCLK_OSC_RC = 0,
	LFCLK_OSC_XTAL = 1,
	LFCLK_OSC_SYNTH = 2,
} lfclk_oscillator_t;

typedef enum {
	CURM_STATE_NONE,
	CURM_STATE_IDLE,
	CURM_STATE_ACC,
	CURM_STATE_BLE_TX,
	CURM_STATE_BLE_RX,
	CURM_STATE_VIB,
	CURM_STATE_LFCLK,
	CURM_STATE_MOTOR_CW,
	CURM_STATE_MOTOR_CCW,
	CURM_STATE_END,
} curm_state_t;

#define LFCLK_CYCLES  (32767)
#define SYSTICK_VALUE (1 << 20)
#define BLE_GAP_ADDR_LEN 6

#define BATT_NUM_MEAS			   5
#define BATT_SAMPLE_DELAY_MS	   26
#define BATT_MEAS_MAX_STANDARD_DEV 25

static bool hfclk_constant_measure_calibrated = false;
static int hfclk_constant_measure_error = 0;
static volatile curm_state_t curm_state = CURM_STATE_NONE;
static volatile curm_state_t curm_state_prev = CURM_STATE_NONE;


static void curm_state_enter(curm_state_t state) {
	const target_t *target = target_get();
	switch (state) {
		case CURM_STATE_IDLE:
		break;
		case CURM_STATE_ACC:
		acc_bma400_init();
		acc_bma400_config(true, false);
		break;
		case CURM_STATE_BLE_TX:
		// Frequency : 2440 MHz (channel 17)
		// Length    : 37 Bytes
		// Packet    : PRBS9 Packet Payload
		rf_dtm_set_phy(LE_PHY_1M);
		rf_dtm_tx(17, 37, DTM_PKT_PRBS9, RADIO_TXPOWER_TXPOWER_Pos4dBm);
		break;
		case CURM_STATE_BLE_RX:
		// Frequency : 2440 MHz (channel 17)
		rf_dtm_rx(17);
		break;
		case CURM_STATE_VIB:
		if (target->vibrator.routed && target->vibrator.pin != BOARD_PIN_UNDEF) {
			gpio_set(target->vibrator.pin, target->vibrator.pin_active_high ? 1 : 0);
			gpio_config(target->vibrator.pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
		}
		break;
		case CURM_STATE_LFCLK:
		prodtest_output_lfclk(true);
		break;
		case CURM_STATE_MOTOR_CW:
		break;
		case CURM_STATE_MOTOR_CCW:
		break;
		default:
		break;
	}
}

static void curm_state_exit(curm_state_t state) {
	const target_t *target = target_get();
	switch (state) {
		case CURM_STATE_IDLE:
		break;
		case CURM_STATE_ACC:
		acc_bma400_deinit();
		break;
		case CURM_STATE_BLE_TX:
		rf_dtm_end();
		break;
		case CURM_STATE_BLE_RX:
		rf_dtm_end();
		break;
		case CURM_STATE_VIB:
		if (target->vibrator.routed && target->vibrator.pin != BOARD_PIN_UNDEF) {
			gpio_set(target->vibrator.pin, target->vibrator.pin_active_high ? 0 : 1);
			target_reset_pin(target->vibrator.pin);
		}
		break;
		case CURM_STATE_LFCLK:
		prodtest_output_lfclk(false);
		break;
		case CURM_STATE_MOTOR_CW:
		break;
		case CURM_STATE_MOTOR_CCW:
		break;
		default:
		break;
	}
}

static bool test_hclock(void)
{
	bool res;

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;

	volatile uint32_t spoonguard = cpu_core_clock_freq()/4;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED != 1 && --spoonguard);

	res = NRF_CLOCK->HFCLKSTAT == (CLOCK_HFCLKSTAT_SRC_Xtal | CLOCK_HFCLKSTAT_STATE_Running << CLOCK_HFCLKSTAT_STATE_Pos);
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	return res;
}

static bool test_lclock(void)
{
	bool res;

	NRF_CLOCK->TASKS_LFCLKSTOP = 1;
	NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSTAT_SRC_Xtal;
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;

	volatile uint32_t spoonguard = cpu_core_clock_freq()/4;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED != 1 && -spoonguard);

	res = NRF_CLOCK->LFCLKSTAT == (CLOCK_LFCLKSTAT_SRC_Xtal | CLOCK_LFCLKSTAT_STATE_Running << CLOCK_LFCLKSTAT_STATE_Pos);

	NRF_CLOCK->TASKS_LFCLKSTOP = 1;
	return res;
}

static int cli_lfck(int argc, const char **argv)
{
	if (argc != 1 || (argv[0][0] != '0' && argv[0][0] != '1')) {
		return ERR_CLI_EINVAL;
    }
	return prodtest_output_lfclk(argv[0][0] == '1');
}
CLI_FUNCTION(cli_lfck, "LFCK", "LF CLOCK output 16384Hz. LFCK=1; starts the output, LFCK=0; stops the output");

static uint32_t hclock_cycles_on_lclock_cycles(lfclk_oscillator_t osc, uint32_t lfclk_cycles)
{
	__DSB();
	SysTick->LOAD = SYSTICK_VALUE - 1;
	SysTick->VAL = 0UL;

	uint32_t old_lfclk_osc = NRF_CLOCK->LFCLKSRC;
	// wait for selected low-frequency oscillator to start up
	NRF_CLOCK->TASKS_LFCLKSTOP = 1;
	NRF_CLOCK->LFCLKSRC = osc;
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
		;

	// setup and reset rtc
	NRF_RTC0->PRESCALER = 0;
	NRF_RTC0->TASKS_CLEAR = 1;
	while (NRF_RTC0->COUNTER != 0)
		;
	__DSB();

	uint32_t hfclk_cycles = 0;

	// start measuring
	NRF_RTC0->TASKS_START = 1;
	SysTick->CTRL = (1 << 0) | (1 << 2); // enable, use core clock
	while (NRF_RTC0->COUNTER < lfclk_cycles) {
		if (SysTick->CTRL & (1 << 16)) {
			// systick overflow
			hfclk_cycles += SYSTICK_VALUE;
		}
	}
	uint32_t systick_stop_val = SysTick->VAL;
	uint32_t rest = (SYSTICK_VALUE - 1) - systick_stop_val;
	hfclk_cycles += rest;
	SysTick->CTRL = 0;		  // disable SysTick
	NRF_RTC0->TASKS_STOP = 1; // disable rtc0

	// wait for original low-frequency oscillator to start up
	NRF_CLOCK->TASKS_LFCLKSTOP = 1;
	NRF_CLOCK->LFCLKSRC = old_lfclk_osc;
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
		;

	__DSB();

	return hfclk_cycles;
}

static int32_t hclock_deviation(bool verbose)
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
		;

	if (!hfclk_constant_measure_calibrated) {
		if (verbose) {
			printf("Calibrating...\r\n");
		}

		// warm up, tune in, or some other hw thing
		(void)hclock_cycles_on_lclock_cycles(LFCLK_OSC_SYNTH, 100);
		// calculate constant error by using LF clock synthetisized from HF clock
		uint32_t synthetisized_lfclk = hclock_cycles_on_lclock_cycles(LFCLK_OSC_SYNTH, LFCLK_CYCLES);
		hfclk_constant_measure_error = SystemCoreClock - synthetisized_lfclk;
		if (verbose) {
			printf("Calibration done [constant measure error:%d]\r\n", hfclk_constant_measure_error);
		}
		hfclk_constant_measure_calibrated = 1;
	}

	// warm up, tune in, or some other hw thing
	(void)hclock_cycles_on_lclock_cycles(LFCLK_OSC_XTAL, 100);

	uint32_t hfclk = hclock_cycles_on_lclock_cycles(LFCLK_OSC_XTAL, LFCLK_CYCLES);
	const uint32_t hfclk_freq = SystemCoreClock;
	int32_t delta = hfclk_freq - hfclk;
	delta -= hfclk_constant_measure_error;
	int32_t hfclk_mhz = hfclk_freq / 1000000UL;
	int32_t ppm = (delta + (delta > 0 ? hfclk_mhz : -hfclk_mhz) / 2) / hfclk_mhz;
	if (verbose) {
		printf("CYCLES:%d (%+d constant error)  EXPECTED:%d  DELTA:%d\r\nDEVIATION %+dppm\r\n", hfclk,
					hfclk_constant_measure_error, hfclk_freq, delta, ppm);
	}

	NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	return ppm;
}

static int cli_fcte(int argc, const char **argv) {
	int err = ERROR_OK;
    const target_t *target = target_get();

	// check battery

	int16_t vBat_mv[BATT_NUM_MEAS] = { 0 };
	int16_t mean_batt_mv = 0;
	uint32_t standard_dev = 0;

	for (int i = 0; i < BATT_NUM_MEAS; i++) {
		cpu_halt(BATT_SAMPLE_DELAY_MS);
		err = batt_single_measurement(false, &vBat_mv[i]);
		if (err!= ERROR_OK) {
			return err;
		}
		mean_batt_mv += vBat_mv[i];
		printf("vBat_mv[%d] = %d\r\n", i, vBat_mv[i]);
	}
	mean_batt_mv /= BATT_NUM_MEAS;
	for (int i = 0; i < BATT_NUM_MEAS; i++) {
		int16_t delta = vBat_mv[i] - mean_batt_mv;
		standard_dev += delta * delta;
	}

	standard_dev = (uint32_t)sqrtf(standard_dev / BATT_NUM_MEAS);

	if (standard_dev > BATT_MEAS_MAX_STANDARD_DEV)
		return EPRODTEST_BATT_MEASUREMENT_INV;

	// check accelerometer
	if (target->accelerometer.routed) {
		err = acc_bma400_init();
		if (err != ERROR_OK) return err;
		err = acc_bma400_config(false, false);
		if (err != ERROR_OK) return err;

		volatile uint32_t spoonguard = cpu_core_clock_freq()/256;

		while (acc_bma400_get_sample_count() == 0 && --spoonguard) {
			yield_for_events();
		}

		acc_bma400_deinit();
	}

	// check crystals 

	bool lclock = test_lclock();
	bool hclock = test_hclock();

	int32_t ppm = 0;
	if (hclock && lclock) {
		ppm = hclock_deviation(false);
		printf("HFOSC: %d ppm;\r\n", ppm);
	}

	if (target->accelerometer.routed) {
		printf("ACCID: 0x%02X ", acc_bma400_get_id());
		printf("ACCINT: %s, ", acc_bma400_get_irq_count() > 0 ? "OK" : "FAILED");
		if (acc_bma400_get_sample_count() > 0) {
			const int16_t *xyz = acc_bma400_get_samples();
			printf("ACCX: %d, ACCY: %d, ACCZ: %d ", xyz[0], xyz[1], xyz[2]);
		} else {
			printf("ACCX: NONE, ACCY: NONE, ACCZ: NONE ");
		}
	}

	printf("ADC: %dmV ", mean_batt_mv);

	printf("RTC %s OSC %s;\r\n", lclock ? "OK" : "FAILED", hclock ? "OK" : "FAILED");


   return err;
}
CLI_FUNCTION(cli_fcte, "FCTE", "Functional test");

static int cli_xtal(int argc, const char **argv) {
    int ppm = hclock_deviation(argc > 0 && argv[0][0] == '2');
    printf("HFOSC: %d ppm;\r\n", ppm);
    return ERROR_OK;
}
CLI_FUNCTION(cli_xtal, "XTAL", "Measures HF xtal against LF xtal");

static int cli_btmr(int argc, const char **argv) {
	const volatile uint8_t *addr = (const volatile uint8_t *)NRF_FICR->DEVICEADDR;
	char data[5];

	printf("BTMR ");
	for (int i = (BLE_GAP_ADDR_LEN - 1); i >= 0; i--) {
		uint8_t byte = addr[i];

		/* Acc to BLE standard : the two highest, msb bits should be set */
		/* Bluetooth Core v4.0, Vol 3, Part C, chapter 10.8.1 */
		if (i == BLE_GAP_ADDR_LEN - 1)
			byte |= 0xC0;

		sprintf(data, "%02X%c", byte, i != 0 ? ':' : ';');
		printf(data);
	}
	printf("\r\n");
	return ERROR_OK;
}
CLI_FUNCTION(cli_btmr, "BTMR", "Read BLE MAC address");

static int cli_vibc(int argc, const char **argv) {
    const target_t *target = target_get();
	if (!target->vibrator.routed || target->vibrator.pin == BOARD_PIN_UNDEF) return -ENOTSUP;
	if (argc != 1 || (argv[0][0] != '0' && argv[0][0] != '1')) {
		return ERR_CLI_EINVAL;
	}
	if (argv[0][0] == '0') {
		gpio_set(target->vibrator.pin, target->vibrator.pin_active_high ? 0 : 1);
		gpio_config(target->vibrator.pin, GPIO_DIRECTION_INPUT, GPIO_PULL_NONE);
	} else {
		gpio_set(target->vibrator.pin, target->vibrator.pin_active_high ? 1 : 0);
		gpio_config(target->vibrator.pin, GPIO_DIRECTION_OUTPUT, GPIO_PULL_NONE);
	}

	return ERROR_OK;
}
CLI_FUNCTION(cli_vibc, "VIBC", "Vibrator control. VIBC=1; starts the vibrator, VIBC=0; stops the vibrator");

static void curm_irq(uint16_t pin, uint8_t state) {
	cpu_halt(10); // pin settle
	if (gpio_read(pin) == 0 && curm_state == curm_state_prev) {
		curm_state++;
		if (curm_state >= CURM_STATE_END) {
			curm_state = CURM_STATE_NONE;
		}
	}
}

static int cli_curm(int argc, const char **argv) {
	const target_t *target = target_get();
	uint16_t pin = target->buttons[BUTTON_ID_PRODTEST].pin;
	if (pin == BOARD_PIN_UNDEF) {
		return -ENOTSUP;
	}

	curm_state_prev = CURM_STATE_NONE;
	curm_state = CURM_STATE_IDLE;

	printf("CURM Start;\r\n");

	cpu_halt(10); // wait for the last uart byte to be shifted out

	gpio_config(pin, GPIO_DIRECTION_INPUT, GPIO_PULL_DOWN);
	gpio_irq_callback(pin, curm_irq);
	uart_prodtest_deinit();

	while (curm_state != CURM_STATE_NONE) {
		yield_for_events();
		if (curm_state_prev != curm_state) {
			curm_state_exit(curm_state_prev);
			curm_state_enter(curm_state);
			curm_state_prev = curm_state;
		}
		__WFI();
	}

	gpio_irq_callback(pin, NULL);
	target_reset_pin(pin);
	uart_prodtest_init();

	return ERROR_OK;
}
CLI_FUNCTION(cli_curm, "CURM", "Put the device in current Test Mode");
