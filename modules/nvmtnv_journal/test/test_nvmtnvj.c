#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "testrunner.h"
#include "testbench.h"
#include "nvmtnvj.h"
#include "flash_driver.h"

#define PAGE_SIZE 128
#define PAGE_COUNT 8
#define BLOCK_PAGES 2
#define TAG_MAX_SIZE 8
static uint8_t memory[PAGE_SIZE * PAGE_COUNT];

SUITE(basic)

static void setup(test_t *t)
{
	memset(memory, 0x00, sizeof(memory));
	flash_emul_t f = {
		.flags = FLASH_EMUL_DISALLOW_BIT_PULLING | FLASH_EMUL_WRITE_BY_OR,
		.flash_address = memory,
		.mem = memory,
		.mem_size = sizeof(memory),
		.read_alignment = 0,
		.sector_offset = 0,
		.sector_size = PAGE_SIZE,
		.sectors = PAGE_COUNT,
		.write_alignment = 0};
	flash_emul_set(&f);
	flash_init();
	nvmtnvj_init();
	test_tags_clear();
}

static void teardown(test_t *t)
{
	flash_deinit();
}

TEST(format)
{
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), 0);
	return 0;
}
TEST_END;

TEST(mount)
{
	memset(memory, 0, sizeof(memory));
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), ERR_NVMTNVJ_NOFS);
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	return 0;
}
TEST_END;

TEST(write_read_delete)
{
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), 0);

	TEST_CHECK_EQ(nvmtnvj_write(0x1111, (const uint8_t *)"hello", 5), 0);
	TEST_CHECK_EQ(nvmtnvj_write(0x2222, (const uint8_t *)"wantoo", 6), 0);
	TEST_CHECK_EQ(nvmtnvj_write(0x3333, (const uint8_t *)"birzooo", 7), 0);
	TEST_CHECK_EQ(nvmtnvj_write(0x0000, (const uint8_t *)"", 0), 0);
	uint8_t data[TAG_MAX_SIZE + 4];
	TEST_CHECK_EQ(nvmtnvj_read(0x2222, data), 6);
	TEST_CHECK_EQ(memcmp(data, "wantoo", 6), 0);
	TEST_CHECK_EQ(nvmtnvj_read(0x0000, data), 0);
	TEST_CHECK_EQ(nvmtnvj_read(0x4444, data), ERR_NVMTNVJ_NOENT);
	TEST_CHECK_EQ(nvmtnvj_delete(0x0000), 0);
	TEST_CHECK_EQ(nvmtnvj_read(0x0000, data), ERR_NVMTNVJ_NOENT);
	TEST_CHECK_EQ(nvmtnvj_delete(0x0000), 0);
	TEST_CHECK_EQ(nvmtnvj_delete(0x5555), 0);
	TEST_CHECK_EQ(nvmtnvj_write(0x0000, (const uint8_t *)"polloooo", 8), 0);
	TEST_CHECK_EQ(nvmtnvj_read(0x0000, data), 8);
	TEST_CHECK_EQ(memcmp(data, "polloooo", 8), 0);
	TEST_CHECK_EQ(nvmtnvj_write(0x0000, (const uint8_t *)"poLLoooo1234", TAG_MAX_SIZE), 0);
	memset(data, 0x0, sizeof(data));
	TEST_CHECK_EQ(nvmtnvj_read(0x0000, data), TAG_MAX_SIZE);
	TEST_CHECK_EQ(memcmp(data, "poLLoooo\0", TAG_MAX_SIZE + 1), 0);
	return 0;
}
TEST_END;

TEST(aborted_write)
{
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), 0);
	uint8_t data[8];

	flash_emul_reset_bytes_written_count();
	TEST_CHECK_EQ(nvmtnvj_write(0x1111, (const uint8_t *)"12345678", 8), 0);
	uint32_t wr = flash_emul_get_bytes_written_count();
	TEST_CHECK_EQ(nvmtnvj_read(0x1111, data), 8);
	TEST_CHECK_EQ(memcmp(data, "12345678", 8), 0);

	flash_emul_write_fail_after_bytes(wr - 4);
	TEST_CHECK_EQ(nvmtnvj_write(0x1111, (const uint8_t *)"xyzqweab", 8), FLASH_EMUL_FORCE_FAIL);
	TEST_CHECK_EQ(nvmtnvj_unmount(), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, 8), 0);
	TEST_CHECK_EQ(nvmtnvj_read(0x1111, data), 8);
	TEST_CHECK_EQ(memcmp(data, "12345678", 8), 0);
	TEST_CHECK_EQ(nvmtnvj_write(0x1111, (const uint8_t *)"xyzqweab", 8), 0);
	TEST_CHECK_EQ(nvmtnvj_read(0x1111, data), 8);
	TEST_CHECK_EQ(memcmp(data, "xyzqweab", 8), 0);

	return 0;
}
TEST_END;

