/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _FLASH_NRF52_H_
#define _FLASH_NRF52_H_

/* This start sector represents the start address of the flash,
 * and covers address range 0x00000000 -- (flash_page_size-1).
 * Subsequent sectors represents subsequent flash pages.
 */
#define NRF_FLASH_SECTOR_CODE           (0)

/* This single sector represents the whole UICR, 
 * and covers address range 0x10001080 -- 0x10001210.
 */
#define NRF_FLASH_SECTOR_UICR           (0x10000000)

#endif // _FLASH_NRF52_H_
