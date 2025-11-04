#pragma once

#include "bmtypes.h"

#define FLASH_EMUL_ALLOW_BIT_PULLING (1 << 0)
#define FLASH_EMUL_DISALLOW_BIT_PULLING (0 << 0)
#define FLASH_EMUL_WRITE_BY_OR (0 << 1)
#define FLASH_EMUL_WRITE_BY_AND (1 << 1)
#define FLASH_EMUL_FORCE_FAIL -0x12345678

typedef struct
{
    uint8_t *mem;
    uint32_t mem_size;
    uint32_t sectors;
    uint32_t sector_size;
    uint32_t sector_offset;
    uint8_t read_alignment;
    uint8_t write_alignment;
    void *flash_address;
    uint32_t flags;
    struct
    {
        uint32_t read_garbage_countdown;
        uint32_t read_garbage_count;
        uint32_t read_fail_countdown;
        uint32_t write_garbage_countdown;
        uint32_t write_garbage_count;
        uint32_t write_fail_countdown;
        uint32_t bytes_written;
        uint32_t bytes_read;
    } private;
} flash_emul_t;

void flash_emul_set(const flash_emul_t *f);
const flash_emul_t *flash_emul_get(void);
// any read operation will return FLASH_EMUL_FORCE_FAIL after given bytes
void flash_emul_read_fail_after_bytes(uint32_t byte_countdown);
// any read operation will return given garbage data after given bytes
void flash_emul_read_scramble_after_bytes(uint32_t byte_countdown, uint32_t bytes_to_scramble);
// any write operation will return FLASH_EMUL_FORCE_FAIL after given bytes
void flash_emul_write_fail_after_bytes(uint32_t byte_countdown);
// any write operation will write given garbage data after given bytes
void flash_emul_write_scramble_after_bytes(uint32_t byte_countdown, uint32_t bytes_to_scramble);
// push current flash content
void flash_emul_push(void);
// pop previously pushed flash content
int flash_emul_pop(void);
// reset byte write counter
void flash_emul_reset_bytes_written_count(void);
// return number of written bytes
uint32_t flash_emul_get_bytes_written_count(void);
// reset byte read counter
void flash_emul_reset_bytes_read_count(void);
// return number of read bytes
uint32_t flash_emul_get_bytes_read_count(void);
uint32_t flash_emul_get_sector_erases(uint32_t sector);