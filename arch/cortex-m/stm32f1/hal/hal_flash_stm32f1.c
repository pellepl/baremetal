/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "bmtypes.h"
#include "flash_driver.h"
#include "flash_stm32f1.h"
#include "stm32f1xx.h"

static uint16_t _sectors;
static uint16_t _sector_size;

#ifndef STM32F1_FLASH_TIMEOUT
#define STM32F1_FLASH_TIMEOUT 0xb0000
#endif

typedef enum {
  FLASH_OK = 0,
  FLASH_ERR_BUSY,
  FLASH_ERR_WRITE_PROTECTED,
  FLASH_ERR_TIMEOUT,
  FLASH_ERR_OTHER
} FLASH_res;

static void _flash_open(void);
static void _flash_close(void);
static FLASH_res _flash_erase(uint32_t addr);
static FLASH_res _flash_write_hword(uint32_t addr, uint16_t data);

static FLASH_res _flash_status(void) {
    FLASH_res res = FLASH_OK;

    if ((FLASH->SR & FLASH_SR_BSY) == FLASH_SR_BSY) {
        res = FLASH_ERR_BUSY;
    } else if ((FLASH->SR & FLASH_SR_PGERR) != 0) {
        res = FLASH_ERR_OTHER;
    } else if ((FLASH->SR & FLASH_SR_WRPRTERR) != 0) {
        res = FLASH_ERR_WRITE_PROTECTED;
    } else {
        res = FLASH_OK;
    }
    return res;
}

static FLASH_res _flash_wait(uint32_t timeout) {
    FLASH_res res;
    while (((res = _flash_status()) == FLASH_ERR_BUSY) && timeout) {
        timeout--;
    }
    if (timeout == 0) {
        res = FLASH_ERR_TIMEOUT;
    }
    return res;
}

