#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvmtnvj.h"
#include "testrunner.h"
#include "testbench.h"

#define DBG(f, ...) printf("%s\t" f, __func__, ##__VA_ARGS__)

typedef struct
{
	enum
	{
		DELETED,
		WRITTEN
	} state;
	uint8_t *data;
	uint8_t len;
} test_tag_t;

test_tag_t tags[65536];

void prand_seed(prand_t *prand, uint32_t seed)
{
	if ((prand->lfsr_a = seed ^ 0x20070515) == 0)
		prand->lfsr_a = 1;
	if ((prand->lfsr_b = seed ^ 0x20090129) == 0)
		prand->lfsr_b = 1;
	if ((prand->lfsr_c = seed ^ 0x20140318) == 0)
		prand->lfsr_c = 1;
}

uint32_t prand(prand_t *prand, uint8_t bits)
{
	// https://www.schneier.com/academic/archives/1994/09/pseudo-random_sequen.html
	uint32_t lfsr_a = prand->lfsr_a;
	uint32_t lfsr_b = prand->lfsr_b;
	uint32_t lfsr_c = prand->lfsr_c;
	uint32_t r = 0;
	while (bits--)
	{
		lfsr_a =
			(((((lfsr_a >> 31) ^ (lfsr_a >> 6) ^ (lfsr_a >> 4) ^ (lfsr_a >> 2) ^ (lfsr_a << 1) ^ lfsr_a) & 1) << 31) |
			 lfsr_a >> 1);
		lfsr_b = ((((lfsr_b >> 30) ^ (lfsr_b >> 2)) & 1) << 30) | (lfsr_b >> 1);
		lfsr_c = ((((lfsr_c >> 28) ^ (lfsr_c >> 1)) & 1) << 28) | (lfsr_c >> 1);
		r = (r << 1) | ((uint32_t)(lfsr_a ^ lfsr_b ^ lfsr_c) & 1);
	}
	prand->lfsr_a = lfsr_a;
	prand->lfsr_b = lfsr_b;
	prand->lfsr_c = lfsr_c;
	return r;
}

void printf_mem(uint8_t *addr, uint32_t offset, uint32_t sz)
{
#define COLS 16
	while (sz > 0)
	{
		uint8_t col = 0;
		printf("%08x: ", offset);
		while (col < COLS && sz > 0)
		{
			printf("%02x%c", addr[offset], (col & 0x3) == 3 ? '.' : ' ');
			col++;
			sz--;
			offset++;
		}
		printf("\n");
	}
}

void printf_mem_flash(void)
{
	const flash_emul_t *f = flash_emul_get();
	for (uint32_t s = 0; s < f->sectors; s++)
	{
		printf("SECTOR %d\n", s + f->sector_offset);
		printf_mem(f->mem, s * f->sector_size, f->sector_size);
	}
}

void test_tags_clear(void)
{
	for (uint32_t i = 0; i < 0x10000; i++)
	{
		if (tags[i].data)
			free(tags[i].data);
	}
	memset(tags, 0, sizeof(tags));
}

void test_tag_store(uint16_t id, const uint8_t *data, uint8_t len)
{
	if (tags[id].data)
		free(tags[id].data);
	tags[id].state = WRITTEN;
	tags[id].data = malloc(len);
	tags[id].len = len;
	memcpy(tags[id].data, data, len);
}

void test_tag_delete(uint16_t id)
{
	if (tags[id].data)
		free(tags[id].data);
	tags[id].state = DELETED;
	tags[id].data = NULL;
}

int test_tag_get(uint16_t id, const uint8_t **data, uint8_t *len)
{
	if (tags[id].state != WRITTEN)
		return -1;
	if (data)
		*data = tags[id].data;
	if (len)
		*len = tags[id].len;

	return 0;
}

int test_tag_compare_all(void)
{
	for (uint32_t id = 0; id < 0x10000; id++)
	{
		int res = test_tag_compare(id);
		if (res < 0)
			return res;
	}
	return 0;
}

int test_tag_compare(uint16_t id)
{
	const uint8_t *test_data;
	uint8_t test_len;
	int test_res = test_tag_get(id, &test_data, &test_len);
	uint8_t ref_data[255];
	uint8_t ref_len;
	int ref_res = nvmtnvj_read(id, ref_data);
	if (ref_res == ERR_NVMTNVJ_NOENT && test_res == -1)
		return 0;
	if (ref_res == ERR_NVMTNVJ_NOENT && test_res != -1)
	{
		DBG("id %04x: ref nonexisting, test existing\n", id);
		return -1;
	}
	if (ref_res >= 0 && test_res == -1)
	{
		DBG("id %04x: ref existing, test nonexisting\n", id);
		return -1;
	}
	if (ref_res < 0)
	{
		DBG("id %04x: nvmtnvj error %d\n", id, ref_res);
		return ref_res;
	}
	if (ref_res != test_len)
	{
		DBG("id %04x: ref length %d, test length %d\n", id, ref_res, test_len);
		return -1;
	}
	for (int i = 0; i < test_len; i++)
	{
		if (ref_data[i] != test_data[i])
		{
			DBG("id %04x: ref/test data differ on index %d, %02x != %02x\n", id, i, ref_data[i], test_data[i]);
			return -1;
		}
	}
	return 0;
}

int store_random_tag_in_nvm_and_testtag(uint16_t id, uint8_t max_size, prand_t *p)
{
	uint8_t size = prand(p, 8) % max_size;
	uint8_t data[size];
	for (uint8_t i = 0; i < size; i++)
		data[i] = prand(p, 8);
	test_tag_store(id, data, size);
	return nvmtnvj_write(id, data, size);
}

int main(int argc, char **argv)
{
	test_init(NULL);
	run_tests(argc, argv);
	return 0;
}

void add_suites(void)
{
	ADD_SUITE(nvmtnvj);
}
