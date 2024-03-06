#pragma once

#include <stdint.h>

// This header defines structs representing the registers in the BMA400.
// See datasheet:
// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bma400-ds000.pdf

#define BMA400_CHIP_ID (0x90)

#define BMA400_SPI_REGISTER_READ_FLAG (0x80)

#define __BMA400_BITSTRUCT                                                                                             \
	uint8_t raw;                                                                                                       \
	struct __attribute__((packed))

#define BMA400_REG_CHIPID 0x00
typedef union {
	__BMA400_BITSTRUCT { uint8_t chipid : 8; };
} bma400_reg_CHIPID_t;

#define BMA400_REG_ERR_REG 0x02
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t _resv0 : 1;
		uint8_t cmd_err : 1;
		uint8_t _resv1 : 6;
	};
} bma400_reg_ERR_REG_t;

#define BMA400_REG_STATUS 0x03
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t int_active : 1;
		enum {
			BMA400_STATUS_POWER_MODE_SLEEP = 0,
			BMA400_STATUS_POWER_MODE_LOWPOW = 1,
			BMA400_STATUS_POWER_MODE_NORMAL = 2
		} power_mode : 2;
		uint8_t _resv0 : 1;
		uint8_t cmd_rdy : 1;
		uint8_t _resv1 : 2;
		uint8_t drdy : 1;
	};
} bma400_reg_STATUS_t;

#define BMA400_REG_ACC_X_LSB 0x04
typedef union {
	__BMA400_BITSTRUCT { uint8_t acc_x_7_0 : 8; };
} bma400_reg_ACC_X_LSB_t;

#define BMA400_REG_ACC_X_MSB 0x05
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t acc_x_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_ACC_x_MSB_t;

#define BMA400_REG_ACC_Y_LSB 0x06
typedef union {
	__BMA400_BITSTRUCT { uint8_t acc_y_7_0 : 8; };
} bma400_reg_ACC_Y_LSB_t;

#define BMA400_REG_ACC_Y_MSB 0x07
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t acc_y_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_ACC_Y_MSB_t;

#define BMA400_REG_ACC_Z_LSB 0x08
typedef union {
	__BMA400_BITSTRUCT { uint8_t acc_z_7_0 : 8; };
} bma400_reg_ACC_Z_LSB_t;

#define BMA400_REG_ACC_Z_MSB 0x09
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t acc_z_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_ACC_Z_MSB_t;

#define BMA400_REG_SENSOR_TIME0 0x0A
typedef union {
	__BMA400_BITSTRUCT { uint8_t sensor_time_7_0 : 8; };
} bma400_reg_SENSOR_TIME0_t;

#define BMA400_REG_SENSOR_TIME1 0x0B
typedef union {
	__BMA400_BITSTRUCT { uint8_t sensor_time_15_8 : 8; };
} bma400_reg_SENSOR_TIME1_t;

#define BMA400_REG_SENSOR_TIME2 0x0C
typedef union {
	__BMA400_BITSTRUCT { uint8_t sensor_time_23_16 : 8; };
} bma400_reg_SENSOR_TIME2_t;

#define BMA400_REG_EVENT 0x0D
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t por_detected : 1;
		uint8_t _resv0 : 7;
	};
} bma400_reg_EVENT_t;

#define BMA400_REG_INT_STAT0 0x0E
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t wkup_int_stat : 1;
		uint8_t orientch_int_stat : 1;
		uint8_t gen1_int_stat : 1;
		uint8_t gen2_int_stat : 1;
		uint8_t ieng_overrun_int_stat : 1;
		uint8_t ffull_int_stat : 1;
		uint8_t fwm_int_stat : 1;
		uint8_t drdy_int_stat : 1;
	};
} bma400_reg_INT_STAT0_t;

#define BMA400_REG_INT_STAT1 0x0F
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t step_int_stat : 2;
		uint8_t s_tap_int_stat : 1;
		uint8_t d_tap_int_stat : 1;
		uint8_t ieng_overrun_int_stat : 1;
		uint8_t _resv0 : 3;
	};
} bma400_reg_INT_STAT1_t;

#define BMA400_REG_INT_STAT2 0x10
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t actch_x_int_stat : 1;
		uint8_t actch_y_int_stat : 1;
		uint8_t actch_z_int_stat : 1;
		uint8_t _resv0 : 1;
		uint8_t ieng_overrun_int_stat : 1;
		uint8_t _resv1 : 3;
	};
} bma400_reg_INT_STAT2_t;

