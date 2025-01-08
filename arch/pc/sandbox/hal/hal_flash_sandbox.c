/* Copyright (c) 2023 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include <string.h>
#include "bmtypes.h"
#include "flash_driver.h"

#ifndef FLASH_SANDBOX_SECTOR_SIZE
#define FLASH_SANDBOX_SECTOR_SIZE   1024
#endif

#ifndef FLASH_SANDBOX_NUM_SECTORS
#define FLASH_SANDBOX_NUM_SECTORS   512
#endif

static uint8_t mem[FLASH_SANDBOX_NUM_SECTORS * FLASH_SANDBOX_SECTOR_SIZE];

int flash_get_address_for_sector(uint32_t sector, void **address) {
    if (sector >= FLASH_SANDBOX_NUM_SECTORS) {
        return ERR_FLASH_OTHER;
    }
    *address = mem + sector * FLASH_SANDBOX_SECTOR_SIZE;
    return 0;
}

int flash_init(void) {
    memset(mem, 0xff, sizeof(mem));
    return 0;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors) {
    if (type != FLASH_TYPE_CODE_BANK0) {
        return ERR_FLASH_OTHER;
    }
    *sector = 0;
    *num_sectors = FLASH_SANDBOX_NUM_SECTORS;
    return 0;
}

int flash_get_sector_for_address(const void *address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size) {
    if (address < mem && address >= mem + sizeof(mem)) {
        return ERR_FLASH_OTHER;
    }
    *sector = (intptr_t)(address - (intptr_t)mem) / FLASH_SANDBOX_SECTOR_SIZE;
    *offset = (intptr_t)(address - (intptr_t)mem) % FLASH_SANDBOX_SECTOR_SIZE;
    *sector_size = FLASH_SANDBOX_SECTOR_SIZE;
    return 0;
}

int flash_get_sector_size(uint32_t sector) {
    if (sector >= FLASH_SANDBOX_NUM_SECTORS) {
        return ERR_FLASH_OTHER;
    }
    return FLASH_SANDBOX_SECTOR_SIZE;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation) {
    if (sector >= FLASH_SANDBOX_NUM_SECTORS) {
        return ERR_FLASH_OTHER;
    }
    return 1;
}

int flash_protect(uint32_t sector) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_unprotect(uint32_t sector) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_is_protected(uint32_t sector) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_erase(uint32_t sector) {
    if (sector >= FLASH_SANDBOX_NUM_SECTORS) {
        return ERR_FLASH_OTHER;
    }
    memset(mem + sector * FLASH_SANDBOX_SECTOR_SIZE, 0xff, FLASH_SANDBOX_SECTOR_SIZE);
    return 0;
}

int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length) {
    if (sector >= FLASH_SANDBOX_NUM_SECTORS) {
        return ERR_FLASH_OTHER;
    }
    if (offset > FLASH_SANDBOX_SECTOR_SIZE)
        return 0;
    int remaining_size = FLASH_SANDBOX_SECTOR_SIZE - offset;
    if (length > remaining_size) 
        length = remaining_size;
    for (int i = 0; i < length; i++) {
        mem[sector * FLASH_SANDBOX_SECTOR_SIZE + offset + i] &= data[i];
    }
    return length;
}

int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length) {
    if (sector >= FLASH_SANDBOX_NUM_SECTORS) {
        return ERR_FLASH_OTHER;
    }
    if (offset > FLASH_SANDBOX_SECTOR_SIZE)
        return 0;
    int remaining_size = FLASH_SANDBOX_SECTOR_SIZE - offset;
    if (length > remaining_size) 
        length = remaining_size;
    memcpy(data, mem + sector * FLASH_SANDBOX_SECTOR_SIZE + offset, length);
    return length;
}

int flash_deinit(void) {
    return 0;
}