TEST(fill)
{
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), 0);
	int res;
	prand_t p;
	prand_seed(&p, 12345678);
	uint16_t tag = 0;
	do
	{
		res = store_random_tag_in_nvm_and_testtag(tag, TAG_MAX_SIZE, &p);
		tag++;
		TEST_CHECK_NEQ(tag, 0); // spun around all id tag space
	} while (res >= 0);
	TEST_CHECK_EQ(res, ERR_NVMTNVJ_FULL);
	test_tag_delete(--tag); // delete last tag that filled the system to pass next test

	TEST_CHECK_EQ(test_tag_compare_all(), 0);
	TEST_CHECK_EQ(nvmtnvj_unmount(), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), 0);
	TEST_CHECK_EQ(test_tag_compare_all(), 0);
	TEST_CHECK_EQ(nvmtnvj_gc(), ERR_NVMTNVJ_FULL);
	return 0;
}
TEST_END;

static int fill_up_until_gc(prand_t *p)
{
	int res;
	const uint32_t tags_per_block = nvmtnvj_test_tags_per_block();
	const uint32_t blocks = PAGE_COUNT / BLOCK_PAGES;
	// assume our tag id space spans half of available data blocks to allow for 50% chance of rewrite
	const uint16_t max_tag_id = tags_per_block * (blocks - 1) / 2;
	const uint16_t tags_before_gc = tags_per_block * (blocks - 1);
	for (uint32_t c = 0; c < tags_before_gc; c++)
	{
		uint16_t id = prand(p, 16) % max_tag_id;
		res = store_random_tag_in_nvm_and_testtag(id, TAG_MAX_SIZE, p);
		if (res != 0)
			return res;
	}
	return 0;
}

static int prime_gc_state(void)
{
	prand_t p;
	flash_emul_write_fail_after_bytes(0);
	int res = nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE);
	if (res < 0)
		return res;
	res = nvmtnvj_mount(0, BLOCK_PAGES + 1);
	if (res < 0)
		return res;
	prand_seed(&p, 123111);
	res = fill_up_until_gc(&p);
	flash_emul_reset_bytes_written_count();
	return res;
}

TEST(aborted_gc)
{
	TEST_CHECK_EQ(prime_gc_state(), 0);
	TEST_CHECK_EQ(nvmtnvj_write(0x1111, (const uint8_t *)"12345678", 8), 0);
	uint32_t wr = flash_emul_get_bytes_written_count();
	TEST_CHECK_EQ(nvmtnvj_delete(0x1111), 0);
	TEST_CHECK_EQ(test_tag_compare_all(), 0);

#define PARTS 6
	for (int part = 1; part < PARTS; part++)
	{
		TEST_CHECK_EQ(prime_gc_state(), 0);
		flash_emul_write_fail_after_bytes(part * wr / PARTS);
		TEST_CHECK_EQ(nvmtnvj_write(0x1111, (const uint8_t *)"12345678", 8), FLASH_EMUL_FORCE_FAIL);
		TEST_CHECK_EQ(nvmtnvj_unmount(), 0);
		int res = nvmtnvj_mount(0, BLOCK_PAGES + 1);
		if (res == ERR_NVMTNVJ_FS_ABORTED)
		{
			uint8_t data[TAG_MAX_SIZE];
			TEST_CHECK_EQ(nvmtnvj_read(0x1111, data), ERR_NVMTNVJ_FS_ABORTED);
			TEST_CHECK_EQ(nvmtnvj_write(0x1111, data, TAG_MAX_SIZE), ERR_NVMTNVJ_FS_ABORTED);
			TEST_CHECK_EQ(nvmtnvj_delete(0x1111), ERR_NVMTNVJ_FS_ABORTED);
			TEST_CHECK_EQ(nvmtnvj_size(0x1111), ERR_NVMTNVJ_FS_ABORTED);
			TEST_CHECK_EQ(nvmtnvj_gc(), ERR_NVMTNVJ_FS_ABORTED);
			TEST_CHECK_EQ(nvmtnvj_fix(), 0);
		} else {
			TEST_CHECK_EQ(res, 0);
		}
		nvmtnvj_delete(0x1111);
		TEST_CHECK_EQ(test_tag_compare_all(), 0);
	}
	return 0;
}
TEST_END;

TEST(mount_scan_first_sector_borked)
{
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), 0);

	const uint32_t tags_per_block = nvmtnvj_test_tags_per_block();
	const uint32_t blocks = PAGE_COUNT / BLOCK_PAGES;
	prand_t p;
	prand_seed(&p, 112131);

	// fill up first block with something that the GC will just love to clean away
	for (uint32_t i = 0; i < tags_per_block; i++)
	{
		test_tag_store(1, (uint8_t *)&i, 4);
		TEST_CHECK_EQ(nvmtnvj_write(1, (uint8_t *)&i, 4), 0);
	}
	// fill up other blocks with stuff that GC won't touch
	uint16_t unique_tag_id = 100;
	for (uint32_t b = 1; b < blocks - 1; b++)
	{
		for (uint32_t i = 0; i < tags_per_block; i++)
		{
			TEST_CHECK_EQ(store_random_tag_in_nvm_and_testtag(unique_tag_id++, TAG_MAX_SIZE, &p), 0);
		}
	}

	// gc
	TEST_CHECK_EQ(nvmtnvj_gc(), 0);
	// check all data is consistent
	TEST_CHECK_EQ(test_tag_compare_all(), 0);
	// destroy header of first block, which should now be spare
	memset(memory, 0, PAGE_SIZE);
	// check all data is still consistent
	TEST_CHECK_EQ(test_tag_compare_all(), 0);
	// make sure fs is found, but flagged aborted
	TEST_CHECK_EQ(nvmtnvj_unmount(), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, BLOCK_PAGES + 1), ERR_NVMTNVJ_FS_ABORTED);
	// fix and check all is dandy
	TEST_CHECK_EQ(nvmtnvj_fix(), 0);
	TEST_CHECK_EQ(test_tag_compare_all(), 0);

	nvmtnvj_test_dump();

	return 0;
}
TEST_END;