#define BMA400_REG_TEMP_DATA 0x11
typedef union {
	__BMA400_BITSTRUCT { uint8_t temp_data : 8; };
} bma400_reg_TEMP_DATA_t;

#define BMA400_REG_FIFO_LENGTH0 0x12
typedef union {
	__BMA400_BITSTRUCT { uint8_t fifo_bytes_cnt_7_0 : 8; };
} bma400_reg_FIFO_LENGTH0_t;

#define BMA400_REG_FIFO_LENGTH1 0x13
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t fifo_bytes_cnt_10_8 : 3;
		uint8_t _resv0 : 5;
	};
} bma400_reg_FIFO_LENGTH1_t;

#define BMA400_REG_FIFO_DATA 0x14
typedef union {
	__BMA400_BITSTRUCT { uint8_t fifo_data_field : 8; };
} bma400_reg_FIFO_DATA_t;

#define BMA400_REG_STEP_CNT_0 0x15
typedef union {
	__BMA400_BITSTRUCT { uint8_t step_cnt_7_0 : 8; };
} bma400_reg_STEP_CNT_0_t;

#define BMA400_REG_STEP_CNT_1 0x16
typedef union {
	__BMA400_BITSTRUCT { uint8_t step_cnt_15_8 : 8; };
} bma400_reg_STEP_CNT_1_t;

#define BMA400_REG_STEP_CNT_2 0x17
typedef union {
	__BMA400_BITSTRUCT { uint8_t step_cnt_23_16 : 8; };
} bma400_reg_STEP_CNT_2_t;

#define BMA400_REG_STEP_STAT 0x18
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_STEPSTAT_STILL = 0,
			BMA400_STEPSTAT_WALKING = 1,
			BMA400_STEPSTAT_RUNNING = 2
		} step_stat_field : 2;
		uint8_t _resv0 : 6;
	};
} bma400_reg_STEP_STAT_t;

#define BMA400_REG_ACC_CONFIG0 0x19
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_ACCCFG0_POWER_MODE_SLEEP = 0,
			BMA400_ACCCFG0_POWER_MODE_LOWPOW = 1,
			BMA400_ACCCFG0_POWER_MODE_NORMAL = 2
		} power_mode_conf : 2;
		uint8_t _resv0 : 3;
		uint8_t osr_lp : 2;
		enum { BMA400_ACCCFG0_FILT_BW_0_4xODR = 0, BMA400_ACCCFG0_FILT_BW_0_2xODR = 1 } filt_bw : 1;
	};
} bma400_reg_ACC_CONFIG0_t;

#define BMA400_REG_ACC_CONFIG1 0x1A
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_ACCCFG1_ACC_ODR_12HZ5 = 5,
			BMA400_ACCCFG1_ACC_ODR_25HZ = 6,
			BMA400_ACCCFG1_ACC_ODR_50HZ = 7,
			BMA400_ACCCFG1_ACC_ODR_100HZ = 8,
			BMA400_ACCCFG1_ACC_ODR_200HZ = 9,
			BMA400_ACCCFG1_ACC_ODR_400HZ = 10,
			BMA400_ACCCFG1_ACC_ODR_800HZ = 11
		} acc_odr : 4;
		uint8_t osr : 2;
		enum {
			BMA400_ACCCFG1_ACC_RANGE_2G = 0,
			BMA400_ACCCFG1_ACC_RANGE_4G = 1,
			BMA400_ACCCFG1_ACC_RANGE_8G = 2,
			BMA400_ACCCFG1_ACC_RANGE_16G = 3
		} acc_range : 2;
	};
} bma400_reg_ACC_CONFIG1_t;

#define BMA400_REG_ACC_CONFIG2 0x1B
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t _resv0 : 2;
		enum {
			BMA400_ACCCFG2_DATA_SRC_ACC_FILT1_VAR = 0,
			BMA400_ACCCFG2_DATA_SRC_ACC_FILT2_100HZ = 1,
			BMA400_ACCCFG2_DATA_SRC_ACC_FILTLP_100HZ = 2
		} data_src_reg : 2;
		uint8_t _resv1 : 4;
	};
} bma400_reg_ACC_CONFIG2_t;

