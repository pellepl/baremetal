#ifndef _ACCELEROMETER_BMA400_H_
#define _ACCELEROMETER_BMA400_H_

#include <stddef.h>

int acc_bma400_init(void);
int acc_bma400_config(bool current_testing, bool debug);
void acc_bma400_deinit(void);
int acc_bma400_power(bool enable);
uint8_t acc_bma400_get_id(void);

uint32_t acc_bma400_get_sample_count(void);
const int16_t *acc_bma400_get_samples(void);
uint32_t acc_bma400_get_irq_count(void);

#endif //_ACCELEROMETER_BMA400_H_