#define WEAR_CYCLES 10000

TEST(wear_balanced)
{
	uint32_t cycles = WEAR_CYCLES;
	prand_t p;
	prand_seed(&p, 111231);
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, 8), 0);
	// assume a tag takes max_size * 2 bytes on flash
	const uint32_t tags_per_block = nvmtnvj_test_tags_per_block();
	const uint32_t blocks = PAGE_COUNT / BLOCK_PAGES;
	// assume our tag id space spans half of available data blocks to allow for 50% chance of rewrite
	const uint16_t max_tag_id = tags_per_block * (blocks - 1) / 2;
	while (cycles--)
	{
		uint16_t id = prand(&p, 16) % max_tag_id;
		if (prand(&p, 2) == 0)
		{
			// delete 1/4
			test_tag_delete(id);
			TEST_CHECK_EQ(nvmtnvj_delete(id), 0);
		}
		else
		{
			// write 3/4
			TEST_CHECK_EQ(store_random_tag_in_nvm_and_testtag(id, TAG_MAX_SIZE, &p), 0);
		}
	}
	// check all data is consistent
	TEST_CHECK_EQ(test_tag_compare_all(), 0);

	// make sure we wear equally
	uint32_t max_diff = 0;
	uint32_t base = flash_emul_get_sector_erases(0);
	for (uint32_t i = 0; i < flash_emul_get()->sectors; i++)
	{
		uint32_t erases = flash_emul_get_sector_erases(i);
		uint32_t diff = erases > base ? (erases - base) : (base - erases);
		if (diff > max_diff)
			max_diff = diff;
	}
	TEST_CHECK_LE(max_diff, 2);
	return 0;
}
TEST_END;

TEST(wear_unbalanced)
{
	uint32_t cycles = WEAR_CYCLES;
	prand_t p;
	prand_seed(&p, 231111);
	TEST_CHECK_EQ(nvmtnvj_format(0, BLOCK_PAGES, PAGE_COUNT / BLOCK_PAGES, TAG_MAX_SIZE), 0);
	TEST_CHECK_EQ(nvmtnvj_mount(0, 8), 0);
	const uint32_t tags_per_block = nvmtnvj_test_tags_per_block();
	const uint32_t blocks = PAGE_COUNT / BLOCK_PAGES;
	// assume our tag id space spans half of available data blocks to allow for 50% chance of rewrite
	const uint16_t max_tag_id = tags_per_block * (blocks - 1) / 2;
	// fill a block with tags never to be deleted
	for (uint16_t fix_tag_id = max_tag_id; fix_tag_id < max_tag_id + tags_per_block; fix_tag_id++)
		TEST_CHECK_EQ(store_random_tag_in_nvm_and_testtag(fix_tag_id, TAG_MAX_SIZE, &p), 0);
	// cycle
	while (cycles--)
	{
		uint16_t id = prand(&p, 16) % max_tag_id;
		if (prand(&p, 2) == 0)
		{
			// delete 1/4
			test_tag_delete(id);
			TEST_CHECK_EQ(nvmtnvj_delete(id), 0);
		}
		else
		{
			// write 3/4
			TEST_CHECK_EQ(store_random_tag_in_nvm_and_testtag(id, TAG_MAX_SIZE, &p), 0);
		}
	}
	// check all data is consistent
	TEST_CHECK_EQ(test_tag_compare_all(), 0);

	// make sure we wear unequally
	uint32_t max_diff = 0;
	uint32_t base = flash_emul_get_sector_erases(0);
	for (uint32_t i = 0; i < flash_emul_get()->sectors; i++)
	{
		uint32_t erases = flash_emul_get_sector_erases(i);
		uint32_t diff = erases > base ? (erases - base) : (base - erases);
		if (diff > max_diff)
			max_diff = diff;
	}
	TEST_CHECK_GE(max_diff, (WEAR_CYCLES / tags_per_block) / 2);
	return 0;
}
TEST_END;

SUITE_TESTS(nvmtnvj);
ADD_TEST(format);
ADD_TEST(mount);
ADD_TEST(write_read_delete);
ADD_TEST(aborted_write);
ADD_TEST(aborted_gc);
ADD_TEST(fill);
ADD_TEST(mount_scan_first_sector_borked);
ADD_TEST(wear_balanced);
ADD_TEST(wear_unbalanced);
SUITE_END(nvmtnvj);
