#ifndef _THERMO_H_
#define _THERMO_H_

#include "timer.h"

typedef struct
{
    int16_t temperature;
    int16_t temperature_junction;
    char is_new;
    uint8_t error;
} thermo_result_t;

void thermoc_init(void);
tick_t thermoc_ticks_since_sample(void);
void thermoc_temp(thermo_result_t *res);
void thermoc_set_multiplier(int m);
void thermoc_set_offset(int o);

#endif