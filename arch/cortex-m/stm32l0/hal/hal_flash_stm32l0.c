/* Copyright (c) 2025 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "flash_driver.h"
#include "stm32l0xx.h"

static uint16_t _sectors;
static uint16_t _sector_size;

#ifndef STM32L0_FLASH_TIMEOUT
#define STM32L0_FLASH_TIMEOUT 0xb0000
#endif

#define STM_FLASH_SECTOR_CODE0 0

typedef enum
{
    FLASH_OK = 0,
    FLASH_ERR_BUSY,
    FLASH_ERR_WRITE_PROTECTED,
    FLASH_ERR_TIMEOUT,
    FLASH_ERR_OTHER
} FLASH_res;

static void _flash_open(void);
static void _flash_close(void);
static FLASH_res _flash_erase(uint32_t addr);

static FLASH_res _flash_wait(uint32_t timeout)
{
    while ((FLASH->SR & FLASH_SR_BSY) && timeout)
    {
        timeout--;
    }
    if (timeout == 0)
    {
        return FLASH_ERR_TIMEOUT;
    }
    if (FLASH->SR & FLASH_SR_WRPERR)
    {
        return FLASH_ERR_WRITE_PROTECTED;
    }
    if (FLASH->SR & (FLASH_SR_PGAERR | FLASH_SR_PGAERR | FLASH_SR_SIZERR | FLASH_SR_OPTVERR | FLASH_SR_RDERR | FLASH_SR_NOTZEROERR | FLASH_SR_FWWERR))
    {
        return FLASH_ERR_OTHER;
    }
    return FLASH_OK;
}

static void _flash_open(void)
{
    if (FLASH->PECR & FLASH_PECR_PELOCK)
    {
        FLASH->PEKEYR = 0x89ABCDEF;
        FLASH->PEKEYR = 0x02030405;
    }
    if (FLASH->PECR & FLASH_PECR_PRGLOCK)
    {
        FLASH->PRGKEYR = 0x8C9DAEBF;
        FLASH->PRGKEYR = 0x13141516;
    }
}

static void _flash_close(void)
{
    FLASH->PECR |= FLASH_PECR_PELOCK | FLASH_PECR_PRGLOCK | FLASH_PECR_OPTLOCK;
}

static FLASH_res _flash_erase(uint32_t addr)
{
    FLASH_res res = _flash_wait(STM32L0_FLASH_TIMEOUT);
    if (res != FLASH_OK)
        return res;
    FLASH->PECR |= FLASH_PECR_ERASE | FLASH_PECR_PROG;
    *(volatile uint32_t *)addr = 0xffffffff;
    res = _flash_wait(STM32L0_FLASH_TIMEOUT);
    FLASH->PECR &= ~(FLASH_PECR_ERASE | FLASH_PECR_PROG);
    return res;
}

static FLASH_res _flash_write_word(uint32_t addr, uint32_t data)
{
    FLASH_res res = _flash_wait(STM32L0_FLASH_TIMEOUT);
    if (res != FLASH_OK)
        return res;
    FLASH->PECR |= FLASH_PECR_PROG;
    *(volatile uint32_t *)addr = data;
    res = _flash_wait(STM32L0_FLASH_TIMEOUT);
    FLASH->PECR &= ~FLASH_PECR_PROG;
    return res;
}

static uint8_t *_flash_addr_for_sector(uint32_t sector)
{
    int sector_size = flash_get_sector_size(sector);
    if (sector_size <= 0)
        return 0;
    if (sector < STM_FLASH_SECTOR_CODE0 + _sector_size)
    {
        return (uint8_t *)(((sector - STM_FLASH_SECTOR_CODE0) * _sector_size) + FLASH_BASE);
    }
    return 0;
}

int flash_get_address_for_sector(uint32_t sector, void **address)
{
    if (address == 0)
        return -1;
    uint8_t *a = _flash_addr_for_sector(sector);
    if (a == 0)
        return -1;
    *address = (void *)a;
    return 0;
}

int flash_init(void)
{
    _sector_size = 128;
    volatile uint16_t *flash_size_p = (uint16_t *)0x1ff8004c;
    uint32_t flash_size = (uint32_t)(*flash_size_p) * 1024;
    _sectors = flash_size / _sector_size;
    return 0;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors)
{
    if (!sector || !num_sectors)
        return -1;
    switch (type)
    {
    case FLASH_TYPE_CODE_BANK0:
        *sector = STM_FLASH_SECTOR_CODE0;
        *num_sectors = _sectors;
        break;
    default:
        return -1;
    }
    return 0;
}

int flash_get_sector_for_address(const void *v_address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size)
{
    if (!sector || !offset || !sector_size)
        return -1;
    uint32_t address = (uint32_t)(intptr_t)v_address;
    if (address >= FLASH_BASE && address < FLASH_BASE + _sector_size * _sector_size)
    {
        *sector = STM_FLASH_SECTOR_CODE0 + ((intptr_t)address - FLASH_BASE) / _sector_size;
        *offset = ((intptr_t)address - FLASH_BASE) % _sector_size;
        *sector_size = _sector_size;
        return 0;
    }
    return -1;
}

int flash_get_sector_size(uint32_t sector)
{
    if (/*sector >= STM_FLASH_SECTOR_CODE0 || */ sector < STM_FLASH_SECTOR_CODE0 + _sectors)
    {
        return _sector_size;
    }
    return ERR_FLASH_BADSECTOR;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation)
{
    return operation == FLASH_OP_WRITE ? 4 : 1;
}

