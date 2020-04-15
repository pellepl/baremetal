/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _NVMTNV_H
#define _NVMTNV_H

#include "types.h"

#ifndef ERR_NVTNV_BASE
#define ERR_NVTNV_BASE      (300)
#endif

#define ERR_NVMTNV_MOUNT        -(ERR_NVTNV_BASE + 0)
#define ERR_NVMTNV_NOFS         -(ERR_NVTNV_BASE + 1)
#define ERR_NVMTNV_FS_ABORTED   -(ERR_NVTNV_BASE + 2)
#define ERR_NVMTNV_FULL         -(ERR_NVTNV_BASE + 3)
#define ERR_NVMTNV_INVAL        -(ERR_NVTNV_BASE + 4)
#define ERR_NVMTNV_NOENT        -(ERR_NVTNV_BASE + 5)
#define ERR_NVMTNV_NONUNIFORM   -(ERR_NVTNV_BASE + 6)

int nvmtnv_mount(uint32_t sector_start);
int nvmtnv_read(uint16_t tag, uint8_t *dst, uint8_t max_size);
int nvmtnv_write(uint16_t tag, const uint8_t *src, uint8_t size);
int nvmtnv_delete(uint16_t tag);
int nvmtnv_size(uint16_t tag);
int nvmtnv_gc(void);
int nvmtnv_fix(void);
int nvmtnv_format(uint32_t sector_start, uint16_t sector_count, uint8_t max_value_size);

#ifndef NVMTNV_DBG
#define NVMTNV_DBG(...)
#endif

#endif // _NVMTNV_H