#define BMA400_REG_INT_CONFIG0 0x1F
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t _resv0 : 1;
		uint8_t orientch_int_en : 1;
		uint8_t gen1_int_en : 1;
		uint8_t gen2_int_en : 1;
		uint8_t _resv1 : 1;
		uint8_t ffull_int_en : 1;
		uint8_t fwm_int_en : 1;
		uint8_t drdy_int_en : 1;
	};
} bma400_reg_INT_CONFIG0_t;

#define BMA400_REG_INT_CONFIG1 0x20
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t step_int_en : 1;
		uint8_t _resv0 : 1;
		uint8_t s_tap_int_en : 1;
		uint8_t d_tap_int_en : 1;
		uint8_t actch_int_en : 1;
		uint8_t _resv1 : 2;
		uint8_t latch_int : 1;
	};
} bma400_reg_INT_CONFIG1_t;

#define BMA400_REG_INT1_MAP 0x21
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t wkup_int1 : 1;
		uint8_t orientch_int1 : 1;
		uint8_t gen1_int1 : 1;
		uint8_t gen2_int1 : 1;
		uint8_t ieng_overrun_int1 : 1;
		uint8_t ffull_int1 : 1;
		uint8_t fwm_int1 : 1;
		uint8_t drdy_int1 : 1;
	};
} bma400_reg_INT1_MAP_t;

#define BMA400_REG_INT2_MAP 0x22
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t wkup_int2 : 1;
		uint8_t orientch_int2 : 1;
		uint8_t gen1_int2 : 1;
		uint8_t gen2_int2 : 1;
		uint8_t ieng_overrun_int2 : 1;
		uint8_t ffull_int2 : 1;
		uint8_t fwm_int2 : 1;
		uint8_t drdy_int2 : 1;
	};
} bma400_reg_INT2_MAP_t;

#define BMA400_REG_INT12_MAP 0x23
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t step_int1 : 1;
		uint8_t _resv0 : 1;
		uint8_t tap_int1 : 1;
		uint8_t actch_int1 : 1;
		uint8_t step_int2 : 1;
		uint8_t _resv1 : 1;
		uint8_t tap_int2 : 1;
		uint8_t actch_int2 : 1;
	};
} bma400_reg_INT12_MAP_t;

#define BMA400_REG_INT12_IO_CTRL 0x24
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t _resv0 : 1;
		enum { BMA400_INT12IOCTL_INT1_LVL_ACTIVE_HIGH = 1, BMA400_INT12IOCTL_INT1_LVL_ACTIVE_LOW = 0 } int1_lvl : 1;
		enum { BMA400_INT12IOCTL_INT1_OPENDRAIN = 1, BMA400_INT12IOCTL_INT1_PUSHPULL = 0 } int1_od : 1;
		uint8_t _resv1 : 2;
		enum { BMA400_INT12IOCTL_INT2_LVL_ACTIVE_HIGH = 1, BMA400_INT12IOCTL_INT2_LVL_ACTIVE_LOW = 0 } int2_lvl : 1;
		enum { BMA400_INT12IOCTL_INT2_OPENDRAIN = 1, BMA400_INT12IOCTL_INT2_PUSHPULL = 0 } int2_od : 1;
		uint8_t _resv2 : 1;
	};
} bma400_reg_INT12_IO_CTRL_t;

#define BMA400_REG_FIFO_CONFIG0 0x26
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t auto_flush : 1;
		uint8_t fifo_stop_on_full : 1;
		uint8_t fifo_time_en : 1;
		enum {
			BMA400_FIFOCFG0_FIFO_DATA_SRC_FILT1_VAR = 0,
			BMA400_FIFOCFG0_FIFO_DATA_SRC_FILT2_100HZ = 1
		} fifo_data_src : 1;
		enum { BMA400_FIFOCFG0_FIFO_8BIT = 1, BMA400_FIFOCFG0_FIFO_12BIT = 0 } fifo_8bit_en : 1;
		uint8_t fifo_x_en : 1;
		uint8_t fifo_y_en : 1;
		uint8_t fifo_z_en : 1;
	};
} bma400_reg_FIFO_CONFIG0_t;

#define BMA400_REG_FIFO_CONFIG1 0x27
typedef union {
	__BMA400_BITSTRUCT { uint8_t fifo_watermark_7_0 : 8; };
} bma400_reg_FIFO_CONFIG1_t;

#define BMA400_REG_FIFO_CONFIG2 0x28
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t fifo_watermark_8_10 : 3;
		uint8_t _resv0 : 5;
	};
} bma400_reg_FIFO_CONFIG2_t;