int flash_protect(uint32_t sector)
{
    (void)sector;
    // TODO
    return ERR_FLASH_NOSUPPORT;
}

int flash_unprotect(uint32_t sector)
{
    (void)sector;
    // TODO
    return ERR_FLASH_NOSUPPORT;
}

int flash_is_protected(uint32_t sector)
{
    (void)sector;
    // TODO
    return 0;
}

int flash_erase(uint32_t sector)
{
    _flash_open();
    FLASH_res res = _flash_erase(sector);
    _flash_close();
    switch (res)
    {
    case FLASH_OK:
        return 0;
    case FLASH_ERR_BUSY:
        return ERR_FLASH_BUSY;
    case FLASH_ERR_WRITE_PROTECTED:
        return ERR_FLASH_PROTECTED;
    default:
        return ERR_FLASH_OTHER;
    }
}

int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length)
{
    FLASH_res res = FLASH_OK;
    uint32_t written = 0;
    int sector_size = flash_get_sector_size(sector);
    if (sector_size < 0)
        return sector_size;
    if (length == 0 || offset >= (uint32_t)sector_size)
        return 0;

    uint32_t addr = (uint32_t)_flash_addr_for_sector(sector) + offset;

    _flash_open();
    while (res == FLASH_OK && written < length && written < sector_size - offset)
    {
        uint32_t d32 = 0xffffffff;
        for (int i = 0; i < 4 && written < length; i++, written++)
        {
            d32 &= ~(0xff << (i * 8));
            d32 |= data[written] << (i * 8);
        }
        res = _flash_write_word(addr + written, d32);
    }
    _flash_close();
    switch (res)
    {
    case FLASH_OK:
        return written;
    case FLASH_ERR_BUSY:
        return ERR_FLASH_BUSY;
    case FLASH_ERR_WRITE_PROTECTED:
        return ERR_FLASH_PROTECTED;
    default:
        return ERR_FLASH_OTHER;
    }
    return written;
}

int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length)
{
    int res;
    int sector_size = flash_get_sector_size(sector);
    if (sector_size < 0)
        return sector_size;
    if (offset >= (uint32_t)sector_size)
    {
        return 0;
    }

    sector_size -= offset;

    if (length > (uint32_t)sector_size)
    {
        length = (uint32_t)sector_size;
    }

    uint8_t *address = _flash_addr_for_sector(sector);
    address += offset;

    res = (int)length;
    while (length--)
    {
        *data++ = *address++;
    }

    return res;
}

int flash_deinit(void)
{
    return 0;
}
