/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "flash_driver.h"
#include "flash_nrf52.h"
#include "nrf52.h"

#define FLASH_UICR_ADDR_START       (0x10001080)
#define FLASH_UICR_ADDR_END         (0x10001210)

int flash_init(void) {
    return 0;
}

int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors) {
    if (!sector || !num_sectors) return -1;
    switch (type) {
        case FLASH_TYPE_CODE_BANK0:
            *sector = NRF_FLASH_SECTOR_CODE;
            *num_sectors = NRF_FICR->CODESIZE;
            break;
        case FLASH_TYPE_USER0:
            *sector = NRF_FLASH_SECTOR_UICR;
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
    if (address >= FLASH_UICR_ADDR_START && address < FLASH_UICR_ADDR_END) {
        *sector = NRF_FLASH_SECTOR_UICR;
        *offset = address - FLASH_UICR_ADDR_START;
        *sector_size = flash_get_sector_size(*sector);
        return 0;
    } else {
        *sector = address / NRF_FICR->CODEPAGESIZE + NRF_FLASH_SECTOR_CODE;
        *offset = address % NRF_FICR->CODEPAGESIZE;
        *sector_size = flash_get_sector_size(*sector);
        return (*sector - NRF_FLASH_SECTOR_CODE) < NRF_FICR->CODESIZE ? 0 : -1;
    }
}

int flash_get_sector_size(uint32_t sector) {
    if (/*sector >= NRF_FLASH_SECTOR_CODE || */sector < NRF_FLASH_SECTOR_CODE + NRF_FICR->CODESIZE) {
        return NRF_FICR->CODEPAGESIZE;
    } else if (sector == NRF_FLASH_SECTOR_UICR) {
       return FLASH_UICR_ADDR_END - FLASH_UICR_ADDR_START;
    }
    return ERR_FLASH_BADSECTOR;
}

int flash_get_sector_alignment(uint32_t sector, flash_op_t operation) {
    (void)sector;
    return operation == FLASH_OP_WRITE ? 4 : 1;
}

int flash_protect(uint32_t sector) {
    (void)sector;
    return ERR_FLASH_NOSUPPORT;
}

int flash_unprotect(uint32_t sector) {
    (void)sector;
    return ERR_FLASH_NOSUPPORT;
}

int flash_is_protected(uint32_t sector) {
    (void)sector;
    return 0;
}

static int validate_sector_and_flash_state(uint32_t sector) {
    if ((/*sector < NRF_FLASH_SECTOR_CODE || */sector >= NRF_FLASH_SECTOR_CODE + NRF_FICR->CODESIZE) &&
        (sector != NRF_FLASH_SECTOR_UICR)) {
       return ERR_FLASH_BADSECTOR;
    }
    if (NRF_NVMC->READY == 0) {
        return ERR_FLASH_BUSY;
    }
    return 0;
}

int flash_erase(uint32_t sector) {
    int res = validate_sector_and_flash_state(sector);
    if (res) return res;

    NRF_NVMC->CONFIG = 2;
    if (/*sector >= NRF_FLASH_SECTOR_CODE ||*/ sector < NRF_FLASH_SECTOR_CODE + NRF_FICR->CODESIZE) {
        NRF_NVMC->ERASEPAGE = (sector - NRF_FLASH_SECTOR_CODE) * NRF_FICR->CODEPAGESIZE;
    } else if (sector == NRF_FLASH_SECTOR_UICR) {
        NRF_NVMC->ERASEUICR = 1;
    }
    while (NRF_NVMC->READY == 0);
    NRF_NVMC->CONFIG = 0;

    return 0;
}

int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length) {
    int res = validate_sector_and_flash_state(sector);
    if (res) {
        return res;
    }
    if ((offset % flash_get_sector_alignment(sector, FLASH_OP_WRITE)) ||
        (length % flash_get_sector_alignment(sector, FLASH_OP_WRITE))) {
        return ERR_FLASH_ALIGN;
    }

    uint32_t sector_size = (uint32_t)flash_get_sector_size(sector);
    if (offset >= sector_size) {
        return 0;
    }

    // allow for cross page writing
    // sector_size -= offset;
    // if (length > sector_size) {
    //     length = sector_size;
    // }

    uint32_t address;
    if (/*sector >= NRF_FLASH_SECTOR_CODE || */ sector < NRF_FLASH_SECTOR_CODE + NRF_FICR->CODESIZE) {
        address = (sector - NRF_FLASH_SECTOR_CODE) * NRF_FICR->CODEPAGESIZE;
    } else {
        address = FLASH_UICR_ADDR_START;
    }

    address += offset;

    res = (int)length;
    NRF_NVMC->CONFIG = 1;
    while (length) {
        uint32_t word =
            (data[0] <<  0) |
            (data[1] <<  8) |
            (data[2] << 16) |
            (data[3] << 24);
        *((uint32_t *)address) = word;
        while (NRF_NVMC->READY == 0);
        address += 4;
        data += 4;
        length -= 4;
    }
    NRF_NVMC->CONFIG = 0;

    return res;
}

int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length) {
    int res = validate_sector_and_flash_state(sector);
    if (res) {
        return res;
    }

    uint32_t sector_size = (uint32_t)flash_get_sector_size(sector);
    if (offset >= sector_size) {
        return 0;
    }

    sector_size -= offset;

    if (length > sector_size) {
        length = sector_size;
    }

    uint8_t *address;
    if (/*sector >= NRF_FLASH_SECTOR_CODE || */ sector < NRF_FLASH_SECTOR_CODE + NRF_FICR->CODESIZE) {
        address = (uint8_t *)((sector - NRF_FLASH_SECTOR_CODE) * NRF_FICR->CODEPAGESIZE);
    } else {
        address = (uint8_t *)FLASH_UICR_ADDR_START;
    }

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
