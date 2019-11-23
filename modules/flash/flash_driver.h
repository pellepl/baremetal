#ifndef _FLASH_DRIVER_H
#define _FLASH_DRIVER_H

#include "types.h"

#ifndef ERR_FLASH_BASE
#define ERR_FLASH_BASE      (200)
#endif

#define ERR_FLASH_NOINIT    -(ERR_FLASH_BASE + 0)
#define ERR_FLASH_BUSY      -(ERR_FLASH_BASE + 1)
#define ERR_FLASH_BADSECTOR -(ERR_FLASH_BASE + 2)
#define ERR_FLASH_PROTECTED -(ERR_FLASH_BASE + 3)
#define ERR_FLASH_ALIGN     -(ERR_FLASH_BASE + 4)
#define ERR_FLASH_NOSUPPORT -(ERR_FLASH_BASE + 5)

typedef enum {
    // typically internal flash where code normally is executed from
    FLASH_TYPE_CODE_BANK0 = 0,
    FLASH_TYPE_CODE_BANK1,
    FLASH_TYPE_CODE_BANK2,
    FLASH_TYPE_CODE_BANK3,
    // typically some otp area
    FLASH_TYPE_OTP0,
    FLASH_TYPE_OTP1,
    FLASH_TYPE_OTP2,
    FLASH_TYPE_OTP3,
    // typically some settings or user area
    FLASH_TYPE_USER0,
    FLASH_TYPE_USER1,
    FLASH_TYPE_USER2,
    FLASH_TYPE_USER3,
    // typically some spi-flash or i2c nvm
    FLASH_TYPE_EXTERNAL0,
    FLASH_TYPE_EXTERNAL1,
    FLASH_TYPE_EXTERNAL2,
    FLASH_TYPE_EXTERNAL3,
    // typically whatever else I didn't foresee
    FLASH_TYPE_WHATEVER0,
    FLASH_TYPE_WHATEVER1,
    FLASH_TYPE_WHATEVER2,
    FLASH_TYPE_WHATEVER3,
    _FLASH_TYPE_CNT
} flash_type_t;

typedef enum {
    FLASH_OP_WRITE = 0,
    FLASH_OP_READ,
} flash_op_t;

int flash_init(void);
/**
 * Sets sector and num_sector to starting sector and number of sectors for given
 * flash type.
 * @return 0 on success, -1 if type is not supported
 */
int flash_get_sectors_for_type(flash_type_t type, uint32_t *sector, uint32_t *num_sectors);
/**
 * Sets sector, offset in sector, and sector length for corresponding given address.
 * @return 0 on success, -1 if no such address or sector exists
 */
int flash_get_sector_for_address(uint32_t address, uint32_t *sector, uint32_t *offset, uint32_t *sector_size);
/** Returns size of given sector */
int flash_get_sector_size(uint32_t sector);
/**
 * Returns least necessary alignment of given sector in bytes, or 0 if no alignment
 * is needed
 */
int flash_get_sector_alignment(uint32_t sector, flash_op_t operation);
/** Protects given sector from erases and writes. */
int flash_protect(uint32_t sector);
/** Allows writing and erasing of given sector. */
int flash_unprotect(uint32_t sector);
/** Checks if given sector allows writing and erasing. */
int flash_is_protected(uint32_t sector);
/** Erase sector. */
int flash_erase(uint32_t sector);
/** Write data to sector. Writes beyond sector boundary are ignored.
 *  Returns number of bytes written. */
int flash_write(uint32_t sector, uint32_t offset, const uint8_t *data, uint32_t length);
/** Read data from sector. Reads beyond sector boundary are ignored.
 *  Returns number of bytes read. */
int flash_read(uint32_t sector, uint32_t offset, uint8_t *data, uint32_t length);

int flash_deinit(void);

#endif // _FLASH_DRIVER_H
