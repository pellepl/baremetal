#pragma once
#include <stdbool.h>

int ppg_afe4410_init(void);
void ppg_afe4410_deinit(void);
int ppg_afe4410_power(bool enable);
int ppg_afe4410_reset(bool enable);