static void _flash_open() {
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->SR = (FLASH_SR_BSY | FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
}

static void _flash_close() {
    FLASH->CR |= FLASH_CR_LOCK;
}

static FLASH_res _flash_erase(uint32_t addr) {
    FLASH_res res = _flash_wait(STM32F1_FLASH_TIMEOUT);
    if (res == FLASH_OK) {
        FLASH->CR |= FLASH_CR_PER;
        FLASH->AR = addr;
        FLASH->CR |= FLASH_CR_STRT;
        res = _flash_wait(STM32F1_FLASH_TIMEOUT );
        FLASH->CR &= ~FLASH_CR_PER;
    }
    return res;
}

static FLASH_res _flash_write_hword(uint32_t addr, uint16_t data) {
    if (data == *((uint16_t *)addr)) return FLASH_OK;
    FLASH_res res = _flash_wait(STM32F1_FLASH_TIMEOUT);
    if (res == FLASH_OK) {
        FLASH->CR |= FLASH_CR_PG;
        *(volatile uint16_t*) addr = data;
        res = _flash_wait(STM32F1_FLASH_TIMEOUT);
        FLASH->CR &= ~FLASH_CR_PG;
    }
    return res;
}

static uint8_t *_flash_addr_for_sector(uint32_t sector) {
    int sector_size = flash_get_sector_size(sector);
    if (sector_size <= 0) return 0;
    if (sector < STM_FLASH_SECTOR_CODE0 + _sector_size) {
        return (uint8_t *)(((sector - STM_FLASH_SECTOR_CODE0) * _sector_size) + FLASH_BASE);
    } else {
        return (uint8_t *)(OB_BASE);
    }
}

int flash_get_address_for_sector(uint32_t sector, void **address) {
    if (address == 0) return -1;
    uint8_t *a = _flash_addr_for_sector(sector);
    if (a == 0) return -1;
    *address = (void *)a;
    return 0;
}

int flash_init(void) {
    volatile uint16_t *FLASH_SIZE = (volatile uint16_t *)0x1ffff7e0; // in kB
    uint32_t size = *FLASH_SIZE;
    #if STMF10x_MEDIUM_DENSITY || STM10x_LOW_DENSITY
    _sector_size = 1024;
    #elif STMF10x_HIGH_DENSITY || STM10x_CONNECTIVITY
    _sector_size = 2048;
    #else
    _sector_size = size >= 128 ? 2048 : 1024;
    #endif
    _sectors = size * 1024 / _sector_size;
    #if 0 // won't work without debugger
    volatile uint32_t *DBGMCU_IDCODE = (volatile uint32_t *)0xe0042000;
    uint32_t code = *DBGMCU_IDCODE;
    uint32_t dev = (code & 0x0fff);
    _sector_size = 0;
    // see https://github.com/blacksphere/blackmagic/blob/master/src/target/stm32f1.c
    switch(dev) {
	case 0x410:  // medium density
	case 0x412:  // low density
	case 0x420:  // value, low-/medium density
		_sector_size = 1024;
		break;
	case 0x414:	 // high density
	case 0x418:  // connectivity
	case 0x428:	 // value, high density
        _sector_size = 2048;
        break;
	case 0x438:  // STM32F303x6/8 and STM32F328
	case 0x422:  // STM32F30x
	case 0x446:  // STM32F303xD/E and STM32F398xE
	case 0x432:  // STM32F37x
	case 0x439:  // STM32F302C8
        _sector_size = 2048;
        break;
    case 0x430: // xl density
        // two banks 512+256/512
        _sector_size = 2048;
        break;
	}

    if (_sector_size == 0) {
        // TODO stm32f4xx

        volatile uint32_t *DBGMCU_IDCODE_CM0 = (volatile uint32_t *)0x40015800;
        code = *DBGMCU_IDCODE_CM0;
        dev = (code & 0x0fff);
        switch(dev) {
        case 0x444:  // STM32F03 RM0091 Rev.7, STM32F030x[4|6] RM0360 Rev. 4
        case 0x445:  // STM32F04 RM0091 Rev.7, STM32F070x6 RM0360 Rev. 4
        case 0x440:  // STM32F05 RM0091 Rev.7, STM32F030x8 RM0360 Rev. 4
            _sector_size = 1024;
            break;
        case 0x448:  // STM32F07 RM0091 Rev.7, STM32F070xB RM0360 Rev. 4
        case 0x442:  // STM32F09 RM0091 Rev.7, STM32F030xC RM0360 Rev. 4
            _sector_size = 2048;
            break;
        }
    }

    _sectors = size * 1024 / _sector_size;
    #endif

    return _sector_size != 0 ? 0 : ERR_FLASH_NOSUPPORT;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors) {
    if (!sector || !num_sectors) return -1;
    switch (type) {
        case FLASH_TYPE_CODE_BANK0:
            *sector = STM_FLASH_SECTOR_CODE0;
            *num_sectors = _sectors;
            break;
        case FLASH_TYPE_USER0:
            *sector = STM_FLASH_SECTOR_OPT;
            *num_sectors = 1;
            break;
        default:
            return -1;
    }
    return 0;
}

int flash_get_sector_for_address(const void *v_address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size) {
    if (!sector || !offset || !sector_size) return -1;
    uint32_t address = (uint32_t)(intptr_t)v_address;
    if (address >= FLASH_BASE && address < FLASH_BASE + _sector_size * _sector_size) {
        *sector = STM_FLASH_SECTOR_CODE0 + ((intptr_t)address - FLASH_BASE) / _sector_size;
        *offset = ((intptr_t)address - FLASH_BASE) % _sector_size;
        *sector_size = _sector_size;
        return 0;
    } else if (address >= OB_BASE && address < OB_BASE+16) {
        *sector = STM_FLASH_SECTOR_OPT;
        *offset = address - OB_BASE;
        *sector_size = 16;
        return 0;
    }
    return -1;
}

int flash_get_sector_size(uint32_t sector) {
    if (/*sector >= STM_FLASH_SECTOR_CODE0 || */sector < STM_FLASH_SECTOR_CODE0 + _sectors) {
        return _sector_size;
    } else if (sector == STM_FLASH_SECTOR_OPT) {
       return 16;
    }
    return ERR_FLASH_BADSECTOR;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation) {
    return operation == FLASH_OP_WRITE ? 2 : 1;
}

int flash_protect(uint32_t sector) {
    (void)sector;
    // TODO
    return ERR_FLASH_NOSUPPORT;
}

int flash_unprotect(uint32_t sector) {
    (void)sector;
    // TODO
    return ERR_FLASH_NOSUPPORT;
}

int flash_is_protected(uint32_t sector) {
    (void)sector;
    // TODO
    return 0;
}

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
    FLASH_res res = 0;
    _flash_open();
    if (offset & 1) {
        uint16_t d16 = (data[written] << 8) | 0xff;
        res = _flash_write_hword((uint32_t)addr - 1 + offset, d16);
        offset += 1;
        written += 1;
        length -= 1;
    }
    while (res == 0 && offset < (uint32_t)sector_size && length > 1) {
        uint16_t d16 = (data[written+1] << 8) | data[written];
        res = _flash_write_hword((uint32_t)addr + offset, d16);
        offset += 2;
        written += 2;
        length -= 2;
    }
    if (res == 0 && length > 0 && offset < (uint32_t)sector_size) {
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
    int res;
    int sector_size = flash_get_sector_size(sector);
    if (sector_size < 0) return sector_size;
    if (offset >= (uint32_t)sector_size) {
        return 0;
    }

    sector_size -= offset;

    if (length > (uint32_t)sector_size) {
        length = (uint32_t)sector_size;
    }

    uint8_t *address = _flash_addr_for_sector(sector);
    address += offset;

    res = (int)length;
    while (length--) {
        *data++ = *address++;
    }

    return res;
}

int flash_deinit(void) {
    return 0;
}
    
