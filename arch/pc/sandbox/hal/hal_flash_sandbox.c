/* Copyright (c) 2023 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "flash_driver.h"

int flash_get_address_for_sector(uint32_t sector, void **address) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_init(void) {
    return 0;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_get_sector_for_address(uint32_t address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_get_sector_size(uint32_t sector) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation) {
    return ERR_FLASH_NOSUPPORT;
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
    return ERR_FLASH_NOSUPPORT;
}

int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length) {
    return ERR_FLASH_NOSUPPORT;
}

int flash_deinit(void) {
    return 0;
}