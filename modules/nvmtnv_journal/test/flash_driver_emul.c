#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "bmtypes.h"
#include "flash_driver.h"
#include "flash_emul.h"
#include "testbench.h"

#define FLASH_INITIATED 0x12312345

typedef struct list_entry
{
    uint8_t *mem;
    uint8_t *old_mem;
    struct list_entry *next;
} list_entry_t;

static uint32_t flash_initiated = 0;
static flash_emul_t emul;
static list_entry_t *list = NULL;
struct sector_meta {
    uint32_t erase_counts;
} *sector_meta = NULL;

void flash_emul_set(const flash_emul_t *f)
{
    emul = *f;
    flash_initiated = FLASH_INITIATED;
    memset(&emul.private, 0, sizeof(emul.private));
    list = NULL;
    if (sector_meta) {
        free(sector_meta);
    }
    sector_meta = malloc(emul.sectors * sizeof(sector_meta));
    memset(sector_meta, 0, emul.sectors * sizeof(sector_meta));
}

const flash_emul_t *flash_emul_get(void)
{
    if (flash_initiated == FLASH_INITIATED)
        return &emul;
    return NULL;
}

int flash_init(void)
{
    return flash_initiated == FLASH_INITIATED ? 0 : ERR_FLASH_NOINIT;
}

uint32_t flash_emul_get_sector_erases(uint32_t sector) {
    return sector_meta[sector].erase_counts;
}

void flash_emul_push(void)
{
    uint8_t *new_mem = malloc(emul.mem_size);
    memcpy(new_mem, emul.mem, emul.mem_size);
    list_entry_t *l = malloc(sizeof(list_entry_t));
    l->mem = new_mem;
    l->old_mem = emul.mem;
    l->next = list;
    list = l;
    emul.mem = new_mem;
}

int flash_emul_pop(void)
{
    if (list == NULL)
        return -1;
    list_entry_t *n = list->next;
    emul.mem = list->old_mem;
    free(list->mem);
    free(list);
    list = n;
    return 0;
}

int flash_deinit(void)
{
    flash_initiated = 0;
    while (flash_emul_pop() >= 0)
        ;
    if (sector_meta) {
        free(sector_meta);
        sector_meta = NULL;
    }

    return 0;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors)
{
    if (type != FLASH_TYPE_CODE_BANK0)
        return -1;
    *sector = emul.sector_offset;
    *num_sectors = emul.sectors;
    return 0;
}

int flash_get_sector_for_address(const void *address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size)
{
    if (address < emul.flash_address || (intptr_t)address >= (intptr_t)emul.flash_address + emul.sector_size * emul.sectors)
        return -1;
    *sector = ((intptr_t)address - (intptr_t)emul.flash_address) / emul.sector_size + emul.sector_offset;
    *offset = ((intptr_t)address - (intptr_t)emul.flash_address) % emul.sector_size;
    *sector_size = emul.sector_size;
    return 0;
}

int flash_get_sector_size(uint32_t sector)
{
    if (sector < emul.sector_offset || sector >= emul.sector_offset + emul.sectors)
        return -1;
    return emul.sector_size;
}

int flash_get_address_for_sector(uint32_t sector, void **address)
{
    if (sector < emul.sector_offset || sector >= emul.sector_offset + emul.sectors)
        return -1;
    *address = (uint8_t *)emul.flash_address + (sector - emul.sector_offset) * emul.sector_size;
    return 0;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation)
{
    if (operation == FLASH_OP_READ)
        return emul.read_alignment;
    else
        return emul.write_alignment;
}

static uint8_t erased_byte(void)
{
    return emul.flags & FLASH_EMUL_WRITE_BY_AND ? 0xff : 0x00;
}

int flash_erase(uint32_t sector)
{
    if (sector < emul.sector_offset || sector >= emul.sector_offset + emul.sectors)
        return -1;
    const uint8_t e = erased_byte();
    for (uint32_t i = 0; i < emul.sector_size; i++)
        emul.mem[i + (sector - emul.sector_offset) * emul.sector_size] = e;
    sector_meta[sector].erase_counts++;
    return 0;
}