#define BMA400_REG_FIFO_PWR_CONFIG 0x29
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t fifo_read_disable : 1;
		uint8_t _resv0 : 7;
	};
} bma400_reg_FIFO_PWR_CONFIG_t;

#define BMA400_REG_AUTOLOWPOW_0 0x2A
typedef union {
	__BMA400_BITSTRUCT { uint8_t auto_lp_timeout_thres_11_4 : 8; };
} bma400_reg_AUTOLOWPOW_0_t;

#define BMA400_REG_AUTOLOWPOW_1 0x2B
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t drdy_lowpow_trig : 1;
		uint8_t gen1_int : 1;
		enum {
			BMA400_AUTOLOWPOW1_AUTO_LP_TIMEOUT_DISABLED = 0,
			BMA400_AUTOLOWPOW1_AUTO_LP_TIMEOUT_ACTIVE = 1,
			BMA400_AUTOLOWPOW1_AUTO_LP_TIMEOUT_ACTIVE_TIMERRESET_ON_GEN2INT = 2
		} auto_lp_timeout : 2;
		uint8_t auto_lp_timeout_thres_3_0 : 4;
	};
} bma400_reg_AUTOLOWPOW_1_t;

#define BMA400_REG_AUTOWAKEUP_0 0x2C
typedef union {
	__BMA400_BITSTRUCT { uint8_t wakeup_timeout_thres_11_4 : 8; };
} bma400_reg_AUTOWAKEUP_0_t;

#define BMA400_REG_AUTOWAKEUP_1 0x2D
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t _resv0 : 1;
		uint8_t wkup_int : 1;
		uint8_t wkup_timeout : 1;
		uint8_t _resv1 : 1;
		uint8_t wakeup_timeout_thres_3_0 : 4;
	};
} bma400_reg_AUTOWAKEUP_1_t;

#define BMA400_REG_WKUP_INT_CONFIG0 0x2F
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_WKUPINTCFG0_WKUP_REFU_MANUAL = 0,
			BMA400_WKUPINTCFG0_WKUP_REFU_ONETIME = 1,
			BMA400_WKUPINTCFG0_WKUP_REFU_EVERYTIME = 2
		} wkup_refu : 2;
		uint8_t num_of_samples : 3;
		uint8_t wkup_x_en : 1;
		uint8_t wkup_y_en : 1;
		uint8_t wkup_z_en : 1;
	};
} bma400_reg_WKUP_INT_CONFIG0_t;

#define BMA400_REG_WKUP_INT_CONFIG1 0x30
typedef union {
	__BMA400_BITSTRUCT { uint8_t int_wkup_thres : 8; };
} bma400_reg_WKUP_INT_CONFIG1_t;

#define BMA400_REG_WKUP_INT_CONFIG2 0x31
typedef union {
	__BMA400_BITSTRUCT { uint8_t int_wkup_refx : 8; };
} bma400_reg_WKUP_INT_CONFIG2_t;

#define BMA400_REG_WKUP_INT_CONFIG3 0x32
typedef union {
	__BMA400_BITSTRUCT { uint8_t int_wkup_refy : 8; };
} bma400_reg_WKUP_INT_CONFIG3_t;

#define BMA400_REG_WKUP_INT_CONFIG4 0x33
typedef union {
	__BMA400_BITSTRUCT { uint8_t int_wkup_refz : 8; };
} bma400_reg_WKUP_INT_CONFIG4_t;

#define BMA400_REG_ORIENTCH_CONFIG0 0x35
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t _resv0 : 2;
		enum {
			BMA400_ORIENTCHCFG0_ORIENT_REFU_MANUAL = 0,
			BMA400_ORIENTCHCFG0_ORIENT_REFU_ONETIME_FILT2 = 1,
			BMA400_ORIENTCHCFG0_ORIENT_REFU_ONETIME_FILTLP = 2
		} orient_refu : 2;
		enum {
			BMA400_ORIENTCHCFG0_ORIENT_DATA_SRC_FILT2 = 0,
			BMA400_ORIENTCHCFG0_ORIENT_DATA_SRC_FILTLP = 1
		} orient_data_src : 1;
		uint8_t orient_x_en : 1;
		uint8_t orient_y_en : 1;
		uint8_t orient_z_en : 1;
	};
} bma400_reg_ORIENTCH_CONFIG0_t;

