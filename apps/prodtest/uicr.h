#ifndef _UICR_H_
#define _UICR_H_

#include "targets.h"

#define UICR_STRLEN_BOARD_ID 16
#define UICR_STRLEN_ITEM_ID	 11
#define UICR_STRLEN_SERIAL	 11

#define UICR_MARKER_PRODTEST_SIZE	  4
#define UICR_MARKER_CORETEST_SIZE	  1
#define UICR_MARKER_MOVEMNETTEST_SIZE 1

typedef struct __attribute__(( packed )) {
	target_hw_identifier_t id;
	uint8_t serial[5]; /* yywwb0b1b2 */
	uint8_t unused[1];
	uint8_t itemid[4];
	uint32_t prodtest;
	uint32_t coretest;
	uint32_t movementtest;
	uint32_t dummy;
	uint32_t x_axis_calibration;
	uint32_t y_axis_calibration;
	uint32_t z_axis_calibration;
} __attribute__(( packed )) uicr_pluto_t;

typedef struct __attribute__(( packed )) {
	target_hw_identifier_t id;
	uint8_t serial[5];	 /* yywwb0b1b2 */
	uint8_t unused_1[1]; //For alignment and can't be used
	uint32_t itemid;
	union {
		struct {
			uint32_t prodtest_legacy;
			uint32_t coretest_legacy;
			uint32_t movementtest_legacy;
			uint32_t dummy_1;
			uint32_t dummy_2;
			uint16_t dummy_3;
		};
		struct {
			uint16_t prodtest[UICR_MARKER_PRODTEST_SIZE];
			uint16_t movementtest;
			struct {
				uint16_t flag;
				uint16_t active;
			} coretest[3];
		};
	};

	uint16_t pba_motor_rev;
	uint16_t pba_hr_rev;
	uint16_t pba_display_rev;

	int8_t chg_freq_correction;
	uint8_t unused_2[3]; //Align next element on 32 bits

	uint16_t fg_empty_voltage_mV;	 //empty voltage in mV for fuel gauge cfg
	uint16_t fg_design_capacity_mAh; //designCap in mAh for fuel gauge cfg
	uint16_t fg_i_chg_term_uA;		 //i_chg_term in uA for fuel gauge cfg
	uint8_t unused_3[2];			 //Align next element on 32 bits

	uint32_t coretest_not_allow_to_fail;
	uint32_t after_sale; //flag indicate the movement has been created in after sale
}  __attribute__(( packed )) uicr_pascal_t;

volatile const uicr_pascal_t *uicr_get_pascal(void);
volatile const uicr_pluto_t *uicr_get_pluto(void);
void uicr_get_hw_identifier(target_hw_identifier_t *id);
void uicr_get_serial_str(char *buf, size_t buf_size);
void uicr_get_item_id_str(char *buf, size_t buf_size);
bool uicr_pba_motor_rev_is_set(volatile const uicr_pascal_t *u);
bool uicr_pba_display_rev_is_set(volatile const uicr_pascal_t *u);
bool uicr_pba_hr_rev_is_set(volatile const uicr_pascal_t *u);
int uicr_get_active_entry_bit_ix(uint32_t word, uint8_t bitsize);
int uicr_write_word(uint32_t addr, uint32_t word);
uint32_t uicr_get_prodtest_marker(void);
int uicr_set_prodtest_marker(int ix);
int uicr_clear_prodtest_marker(void);

#endif //_UICR_H_