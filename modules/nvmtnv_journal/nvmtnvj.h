/* Copyright (c) 2025 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _NVMTNVJ_H
#define _NVMTNVJ_H

#include "bmtypes.h"

#ifndef ERR_NVTNVJ_BASE
#define ERR_NVTNVJ_BASE (350)
#endif

// mount state error (already mounted or not mounted)
#define ERR_NVMTNVJ_MOUNT -(ERR_NVTNVJ_BASE + 0)
// not a filesystem, structure broken
#define ERR_NVMTNVJ_NOFS -(ERR_NVTNVJ_BASE + 1)
// filesystem operation aborted, needs fix
#define ERR_NVMTNVJ_FS_ABORTED -(ERR_NVTNVJ_BASE + 2)
// file system full
#define ERR_NVMTNVJ_FULL -(ERR_NVTNVJ_BASE + 3)
// bad input
#define ERR_NVMTNVJ_INVAL -(ERR_NVTNVJ_BASE + 4)
// no entry
#define ERR_NVMTNVJ_NOENT -(ERR_NVTNVJ_BASE + 5)
// sectors not in uniform size
#define ERR_NVMTNVJ_NONUNIFORM -(ERR_NVTNVJ_BASE + 6)
// sectors not in uniform size
#define ERR_NVMTNVJ_FATAL -(ERR_NVTNVJ_BASE + 7)

void nvmtnvj_init(void);
int nvmtnvj_mount(uint32_t sector_start, uint8_t max_lookahead_sectors);
int nvmtnvj_unmount(void);
int nvmtnvj_read(uint16_t tag, uint8_t *dst);
int nvmtnvj_write(uint16_t tag, const uint8_t *src, uint8_t size);
int nvmtnvj_delete(uint16_t tag);
int nvmtnvj_size(uint16_t tag);
int nvmtnvj_gc(void);
int nvmtnvj_fix(void);
int nvmtnvj_format(uint32_t sector_start, uint8_t sectors_per_block, uint8_t block_count, uint8_t max_value_size);

#if NVMTNVJ_TEST
// expose some privates to ease unittests
int nvmtnvj_test_tags_per_block(void);
int nvmtnvj_test_dump(void);
#endif

#ifndef NVMTNVJ_DBG
#define NVMTNVJ_DBG(...)
#endif

#endif // _NVMTNVJ_H