#define BMA400_REG_ORIENTCH_CONFIG1 0x36
typedef union {
	__BMA400_BITSTRUCT { uint8_t orient_thres : 8; };
} bma400_reg_ORIENTCH_CONFIG1_t;

#define BMA400_REG_ORIENTCH_CONFIG3 0x38
typedef union {
	__BMA400_BITSTRUCT { uint8_t orient_dur : 8; };
} bma400_reg_ORIENTCH_CONFIG3_t;

#define BMA400_REG_ORIENTCH_CONFIG4 0x39
typedef union {
	__BMA400_BITSTRUCT { uint8_t int_orient_refx_7_0 : 8; };
} bma400_reg_ORIENTCH_CONFIG4_t;

#define BMA400_REG_ORIENTCH_CONFIG5 0x3A
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t int_orient_refx_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_ORIENTCH_CONFIG5_t;

#define BMA400_REG_ORIENTCH_CONFIG6 0x3B
typedef union {
	__BMA400_BITSTRUCT { uint8_t int_orient_refy_7_0 : 8; };
} bma400_reg_ORIENTCH_CONFIG6_t;

#define BMA400_REG_ORIENTCH_CONFIG7 0x3C
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t int_orient_refy_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_ORIENTCH_CONFIG7_t;

#define BMA400_REG_ORIENTCH_CONFIG8 0x3D
typedef union {
	__BMA400_BITSTRUCT { uint8_t int_orient_refz_7_0 : 8; };
} bma400_reg_ORIENTCH_CONFIG8_t;

#define BMA400_REG_ORIENTCH_CONFIG9 0x3E
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t int_orient_refz_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_ORIENTCH_CONFIG9_t;

#define BMA400_REG_GEN1INT_CONFIG0 0x3F
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_GEN1INTCFG0_GEN1_ACT_HYST_NOTACTIVE = 0,
			BMA400_GEN1INTCFG0_GEN1_ACT_HYST_24MG = 1,
			BMA400_GEN1INTCFG0_GEN1_ACT_HYST_48MG = 2,
			BMA400_GEN1INTCFG0_GEN1_ACT_HYST_96MG = 3
		} gen1_act_hyst : 2;
		enum {
			BMA400_GEN1INTCFG0_GEN1_ACT_REFU_MANUAL = 0,
			BMA400_GEN1INTCFG0_GEN1_ACT_REFU_ONETIME = 1,
			BMA400_GEN1INTCFG0_GEN1_ACT_REFU_EVERYTIME = 2,
			BMA400_GEN1INTCFG0_GEN1_ACT_REFU_EVERYTIME_LP = 3
		} gen1_act_refu : 2;
		enum {
			BMA400_GEN1INTCFG0_GEN1_DATA_SRC_FILT1 = 0,
			BMA400_GEN1INTCFG0_GEN1_DATA_SRC_FILT2 = 1
		} gen1_data_src : 1;
		uint8_t gen1_act_x_en : 1;
		uint8_t gen1_act_y_en : 1;
		uint8_t gen1_act_z_en : 1;
	};
} bma400_reg_GEN1INT_CONFIG0_t;

#define BMA400_REG_GEN1INT_CONFIG1 0x40
typedef union {
	__BMA400_BITSTRUCT
	{
		enum { BMA400_GEN1INTCFG1_GEN1_COMB_SEL_OR = 0, BMA400_GEN1INTCFG1_GEN1_COMB_SEL_AND = 1 } gen1_comb_sel : 1;
		enum {
			BMA400_GEN1INTCFG1_GEN1_CRITERION_SEL_INACTIVITY = 0,
			BMA400_GEN1INTCFG1_GEN1_CRITERION_SEL_ACTIVITY = 1
		} gen1_criterion_sel : 1;
		uint8_t _resv0 : 6;
	};
} bma400_reg_GEN1INT_CONFIG1_t;

#define BMA400_REG_GEN1INT_CONFIG2 0x41
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen1_int_thres : 8; };
} bma400_reg_GEN1INT_CONFIG2_t;

#define BMA400_REG_GEN1INT_CONFIG3 0x42
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen1_int_dur_15_8 : 8; };
} bma400_reg_GEN1INT_CONFIG3_t;

#define BMA400_REG_GEN1INT_CONFIG31 0x43
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen1_int_dur_7_0 : 8; };
} bma400_reg_GEN1INT_CONFIG31_t;