int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length)
{
    if (sector < emul.sector_offset || sector >= emul.sector_offset + emul.sectors)
        return -1;
    if (offset >= emul.sector_size)
        return 0;
    if (length + offset > emul.sector_size)
        length = emul.sector_size - offset;
    if (emul.write_alignment > 0 && (offset % emul.write_alignment != 0 || length % emul.write_alignment != 0))
        return ERR_FLASH_ALIGN;
    const uint8_t e = erased_byte();
    for (uint32_t i = 0; i < length; i++)
    {
        const uint8_t d = *data++;
        uint32_t ix = i + (sector - emul.sector_offset) * emul.sector_size + offset;
        if ((emul.flags & FLASH_EMUL_ALLOW_BIT_PULLING) == 0 && emul.mem[ix] != e)
            return ERR_FLASH_OTHER;
        if (emul.flags & FLASH_EMUL_WRITE_BY_AND)
            emul.mem[ix] &= d;
        else
            emul.mem[ix] |= d;
        emul.private.bytes_written++;
        if (emul.private.write_fail_countdown > 0)
        {
            emul.private.write_fail_countdown--;
            if (emul.private.write_fail_countdown == 0)
                return FLASH_EMUL_FORCE_FAIL;
        }
        if (emul.private.write_garbage_countdown > 0)
            emul.private.write_garbage_countdown--;
        if (emul.private.write_garbage_count > 0 && emul.private.write_garbage_countdown == 0)
        {
            emul.mem[ix] = ~emul.mem[ix];
            emul.private.write_garbage_count--;
        }
    }
    return length;
}

int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length)
{
    if (sector < emul.sector_offset || sector >= emul.sector_offset + emul.sectors)
        return -1;
    if (offset >= emul.sector_size)
        return 0;
    if (length + offset > emul.sector_size)
        length = emul.sector_size - offset;
    if (emul.read_alignment > 0 && (offset % emul.read_alignment != 0 || length % emul.read_alignment != 0))
        return ERR_FLASH_ALIGN;
    uint32_t ix = (sector - emul.sector_offset) * emul.sector_size + offset;
    for (uint32_t i = 0; i < length; i++)
    {
        *data = emul.mem[ix + i];
        emul.private.bytes_read++;
        if (emul.private.read_fail_countdown > 0)
        {
            emul.private.read_fail_countdown--;
            if (emul.private.read_fail_countdown == 0)
                return FLASH_EMUL_FORCE_FAIL;
        }
        if (emul.private.read_garbage_countdown > 0)
            emul.private.read_garbage_countdown--;
        if (emul.private.read_garbage_count > 0 && emul.private.read_garbage_countdown == 0)
        {
            *data = ~*data;
            emul.private.read_garbage_count--;
        }
        data++;
    }

    return length;
}

void flash_emul_read_fail_after_bytes(uint32_t byte_countdown)
{
    emul.private.read_fail_countdown = byte_countdown;
}
void flash_emul_read_scramble_after_bytes(uint32_t byte_countdown, uint32_t bytes_to_scramble)
{
    emul.private.read_garbage_countdown = byte_countdown;
    emul.private.read_garbage_count = bytes_to_scramble;
}
void flash_emul_write_fail_after_bytes(uint32_t byte_countdown)
{
    emul.private.write_fail_countdown = byte_countdown;
}
void flash_emul_write_scramble_after_bytes(uint32_t byte_countdown, uint32_t bytes_to_scramble)
{
    emul.private.write_garbage_countdown = byte_countdown;
    emul.private.write_garbage_count = bytes_to_scramble;
}

void flash_emul_reset_bytes_written_count(void)
{
    emul.private.bytes_written = 0;
}
uint32_t flash_emul_get_bytes_written_count(void)
{
    return emul.private.bytes_written;
}
void flash_emul_reset_bytes_read_count(void)
{
    emul.private.bytes_read = 0;
}
uint32_t flash_emul_get_bytes_read_count(void)
{
    return emul.private.bytes_read;
}
