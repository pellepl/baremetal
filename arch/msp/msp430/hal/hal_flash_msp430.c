/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "flash_driver.h"

// TODO PETER

int flash_init(void) {
    return -1;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors) {
    return -1;
}

int flash_get_sector_for_address(uint32_t address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size) {
    return -1;
}

int flash_get_sector_size(uint32_t sector) {
    return -1;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation) {
    return -1;
}

int flash_protect(uint32_t sector) {
    return -1;
}

int flash_unprotect(uint32_t sector) {
    return -1;
}

int flash_is_protected(uint32_t sector) {
    return -1;
}

int flash_erase(uint32_t sector) {
    return -1;
}

int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length) {
    return -1;
}

int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length) {
    return -1;
}

int flash_deinit(void) {
    return -1;
}
   