#define BMA400_REG_GEN1INT_CONFIG4 0x44
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen1_int_th_refx_7_0 : 8; };
} bma400_reg_GEN1INT_CONFIG4_t;

#define BMA400_REG_GEN1INT_CONFIG5 0x45
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t gen1_int_th_refx_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_GEN1INT_CONFIG5_t;

#define BMA400_REG_GEN1INT_CONFIG6 0x46
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen1_int_th_refy_7_0 : 8; };
} bma400_reg_GEN1INT_CONFIG6_t;

#define BMA400_REG_GEN1INT_CONFIG7 0x47
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t gen1_int_th_refy_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_GEN1INT_CONFIG7_t;

#define BMA400_REG_GEN1INT_CONFIG8 0x48
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen1_int_th_refz_7_0 : 8; };
} bma400_reg_GEN1INT_CONFIG8_t;

#define BMA400_REG_GEN1INT_CONFIG9 0x49
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t gen1_int_th_refz_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_GEN1INT_CONFIG9_t;

#define BMA400_REG_GEN2INT_CONFIG0 0x4A
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_GEN2INTCFG0_GEN2_ACT_HYST_NOTACTIVE = 0,
			BMA400_GEN2INTCFG0_GEN2_ACT_HYST_24MG = 1,
			BMA400_GEN2INTCFG0_GEN2_ACT_HYST_48MG = 2,
			BMA400_GEN2INTCFG0_GEN2_ACT_HYST_96MG = 3
		} gen2_act_hyst : 2;
		enum {
			BMA400_GEN2INTCFG0_GEN2_ACT_REFU_MANUAL = 0,
			BMA400_GEN2INTCFG0_GEN2_ACT_REFU_ONETIME = 1,
			BMA400_GEN2INTCFG0_GEN2_ACT_REFU_EVERYTIME = 2,
			BMA400_GEN2INTCFG0_GEN2_ACT_REFU_EVERYTIME_LP = 3
		} gen2_act_refu : 2;
		enum {
			BMA400_GEN2INTCFG0_GEN2_DATA_SRC_FILT1 = 0,
			BMA400_GEN2INTCFG0_GEN2_DATA_SRC_FILT2 = 1
		} gen2_data_src : 1;
		uint8_t gen2_act_x_en : 1;
		uint8_t gen2_act_y_en : 1;
		uint8_t gen2_act_z_en : 1;
	};
} bma400_reg_GEN2INT_CONFIG0_t;

#define BMA400_REG_GEN2INT_CONFIG1 0x4B
typedef union {
	__BMA400_BITSTRUCT
	{
		enum { BMA400_GEN2INTCFG1_GEN2_COMB_SEL_OR = 0, BMA400_GEN2INTCFG1_GEN2_COMB_SEL_AND = 1 } gen2_comb_sel : 1;
		enum {
			BMA400_GEN2INTCFG1_GEN2_CRITERION_SEL_INACTIVITY = 0,
			BMA400_GEN2INTCFG1_GEN2_CRITERION_SEL_ACTIVITY = 1
		} gen2_criterion_sel : 1;
		uint8_t _resv0 : 6;
	};
} bma400_reg_GEN2INT_CONFIG1_t;

#define BMA400_REG_GEN2INT_CONFIG2 0x4C
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen2_int_thres : 8; };
} bma400_reg_GEN2INT_CONFIG2_t;

#define BMA400_REG_GEN2INT_CONFIG3 0x4D
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen2_int_dur_15_8 : 8; };
} bma400_reg_GEN2INT_CONFIG3_t;

#define BMA400_REG_GEN2INT_CONFIG31 0x4E
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen2_int_dur_7_0 : 8; };
} bma400_reg_GEN2INT_CONFIG31_t;

#define BMA400_REG_GEN2INT_CONFIG4 0x4F
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen2_int_th_refx_7_0 : 8; };
} bma400_reg_GEN2INT_CONFIG4_t;

#define BMA400_REG_GEN2INT_CONFIG5 0x50
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t gen2_int_th_refx_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_GEN2INT_CONFIG5_t;

#define BMA400_REG_GEN2INT_CONFIG6 0x51
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen2_int_th_refy_7_0 : 8; };
} bma400_reg_GEN2INT_CONFIG6_t;

#define BMA400_REG_GEN2INT_CONFIG7 0x52
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t gen2_int_th_refy_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_GEN2INT_CONFIG7_t;

