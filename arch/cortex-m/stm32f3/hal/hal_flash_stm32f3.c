/* Copyright (c) 2026 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "flash_driver.h"
#include "flash_stm32f3.h"
#include "stm32f3xx.h"

static uint16_t _sectors;
static uint16_t _sector_size;

#ifndef STM32F3_FLASH_TIMEOUT
#define STM32F3_FLASH_TIMEOUT 0xb0000
#endif

typedef enum {
  FLASH_OK = 0,
  FLASH_ERR_BUSY,
  FLASH_ERR_WRITE_PROTECTED,
  FLASH_ERR_TIMEOUT,
  FLASH_ERR_OTHER
} FLASH_res;

static FLASH_res _flash_status(void) {
    FLASH_res res = FLASH_OK;
    if ((FLASH->SR & FLASH_SR_BSY) == FLASH_SR_BSY) {
        res = FLASH_ERR_BUSY;
    } else if ((FLASH->SR & FLASH_SR_PGERR) != 0) {
        res = FLASH_ERR_OTHER;
    } else if ((FLASH->SR & FLASH_SR_WRPERR) != 0) {
        res = FLASH_ERR_WRITE_PROTECTED;
    }
    return res;
}

static FLASH_res _flash_wait(uint32_t timeout) {
    FLASH_res res;
    while (((res = _flash_status()) == FLASH_ERR_BUSY) && timeout) {
        timeout--;
    }
    return timeout == 0 ? FLASH_ERR_TIMEOUT : res;
}

static void _flash_open(void) {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;
}

static void _flash_close(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

static FLASH_res _flash_erase(uint32_t addr) {
    FLASH_res res = _flash_wait(STM32F3_FLASH_TIMEOUT);
    if (res == FLASH_OK) {
        FLASH->CR |= FLASH_CR_PER;
        FLASH->AR = addr;
        FLASH->CR |= FLASH_CR_STRT;
        res = _flash_wait(STM32F3_FLASH_TIMEOUT);
        FLASH->CR &= ~FLASH_CR_PER;
    }
    return res;
}

static FLASH_res _flash_write_hword(uint32_t addr, uint16_t data) {
    if (data == *((uint16_t *)addr)) return FLASH_OK;
    FLASH_res res = _flash_wait(STM32F3_FLASH_TIMEOUT);
    if (res == FLASH_OK) {
        FLASH->CR |= FLASH_CR_PG;
        *(volatile uint16_t *)addr = data;
        res = _flash_wait(STM32F3_FLASH_TIMEOUT);
        FLASH->CR &= ~FLASH_CR_PG;
    }
    return res;
}

static uint8_t *_flash_addr_for_sector(uint32_t sector) {
    int sector_size = flash_get_sector_size(sector);
    if (sector_size <= 0) return 0;
    if (sector < STM_FLASH_SECTOR_CODE0 + _sectors) {
        return (uint8_t *)(((sector - STM_FLASH_SECTOR_CODE0) * _sector_size) + FLASH_BASE);
    }
    return 0;
}

int flash_get_address_for_sector(uint32_t sector, void **address) {
    if (address == 0) return -1;
    uint8_t *a = _flash_addr_for_sector(sector);
    if (a == 0) return -1;
    *address = (void *)a;
    return 0;
}

int flash_init(void) {
    volatile uint16_t *flash_size = (volatile uint16_t *)0x1ffff7cc;
    uint32_t size = *flash_size;
    _sector_size = 2048;
    _sectors = size * 1024 / _sector_size;
    return _sector_size != 0 ? 0 : ERR_FLASH_NOSUPPORT;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors) {
    if (!sector || !num_sectors) return -1;
    switch (type) {
    case FLASH_TYPE_CODE_BANK0:
        *sector = STM_FLASH_SECTOR_CODE0;
        *num_sectors = _sectors;
        break;
    default:
        return -1;
    }
    return 0;
}

int flash_get_sector_for_address(const void *v_address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size) {
    if (!sector || !offset || !sector_size) return -1;
    uint32_t address = (uint32_t)(intptr_t)v_address;
    if (address >= FLASH_BASE && address < FLASH_BASE + _sectors * _sector_size) {
        *sector = STM_FLASH_SECTOR_CODE0 + ((intptr_t)address - FLASH_BASE) / _sector_size;
        *offset = ((intptr_t)address - FLASH_BASE) % _sector_size;
        *sector_size = _sector_size;
        return 0;
    }
    return -1;
}

int flash_get_sector_size(uint32_t sector) {
    if (sector < STM_FLASH_SECTOR_CODE0 + _sectors) {
        return _sector_size;
    }
    return ERR_FLASH_BADSECTOR;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation) {
    (void)sector;
    return operation == FLASH_OP_WRITE ? 2 : 1;
}

int flash_protect(uint32_t sector) { (void)sector; return ERR_FLASH_NOSUPPORT; }
int flash_unprotect(uint32_t sector) { (void)sector; return ERR_FLASH_NOSUPPORT; }
int flash_is_protected(uint32_t sector) { (void)sector; return 0; }

int flash_erase(uint32_t sector) {
    uint8_t *addr = _flash_addr_for_sector(sector);
    if (addr == 0) return ERR_FLASH_BADSECTOR;
    _flash_open();
    FLASH_res res = _flash_erase((uint32_t)(intptr_t)addr);
    _flash_close();
    switch (res) {
    case FLASH_OK: return 0;
    case FLASH_ERR_BUSY: return ERR_FLASH_BUSY;
    case FLASH_ERR_WRITE_PROTECTED: return ERR_FLASH_PROTECTED;
    default: return ERR_FLASH_OTHER;
    }
}

int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length) {
    int sector_size = flash_get_sector_size(sector);
    if (sector_size < 0) return sector_size;
    if (length == 0 || offset >= (uint32_t)sector_size) return 0;

    uint8_t *addr = _flash_addr_for_sector(sector);
    int written = 0;
    FLASH_res res = FLASH_OK;
    _flash_open();
    if (offset & 1) {
        uint16_t d16 = (data[written] << 8) | 0xff;
        res = _flash_write_hword((uint32_t)addr - 1 + offset, d16);
        offset += 1;
        written += 1;
        length -= 1;
    }
    while (res == FLASH_OK && offset < (uint32_t)sector_size && length > 1) {
        uint16_t d16 = (data[written + 1] << 8) | data[written];
        res = _flash_write_hword((uint32_t)addr + offset, d16);
        offset += 2;
        written += 2;
        length -= 2;
    }
    if (res == FLASH_OK && length > 0 && offset < (uint32_t)sector_size) {
        uint16_t d16 = 0xff00 | data[written];
        res = _flash_write_hword((uint32_t)addr + offset, d16);
        written += 1;
    }
    _flash_close();
    switch (res) {
    case FLASH_OK: return written;
    case FLASH_ERR_BUSY: return ERR_FLASH_BUSY;
    case FLASH_ERR_WRITE_PROTECTED: return ERR_FLASH_PROTECTED;
    default: return ERR_FLASH_OTHER;
    }
}

int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length) {
    int sector_size = flash_get_sector_size(sector);
    if (sector_size < 0) return sector_size;
    if (offset >= (uint32_t)sector_size) return 0;
    sector_size -= (int)offset;
    if (length > (uint32_t)sector_size) length = (uint32_t)sector_size;

    uint8_t *address = _flash_addr_for_sector(sector) + offset;
    int res = (int)length;
    while (length--) {
        *data++ = *address++;
    }
    return res;
}
