#ifndef _TESTBENCH_H_
#define _TESTBENCH_H_

#include "flash_emul.h"
typedef struct
{
    uint32_t lfsr_a, lfsr_b, lfsr_c;
} prand_t;

void prand_seed(prand_t *prand, uint32_t seed);
uint32_t prand(prand_t *prand, uint8_t bits);

void printf_mem(uint8_t *addr, uint32_t offset, uint32_t sz);
void printf_mem_flash(void);

void test_tags_clear(void);
void test_tag_store(uint16_t id, const uint8_t *data, uint8_t len);
void test_tag_delete(uint16_t id);
int test_tag_get(uint16_t id, const uint8_t **data, uint8_t *len);
int test_tag_compare_all(void);
int test_tag_compare(uint16_t id);

int store_random_tag_in_nvm_and_testtag(uint16_t id, uint8_t max_size, prand_t *p);

#endif // _TESTBENCH_H_