#define BMA400_REG_GEN2INT_CONFIG8 0x53
typedef union {
	__BMA400_BITSTRUCT { uint8_t gen2_int_th_refz_7_0 : 8; };
} bma400_reg_GEN2INT_CONFIG8_t;

#define BMA400_REG_GEN2INT_CONFIG9 0x54
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t gen2_int_th_refz_11_8 : 4;
		uint8_t _resv0 : 4;
	};
} bma400_reg_GEN2INT_CONFIG9_t;

#define BMA400_REG_ACTCH_CONFIG0 0x55
typedef union {
	__BMA400_BITSTRUCT { uint8_t actch_thres : 8; };
} bma400_reg_ACTCH_CONFIG0_t;

#define BMA400_REG_ACTCH_CONFIG1 0x56
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_ACTCHCFG1_ATCCH_NPTS_32 = 0,
			BMA400_ACTCHCFG1_ATCCH_NPTS_64 = 1,
			BMA400_ACTCHCFG1_ATCCH_NPTS_128 = 2,
			BMA400_ACTCHCFG1_ATCCH_NPTS_256 = 3,
			BMA400_ACTCHCFG1_ATCCH_NPTS_512 = 4
		} actch_npts : 4;
		enum { BMA400_ACTCHCFG1_DATA_SRC_ACC_FILT1 = 0, BMA400_ACTCHCFG1_DATA_SRC_ACC_FILT2 = 1 } actch_data_src : 1;
		uint8_t actch_x_en : 1;
		uint8_t actch_y_en : 1;
		uint8_t actch_z_en : 1;
	};
} bma400_reg_ACTCH_CONFIG1_t;

#define BMA400_REG_TAP_CONFIG 0x57
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t tap_sensitivity : 3;
		enum { BMA400_TAPCFG_SEL_AXIS_Z = 0, BMA400_TAPCFG_SEL_AXIS_Y = 1, BMA400_TAPCFG_SEL_AXIS_X = 2 } sel_axis : 2;
		uint8_t _resv0 : 3;
	};
} bma400_reg_TAP_CONFIG_t;

#define BMA400_REG_TAP_CONFIG1 0x58
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_TAPCFG1_TICS_TH_SPLS6 = 0,
			BMA400_TAPCFG1_TICS_TH_SPLS9 = 1,
			BMA400_TAPCFG1_TICS_TH_SPLS12 = 2,
			BMA400_TAPCFG1_TICS_TH_SPLS18 = 3
		} tics_th : 2;
		enum {
			BMA400_TAPCFG1_QUIET_SPLS60 = 0,
			BMA400_TAPCFG1_QUIET_SPLS80 = 1,
			BMA400_TAPCFG1_QUIET_SPLS100 = 2,
			BMA400_TAPCFG1_QUIET_SPLS120 = 3
		} quiet : 2;
		enum {
			BMA400_TAPCFG1_QUIET_DT_SPLS4 = 0,
			BMA400_TAPCFG1_QUIET_DT_SLPS8 = 1,
			BMA400_TAPCFG1_QUIET_DT_SPLS12 = 2,
			BMA400_TAPCFG1_QUIET_DT_SPLS16 = 3
		} quiet_dt : 2;
		uint8_t _resv0 : 2;
	};
} bma400_reg_TAP_CONFIG1_t;

#define BMA400_REG_IF_CONF 0x7C
typedef union {
	__BMA400_BITSTRUCT
	{
		enum { BMA400_IF_SPI4 = 0, BMA400_IF_SPI3 = 1 } spi3 : 1;
		uint8_t _resv0 : 7;
	};
} bma400_reg_IF_CONF_t;

#define BMA400_REG_SELF_TEST 0x7D
typedef union {
	__BMA400_BITSTRUCT
	{
		uint8_t acc_self_test_en_x : 1;
		uint8_t acc_self_test_en_y : 1;
		uint8_t acc_self_test_en_z : 1;
		uint8_t acc_self_test_sign : 1;
		uint8_t _resv0 : 4;
	};
} bma400_reg_SELF_TEST_t;

#define BMA400_REG_CMD 0x7E
typedef union {
	__BMA400_BITSTRUCT
	{
		enum {
			BMA400_CMD_NOP = 0x00,
			BMA400_CMD_FIFO_FLUSH = 0xb0,
			BMA400_CMD_STEP_CNT_CLEAR = 0xb1,
			BMA400_CMD_SOFT_RESET = 0xb6
		} cmd : 8;
	};
} bma400_reg_CMD_t;
