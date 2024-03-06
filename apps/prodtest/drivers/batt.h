#ifndef _BATT_H_
#define _BATT_H_

#include <stdint.h>
#include <stdbool.h>

int batt_single_measurement(bool load, int16_t *millivolt);

#endif // _BATT_H_
