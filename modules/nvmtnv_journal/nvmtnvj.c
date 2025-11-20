/* Copyright (c) 2025 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */
#include <stdbool.h>
#include <stddef.h>
#include "nvmtnvj.h"
#include "flash_driver.h"
#ifndef CONFIG_NVMTNVJ_MEMCPY
#include <string.h>
#define _memcpy(d, s, n) memcpy((d), (s), (n))
#else
#define _memcpy(d, s, n) CONFIG_NVMTNVJ_MEMCPY((d), (s), (n))
#endif

/**
 * Journalling tag and value module. Only ever writes to erased words in flash.
 * Assumes flash in equal sized sectors. Handles different flash technologies,
 * where erased flash is either zeroes or ones.
 *
 * Organises sectors/pages in blocks, where a block comprises one or more
 * sectors/pages. Each valid block starts with meta info, the block header.
 *
 * Always need at least two blocks - one data block and one spare block.
 * Once all data blocks are written, the best block candidate is evicted to the
 * spare block. The evicted block becomes the new spare block, and old spare
 * block becomes the latest data block.
 *
 * Tags in a block is always written in sequence from start to end.
 *
 * Rewritten and deleted tags are never modified - the last entry wins.
 * When finding tags, start looking at last written tag entry in latest data block.
 * Go backwards towards start of block. At top, proceed to last written tag entry
 * in previous block. Continue until all blocks are exhausted
 *
 * Deleting a tag implies adding a new tag entry, marking it as deleted. This
 * implies that in case of full system, we must always save at least one tag
 * slot in order to be able to delete a tag to make room.
 *
 * Keep track of current non-full block. Block history traversal by sequence
 * numbers.
 *
 * NB: Scales horribly! A GC can take (tags_per_block * number_of_blocks) ^ 2 reads.
 *
 * Block types:
 *   Spare: singleton, used to fill up with live data when GC
 *   Data: multiple
 *   Data_Free: multiple
 *   Evicting: singleton, the block we're currently are evicting to spare during GC
 *   Unknown: a broken Spare if aborted during GC, will be mended accordingly
 *
 * Block state transitions:
 *   Free -> Data
 *   Spare -> Data
 *   Data -> Evicting
 *   Evicting -> Spare
 *
 * At GC:
 *   o Find best Data block to evict
 *   o Mark best Data block as Evicting block
 *   o Copy live tags from Evicting block to Spare block
 *   o Mark Spare block as Data block
 *   o Erase Evicting block
 *   o Mark erased Evicting block as Spare
 *
 * At init, check blocks for aborted GC:
 *   o First block erased - special case: step forward x sectors and see if a valid block
 *     magic is found. If not, corrupt fs. Else, fetch file system info and scan.
 *   o Data blocks and one Spare block: FS OK
 *   o Evicting block and Spare block: erase Spare block, remake as Spare, and reevict
 *   o Evicting block and Unknown block: erase Unknown block, make as Spare, and reevict
 *   o Evicting block and only Data blocks: erase Evicting block and make as Spare
 *   o Unknown block and only Data blocks: erase Unknown block and make as Spare
 *   o All other cases: corrupt FS
 *
 * Block header:
 *   Magic + FS metainfo
 *   Flags: data evict
 *   sequence number
 *
 * X = unwritten
 * Spare        !data !evict seq:X
 * Data_Free     data !evict seq:X
 * Data          data !evict seq:#
 * Evicting      data  evict seq:#
 *
 * Keep track of current data block, B_current.
 *
 * On tag_write:
 *   if Data block B_current is full:
 *     find Data_Free block B_free
 *     if B_free exists:
 *       B_free.seq = max_seq+1 [B_free -> Data]
 *       B_current = B_free
 *     else:
 *       find block candidate to evict among Data, B_evict
 *       if B_evict not exists:
 *         error(FS full)
 *       B_evict.evict = true [B_evict -> Evicting]
 *       move live tags to B_spare
 *       B_spare.seq = max_seq+1 [B_spare -> Unknown]
 *       B_spare.data = true [B_spare -> Data block]
 *       erase B_evict [B_evict -> Spare block]
 *       B_spare_old = B_spare
 *       B_spare = B_evict
 *       B_current = B_spare_old
 *   append tag to Data block B_current
 *
 * Evict candidate algo:
 * Traverse each block for tags, from recent to old:
 * 1. 	If a tag definition is found, all older tag definitions for same id	in same
 * 	    block can be freed.
 * 	    [Prevents multiple interblock searches of same tag ID]
 * 2. 	If a tag definition is found, and it is redefined in a more recent block,
 * 	    the tag definition can be freed.
 * 3. 	If a tag delete definition is found, and it is not write defined in any older
 * 	    block, the tag delete definition can be freed.
 * Score = nbr of freeable tags + block age
 */

#ifndef NVMTNVJ_DBG
#include <stdio.h>
#define NVMTNVJ_DBG(...) printf(__VA_ARGS__)
#endif

#define _dbg(...) NVMTNVJ_DBG(__VA_ARGS__)

#define MAGIC 0xba

#ifndef CONFIG_NVMTNVJ_FLASH_WORD_SIZE
// minimal writable flash unit in bytes
#define CONFIG_NVMTNVJ_FLASH_WORD_SIZE 4
#endif
#ifndef CONFIG_NVMTNVJ_FLASH_WORD_ERASED
// erased flash unit value (typically 0xffffffff for NOR flashes, 0x00000000 for SONOS/EEPROM flashes)
#define CONFIG_NVMTNVJ_FLASH_WORD_ERASED 0x00000000
#endif
#if CONFIG_NVMTNVJ_FLASH_WORD_SIZE == 1
typedef uint8_t word_t;
#elif CONFIG_NVMTNVJ_FLASH_WORD_SIZE == 2
typedef uint16_t word_t;
#elif CONFIG_NVMTNVJ_FLASH_WORD_SIZE >= 4
typedef uint32_t word_t;
#else
_Static_assert(0, "CONFIG_NVMTNVJ_FLASH_WORD_SIZE invalid configured");
#endif

#define BLOCK_HEADER_FLAG_CLR CONFIG_NVMTNVJ_FLASH_WORD_ERASED
#define BLOCK_HEADER_FLAG_SET ((word_t)0x5a5a5a5a ^ BLOCK_HEADER_FLAG_CLR)

#define ADDR_ALIGNW __attribute__((aligned(CONFIG_NVMTNVJ_FLASH_WORD_SIZE)))
#define ALIGNW(x) (((x) + CONFIG_NVMTNVJ_FLASH_WORD_SIZE - 1) / \
                   CONFIG_NVMTNVJ_FLASH_WORD_SIZE) *            \
                      CONFIG_NVMTNVJ_FLASH_WORD_SIZE

#define SEQ_NBR_UNWRITTEN (word_t)(CONFIG_NVMTNVJ_FLASH_WORD_ERASED)

#define SEQ_NBR_HALF_RANGE (1ull << (8 * CONFIG_NVMTNVJ_FLASH_WORD_SIZE - 1))

#define UNDEF_IX (uint32_t)-1

#define ERR_INTERNAL_ABORTED 0x201

#define ERR_RET(x)         \
    do                     \
    {                      \
        int _x = (int)(x); \
        if (_x < 0)        \
            return _x;     \
    } while (0)

typedef struct
{
    uint8_t magic;
    uint8_t sectors_per_block;
    uint8_t nbr_of_blocks;
    uint8_t max_value_size;
} fs_descr_t;

typedef struct
{
    ADDR_ALIGNW fs_descr_t descr;
    ADDR_ALIGNW word_t seq_nbr;
    ADDR_ALIGNW word_t data_flag;
    ADDR_ALIGNW word_t evict_flag;
} block_header_t;

enum __attribute__((packed)) tag_state_t
{
    TAG_FREE = (uint8_t)CONFIG_NVMTNVJ_FLASH_WORD_ERASED,
    TAG_WRITTEN = 1,
    TAG_DELETED = 2,
};

typedef uint8_t tag_state_t;

typedef ADDR_ALIGNW struct
{
    tag_state_t state;
    uint8_t len;
    uint16_t id;
    uint8_t chk; // last byte, after this comes the data
} __attribute__((packed)) tag_header_t;

_Static_assert(sizeof(tag_header_t) == 5, "unexpected tag_header_t size");

typedef enum
{
    BLOCK_TYPE_UNKNOWN,
    BLOCK_TYPE_DATA_FREE,
    BLOCK_TYPE_DATA,
    BLOCK_TYPE_SPARE,
    BLOCK_TYPE_EVICTING,
} block_type_t;

typedef struct
{
    uint32_t *blocks;
    word_t *seq_nbrs;
    uint32_t count;
} sorted_blocks_t;

typedef struct
{
    uint16_t id;
    enum
    {
        TI_FREE,
        TI_WRITTEN,
        TI_DELETED,
        TI_FREEABLE,
        TI_LIVE_WRITTEN,
        TI_LIVE_DELETED,
    } status;
} tag_evict_info_t;

#define tag_evict_status_to_str(x) (const char *[]){"FREE", "WRIT", "DELE", "FREEABLE", "LIVE_WRIT", "LIVE_DELE"}[x]

static struct
{
    enum
    {
        STATE_UNMOUNTED,
        STATE_MOUNTED,
        STATE_MOUNTED_INCONSISTENT
    } state;
    uint32_t starting_sector;
    uint32_t sector_size;
    uint32_t current_block_ix;
    uint32_t current_tag_ix;
    uint32_t spare_block_ix;
    uint32_t unknown_block_ix;
    uint32_t evict_block_ix;
    uint32_t tags_per_block;
    word_t min_seq_nbr;
    word_t max_seq_nbr;
    uint8_t sectors_per_block;
    uint8_t nbr_of_blocks;
    uint8_t max_value_size;
} sys;

static bool is_flag(word_t w)
{
    return w == BLOCK_HEADER_FLAG_SET || w == BLOCK_HEADER_FLAG_CLR;
}

static bool flag(word_t w)
{
    return w == BLOCK_HEADER_FLAG_SET;
}

static bool block_is_valid(const block_header_t *b)
{
    if (b->descr.magic != MAGIC)
        return false;
    if (b->descr.max_value_size == 0 || b->descr.max_value_size != sys.max_value_size)
        return false;
    if (b->descr.nbr_of_blocks == 0 || b->descr.nbr_of_blocks != sys.nbr_of_blocks)
        return false;
    if (b->descr.sectors_per_block == 0 || b->descr.sectors_per_block != sys.sectors_per_block)
        return false;
    if (!is_flag(b->data_flag))
        return false;
    if (!is_flag(b->evict_flag))
        return false;
    return true;
}

static block_type_t block_type(const block_header_t *bhdr)
{
    bool seq_nbr_written = bhdr->seq_nbr != SEQ_NBR_UNWRITTEN;
    if (!block_is_valid(bhdr))
        return BLOCK_TYPE_UNKNOWN;
    if (!seq_nbr_written && !flag(bhdr->data_flag) && !flag(bhdr->evict_flag))
        return BLOCK_TYPE_SPARE;
    if (!seq_nbr_written && flag(bhdr->data_flag) && !flag(bhdr->evict_flag))
        return BLOCK_TYPE_DATA_FREE;
    if (seq_nbr_written && flag(bhdr->data_flag) && !flag(bhdr->evict_flag))
        return BLOCK_TYPE_DATA;
    if (seq_nbr_written && flag(bhdr->data_flag) && flag(bhdr->evict_flag))
        return BLOCK_TYPE_EVICTING;
    return BLOCK_TYPE_UNKNOWN;
}

static uint32_t tag_size(void)
{
    return ALIGNW(sizeof(tag_header_t) + sys.max_value_size);
}

static word_t seq_nbr_new(void)
{
    word_t nxt = sys.max_seq_nbr + 1;
    if (nxt == SEQ_NBR_UNWRITTEN)
        nxt++;
    return nxt;
}

static bool seq_nbr_is_newer(word_t x, word_t y)
{
    // equality treated as newer
    return (word_t)(x - y) < SEQ_NBR_HALF_RANGE;
}

static word_t seq_nbr_diff(word_t newer, word_t older)
{
    return (word_t)(newer - older);
}

// reads across sector boundaries which flash_* api does not
static int block_read(uint32_t block_ix, uint32_t offset, uint8_t *dst, uint32_t size)
{
    if (block_ix >= sys.nbr_of_blocks)
        return ERR_NVMTNVJ_INVAL;
    uint32_t sector = sys.starting_sector + sys.sectors_per_block * block_ix;
    sector += offset / sys.sector_size;
    while (size > 0)
    {
        uint32_t remaining_in_sector = sys.sector_size - (offset % sys.sector_size);
        uint32_t to_read_in_sector = size > remaining_in_sector ? remaining_in_sector : size;
        int res = flash_read(sector, offset % sys.sector_size, dst, to_read_in_sector);
        ERR_RET(res);
        dst += to_read_in_sector;
        offset += to_read_in_sector;
        size -= to_read_in_sector;
        sector++;
    }
    return 0;
}

static int block_write_word(uint32_t block_ix, uint32_t offset, word_t w)
{
    uint32_t sector = sys.starting_sector + sys.sectors_per_block * block_ix;
    sector += offset / sys.sector_size;
    int res = flash_write(sector, offset % sys.sector_size, (const uint8_t *)&w,
                          CONFIG_NVMTNVJ_FLASH_WORD_SIZE);
    return res < 0 ? res : 0;
}

static int block_write_seq_nbr(uint32_t block_ix, word_t seq_nbr)
{
    return block_write_word(block_ix, offsetof(block_header_t, seq_nbr), seq_nbr);
}

static int block_write_evicting_flag(uint32_t block_ix)
{
    return block_write_word(block_ix, offsetof(block_header_t, evict_flag), BLOCK_HEADER_FLAG_SET);
}

static int block_write_data_flag(uint32_t block_ix)
{
    return block_write_word(block_ix, offsetof(block_header_t, data_flag), BLOCK_HEADER_FLAG_SET);
}

static int block_write_data_concat(uint32_t block_ix, uint32_t offset, const uint8_t *data1, uint8_t len1,
                                   const uint8_t *data2, uint8_t len2)
{
    int res;
    word_t w = CONFIG_NVMTNVJ_FLASH_WORD_ERASED;
    while (len1 >= CONFIG_NVMTNVJ_FLASH_WORD_SIZE)
    {
        _memcpy(&w, data1, CONFIG_NVMTNVJ_FLASH_WORD_SIZE);
        res = block_write_word(block_ix, offset, w);
        ERR_RET(res);
        data1 += CONFIG_NVMTNVJ_FLASH_WORD_SIZE;
        len1 -= CONFIG_NVMTNVJ_FLASH_WORD_SIZE;
        offset += CONFIG_NVMTNVJ_FLASH_WORD_SIZE;
    }
    if (len1 > 0)
    {
        w = CONFIG_NVMTNVJ_FLASH_WORD_ERASED;
        uint8_t *pw = (uint8_t *)&w;
        _memcpy(pw, data1, len1);
        pw += len1;

        uint8_t rem_in_w = (uint8_t)(CONFIG_NVMTNVJ_FLASH_WORD_SIZE - len1);
        uint8_t take = len2 > rem_in_w ? rem_in_w : len2;
        if (take)
        {
            _memcpy(pw, data2, take);
            data2 += take;
            len2 -= take;
        }
        res = block_write_word(block_ix, offset, w);
        ERR_RET(res);
        offset += CONFIG_NVMTNVJ_FLASH_WORD_SIZE;
    }
    while (len2 >= CONFIG_NVMTNVJ_FLASH_WORD_SIZE)
    {
        _memcpy(&w, data2, CONFIG_NVMTNVJ_FLASH_WORD_SIZE);
        res = block_write_word(block_ix, offset, w);
        ERR_RET(res);
        data2 += CONFIG_NVMTNVJ_FLASH_WORD_SIZE;
        len2 -= CONFIG_NVMTNVJ_FLASH_WORD_SIZE;
        offset += CONFIG_NVMTNVJ_FLASH_WORD_SIZE;
    }
    if (len2 > 0)
    {
        w = CONFIG_NVMTNVJ_FLASH_WORD_ERASED;
        _memcpy(&w, data2, len2);
        res = block_write_word(block_ix, offset, w);
        ERR_RET(res);
    }
    return 0;
}

static int block_erase(uint32_t block_ix, block_type_t type)
{
    _dbg("erase block %d, type %d\n", block_ix, type);
    const uint32_t block_sector = sys.starting_sector + block_ix * sys.sectors_per_block;
    int res = 0;
    for (uint32_t s = block_sector; s < block_sector + sys.sectors_per_block; s++)
    {
        res = flash_erase(s);
        ERR_RET(res);
    }
    block_header_t bhdr = {
        .descr.magic = MAGIC,
        .descr.max_value_size = sys.max_value_size,
        .descr.nbr_of_blocks = sys.nbr_of_blocks,
        .descr.sectors_per_block = sys.sectors_per_block,
        .data_flag = type == BLOCK_TYPE_SPARE ? BLOCK_HEADER_FLAG_CLR : BLOCK_HEADER_FLAG_SET,
        .evict_flag = BLOCK_HEADER_FLAG_CLR,
        .seq_nbr = SEQ_NBR_UNWRITTEN,
    };
    res = flash_write(block_sector, 0, (const uint8_t *)&bhdr, sizeof(bhdr));
    if (res > 0)
        res = 0;
    ERR_RET(res);
    return res;
}

static int block_read_hdr(uint32_t block_ix, block_header_t *h)
{
    if (block_ix >= sys.nbr_of_blocks)
        return ERR_NVMTNVJ_INVAL;
    return flash_read(sys.starting_sector + sys.sectors_per_block * block_ix, 0, (uint8_t *)h,
                      sizeof(block_header_t));
}

static int block_find_prev(word_t seq_nbr, uint32_t *prev_block_ix, word_t *prev_seq_nbr)
{
    block_header_t bhdr;
    int res;
    word_t min_diff = (word_t)-1;
    word_t seq_nbr_cand = SEQ_NBR_UNWRITTEN;
    uint32_t block_ix = UNDEF_IX;
    for (uint8_t b = 0; b < sys.nbr_of_blocks; b++)
    {
        res = block_read_hdr(b, &bhdr);
        ERR_RET(res);
        block_type_t btype = block_type(&bhdr);
        if (btype != BLOCK_TYPE_DATA && btype != BLOCK_TYPE_EVICTING)
            continue;
        if (seq_nbr_is_newer(bhdr.seq_nbr, seq_nbr))
            continue;
        word_t diff = seq_nbr_diff(seq_nbr, bhdr.seq_nbr);
        if ((uint32_t)diff < min_diff)
        {
            min_diff = diff;
            block_ix = b;
            seq_nbr_cand = bhdr.seq_nbr;
        }
    }
    if (block_ix == UNDEF_IX)
        return ERR_NVMTNVJ_NOENT;
    *prev_block_ix = block_ix;
    if (prev_seq_nbr)
        *prev_seq_nbr = seq_nbr_cand;
    return 0;
}

static int block_alloc(uint32_t *block_ix, word_t *seq_nbr)
{
    block_header_t bhdr;
    for (uint8_t b = 0; b < sys.nbr_of_blocks; b++)
    {
        int res = block_read_hdr(b, &bhdr);
        ERR_RET(res);
        if (block_type(&bhdr) != BLOCK_TYPE_DATA_FREE)
            continue;
        *seq_nbr = seq_nbr_new();
        _dbg("alloc bix %d seq %d\n", b, *seq_nbr);
        res = block_write_seq_nbr(b, *seq_nbr);
        ERR_RET(res);
        *block_ix = b;
        return 0;
    }
    return ERR_NVMTNVJ_NOENT;
}

static uint8_t chk(const uint8_t *data, uint8_t len)
{
    uint8_t s = 0xaa;
    for (uint8_t i = 0; i < len; i++)
        s += data[i];
    return (uint8_t)(0 - s);
}

static int tag_read_hdr_in_block(uint32_t block_ix, uint32_t tag_ix, tag_header_t *t)
{
    if (tag_ix >= sys.tags_per_block)
        return ERR_NVMTNVJ_INVAL;
    return block_read(block_ix, ALIGNW(sizeof(block_header_t)) + tag_ix * tag_size(), (uint8_t *)t,
                      sizeof(tag_header_t));
}

static int tag_write(uint32_t block_ix, uint32_t tag_ix, uint16_t tag_id, tag_state_t state, const uint8_t *data,
                     uint8_t len)
{
    if (len > sys.max_value_size)
        len = sys.max_value_size;
    tag_header_t thdr = {
        .id = tag_id,
        .len = len,
        .state = state,
        .chk = chk(data, len)};
    return block_write_data_concat(block_ix, ALIGNW(sizeof(block_header_t)) + tag_ix * tag_size(),
                                   (const uint8_t *)&thdr, sizeof(tag_header_t), data, len);
}

static int tag_find_next_free_in_block(uint32_t block_ix, uint32_t *tag_ix)
{
    for (uint32_t tix = 0; tix < sys.tags_per_block; tix++)
    {
        tag_header_t thdr;
        int res = tag_read_hdr_in_block(block_ix, tix, &thdr);
        ERR_RET(res);
        if (thdr.state == TAG_FREE)
        {
            *tag_ix = tix;
            return 0;
        }
    }
    return ERR_NVMTNVJ_NOENT;
}

static int tag_read(const tag_header_t *thdr, uint32_t block_ix, uint32_t tag_ix, uint8_t *dst)
{
    uint8_t len = thdr->len > sys.max_value_size ? sys.max_value_size : thdr->len;
    int res = block_read(block_ix,
                         ALIGNW(sizeof(block_header_t)) + tag_ix * tag_size() + sizeof(tag_header_t), dst, len);
    ERR_RET(res);
    if (chk(dst, len) != thdr->chk)
    {
        _dbg("aborted tag write, id %04x, len %d, chk %02x != %02x\n", thdr->id, len, chk(dst, len), thdr->chk);
        return ERR_INTERNAL_ABORTED;
    }
    return len;
}

static int tag_find_and_read(uint16_t tag_id, uint8_t *dst)
{
    int res;
    uint8_t blocks_left = sys.nbr_of_blocks - 1;
    block_header_t bhdr;
    uint32_t cur_block_ix = sys.current_block_ix;
    while (blocks_left > 0) // safe-guard
    {
        res = block_read_hdr(cur_block_ix, &bhdr);
        ERR_RET(res);
        for (uint32_t t = 0; t < sys.tags_per_block; t++)
        {
            tag_header_t thdr;
            uint32_t tag_ix = sys.tags_per_block - 1 - t;
            res = tag_read_hdr_in_block(cur_block_ix, tag_ix, &thdr);
            ERR_RET(res);
            if (thdr.state == TAG_FREE)
                continue;
            if (thdr.id != tag_id)
                continue;
            if (thdr.state == TAG_DELETED)
                return ERR_NVMTNVJ_NOENT;
            res = tag_read(&thdr, cur_block_ix, tag_ix, dst);
            ERR_RET(res);
            if (res == ERR_INTERNAL_ABORTED)
                continue;
            return res;
        }

        uint32_t prev_block_ix;
        res = block_find_prev(bhdr.seq_nbr, &prev_block_ix, NULL);
        ERR_RET(res);
        cur_block_ix = prev_block_ix;
        blocks_left--;
    }
    return ERR_NVMTNVJ_NOENT;
}

// Sorts blocks in historical order, newest first. O(n²) n=nbr_of_blocks
// Only Data and Evicting (when present, during fixing) blocks are included.
static int blocks_sort(sorted_blocks_t *sorted_blocks)
{
    for (uint32_t i = 0; i < sys.nbr_of_blocks; i++)
        sorted_blocks->blocks[i] = UNDEF_IX;
    // pick newest seed among current DATA and EVICTING (if any)
    uint32_t seed_block = sys.current_block_ix;
    word_t seed_seq = sys.max_seq_nbr;
    if (sys.evict_block_ix != UNDEF_IX)
    {
        block_header_t bhdr;
        int r = block_read_hdr(sys.evict_block_ix, &bhdr);
        ERR_RET(r);
        if (block_type(&bhdr) == BLOCK_TYPE_EVICTING &&
            seq_nbr_is_newer(bhdr.seq_nbr, seed_seq))
        {
            seed_block = sys.evict_block_ix;
            seed_seq = bhdr.seq_nbr;
        }
    }
    sorted_blocks->blocks[0] = seed_block;
    sorted_blocks->seq_nbrs[0] = seed_seq;
    sorted_blocks->count = 1;
    word_t cur_seq_nbr = seed_seq;
    while (sorted_blocks->count < sys.nbr_of_blocks)
    {
        word_t prev_seq_nbr;
        int res = block_find_prev(cur_seq_nbr,
                                  &sorted_blocks->blocks[sorted_blocks->count], &prev_seq_nbr);
        if (res == ERR_NVMTNVJ_NOENT)
            break;
        ERR_RET(res);
        cur_seq_nbr = prev_seq_nbr;
        sorted_blocks->seq_nbrs[sorted_blocks->count] = cur_seq_nbr;
        sorted_blocks->count++;
    }
    return 0;
}

// figures out live tags within the same block
static int block_map_live_tags(uint32_t sorted_block_ix, const sorted_blocks_t *sorted_blocks,
                               tag_evict_info_t *tag_info)
{
    int res = 0;
    uint32_t cur_block_ix = sorted_blocks->blocks[sorted_block_ix];
    bool rest_is_free = false;
    tag_header_t thdr;

    // read out id:s and states
    for (uint32_t tag_ix = 0; tag_ix < sys.tags_per_block; tag_ix++)
    {
        if (rest_is_free)
        {
            tag_info[tag_ix].status = TI_FREE;
            continue;
        }
        res = tag_read_hdr_in_block(cur_block_ix, tag_ix, &thdr);
        ERR_RET(res);
        tag_info[tag_ix].id = thdr.id;
        switch (thdr.state)
        {
        case TAG_FREE:
            tag_info[tag_ix].status = TI_FREE;
            rest_is_free = true; // rest of headers are free by design
            break;
        case TAG_DELETED:
            tag_info[tag_ix].status = TI_DELETED;
            break;
        case TAG_WRITTEN:
            tag_info[tag_ix].status = TI_WRITTEN;
            break;
        default:
            return ERR_NVMTNVJ_FATAL;
        }
    }

    // mark old duplicate ids in same block as freeable, if deleted or truly written
    uint8_t tmp_buf[sys.max_value_size];
    for (uint32_t t = 0; t < sys.tags_per_block; t++)
    {
        uint32_t tag_ix = sys.tags_per_block - 1 - t;
        if (tag_info[tag_ix].status == TI_WRITTEN)
        {
            // if aborted written tag, mark FREEABLE directly
            res = tag_read_hdr_in_block(cur_block_ix, tag_ix, &thdr);
            ERR_RET(res);
            res = tag_read(&thdr, cur_block_ix, tag_ix, tmp_buf);
            ERR_RET(res);
            if (res == ERR_INTERNAL_ABORTED)
                tag_info[tag_ix].status = TI_FREEABLE;
            else
                tag_info[tag_ix].status = TI_LIVE_WRITTEN;
        }
        if (tag_info[tag_ix].status == TI_FREEABLE || tag_info[tag_ix].status == TI_FREE)
            continue;
        if (tag_info[tag_ix].status == TI_DELETED)
            tag_info[tag_ix].status = TI_LIVE_DELETED;
        // tag is defined in this block, mark all previous with same id as freeable
        for (uint32_t t2 = 0; t2 < tag_ix; t2++)
        {
            if (tag_info[t2].id == tag_info[tag_ix].id)
                tag_info[t2].status = TI_FREEABLE;
        }
    }
    return res;
}

// returns 1 if a tag is defined later in another block, 0 if not, and negative if error
static int block_is_tag_defined_later(uint32_t current_sorted_block_ix, const sorted_blocks_t *sorted_blocks,
                                      uint16_t tag_id)
{
    int res;
    if (current_sorted_block_ix == 0)
        return 0; // there is no later than current

    for (uint32_t sorted_block_ix = 0; sorted_block_ix < current_sorted_block_ix; sorted_block_ix++)
    {
        uint32_t block_ix = sorted_blocks->blocks[sorted_block_ix];
        uint8_t tmp_buf[sys.max_value_size];
        for (uint32_t t = 0; t < sys.tags_per_block; t++)
        {
            uint32_t tag_ix = sys.tags_per_block - 1 - t;
            tag_header_t thdr;
            res = tag_read_hdr_in_block(block_ix, tag_ix, &thdr);
            ERR_RET(res);
            if (thdr.state == TAG_DELETED && thdr.id == tag_id)
                return 1;
            if (thdr.state == TAG_WRITTEN && thdr.id == tag_id)
            {
                // check tag is not aborted, ignore if so
                res = tag_read(&thdr, block_ix, tag_ix, tmp_buf);
                ERR_RET(res);
                if (res != ERR_INTERNAL_ABORTED)
                    return 1;
            }
        } // per tag
    } // per block
    return 0;
}

// returns 1 if a tag is written earlier in another block, 0 if not, and negative if error
static int block_is_tag_written_earlier(uint32_t current_sorted_block_ix, const sorted_blocks_t *sorted_blocks,
                                        uint16_t tag_id)
{
    int res;
    if (current_sorted_block_ix >= sorted_blocks->count - 1)
        return 0; // there is no earlier than last

    for (uint32_t sorted_block_ix = current_sorted_block_ix + 1; sorted_block_ix < sorted_blocks->count; sorted_block_ix++)
    {
        uint32_t block_ix = sorted_blocks->blocks[sorted_block_ix];
        uint8_t tmp_buf[sys.max_value_size];
        for (uint32_t t = 0; t < sys.tags_per_block; t++)
        {
            uint32_t tag_ix = sys.tags_per_block - 1 - t;
            tag_header_t thdr;
            res = tag_read_hdr_in_block(block_ix, tag_ix, &thdr);
            ERR_RET(res);
            if (thdr.state == TAG_WRITTEN && thdr.id == tag_id)
            {
                // check tag is not aborted, ignore if so
                res = tag_read(&thdr, block_ix, tag_ix, tmp_buf);
                ERR_RET(res);
                if (res != ERR_INTERNAL_ABORTED)
                    return 1;
            }
        } // per tag
    } // per block
    return 0;
}

static int block_find_freeables(uint32_t sorted_block_ix, const sorted_blocks_t *sorted_blocks,
                                tag_evict_info_t *tag_info)
{
    // find live tags internally in same block
    int res = block_map_live_tags(sorted_block_ix, sorted_blocks, tag_info);
    ERR_RET(res);
    // find live tags when looking externally at other blocks
    for (uint32_t t = 0; t < sys.tags_per_block; t++)
    {
        if (tag_info[t].status == TI_LIVE_DELETED || tag_info[t].status == TI_LIVE_WRITTEN)
        {
            // if defined later, this tag info is old - freeable
            res = block_is_tag_defined_later(sorted_block_ix, sorted_blocks, tag_info[t].id);
            ERR_RET(res);
            if (res > 0)
                tag_info[t].status = TI_FREEABLE;
        }
        if (tag_info[t].status == TI_LIVE_DELETED)
        {
            // we can only free a live deleted tag in this block iff it is not defined as
            // written in any earlier block - lest it would reappear upon freeing this block
            res = block_is_tag_written_earlier(sorted_block_ix, sorted_blocks, tag_info[t].id);
            ERR_RET(res);
            if (res == 0)
                tag_info[t].status = TI_FREEABLE;
        }
        _dbg("  tag %04x: %s\n", tag_info[t].id, tag_evict_status_to_str(tag_info[t].status));
    }
    return res;
}

static int block_find_evict_candidate(uint32_t *cand_block_ix, tag_evict_info_t *cand_tag_info)
{
    int res;
    *cand_block_ix = UNDEF_IX;
    uint32_t cand_score = 0;
    const word_t age_diff_max = seq_nbr_diff(sys.max_seq_nbr, sys.min_seq_nbr);
    uint32_t sorted_block_list[sys.nbr_of_blocks];
    word_t sorted_seq_nbr_list[sys.nbr_of_blocks];
    sorted_blocks_t sorted_blocks = {
        .blocks = sorted_block_list,
        .seq_nbrs = sorted_seq_nbr_list,
        .count = 0};
    res = blocks_sort(&sorted_blocks);
    ERR_RET(res);

    // for each block, check all tags within, count live and dead tags
    // O(n²) n=tags_per_block*nbr_of_blocks
    for (uint32_t i = 0; i < sorted_blocks.count; i++)
    {
        _dbg("block %d\n", sorted_blocks.blocks[i]);
        tag_evict_info_t tag_info[sys.tags_per_block];
        res = block_find_freeables(i, &sorted_blocks, tag_info);
        ERR_RET(res);
        // calculate normalized candidate score
        uint32_t age_score = 0;
        if (age_diff_max > 0)
        {
            word_t age_diff = seq_nbr_diff(sys.max_seq_nbr, sorted_blocks.seq_nbrs[i]);
            if (age_diff > (uint32_t)(1 << 16)) // avoid overflow on age_score
                age_score = 0x100;
            else
                age_score = 0x100 * age_diff / age_diff_max;
        }
        uint32_t freeables = 0;
        for (uint32_t t = 0; t < sys.tags_per_block; t++)
        {
            if (tag_info[t].status == TI_FREEABLE)
                freeables++;
        }
        uint32_t freeable_score = 0x100 * freeables / sys.tags_per_block;
        uint32_t score = freeables == 0 ? 0 : (freeable_score + age_score);
        _dbg("score [age:%d free:%d]: %d\n", age_score, freeable_score, score);
        if (score > cand_score)
        {
            cand_score = score;
            *cand_block_ix = sorted_blocks.blocks[i];
            _memcpy(cand_tag_info, tag_info, sizeof(tag_info));
        }
    }
    return 0;
}

static int block_evict(uint32_t block_ix_src, const tag_evict_info_t *evict_tag_info, uint32_t block_ix_dst)
{
    block_header_t bhdr_src;
    _dbg("evicting block %d to %d\n", block_ix_src, block_ix_dst);
    int res = block_read_hdr(block_ix_src, &bhdr_src);
    ERR_RET(res);
    // mark as evicting
    if (!flag(bhdr_src.evict_flag))
    {
        res = block_write_evicting_flag(block_ix_src);
        ERR_RET(res);
    }
    // copy live tags from src to dest
    uint32_t tag_ix_dst = 0;
    for (uint32_t tag_ix_src = 0; tag_ix_src < sys.tags_per_block; tag_ix_src++)
    {
        if (evict_tag_info[tag_ix_src].status == TI_LIVE_DELETED)
        {
            _dbg("evicting tag %d as DELETED\n", evict_tag_info[tag_ix_src].id);
            res = tag_write(block_ix_dst, tag_ix_dst, evict_tag_info[tag_ix_src].id, TAG_DELETED, NULL, 0);
            ERR_RET(res);
        }
        else if (evict_tag_info[tag_ix_src].status == TI_LIVE_WRITTEN)
        {
            tag_header_t thdr;
            uint8_t data[sys.max_value_size];
            res = tag_read_hdr_in_block(block_ix_src, tag_ix_src, &thdr);
            ERR_RET(res);
            uint8_t len = thdr.len > sys.max_value_size ? sys.max_value_size : thdr.len;
            _dbg("evicting tag %d as WRITTEN len %d\n", evict_tag_info[tag_ix_src].id, len);
            res = block_read(block_ix_src,
                             ALIGNW(sizeof(block_header_t)) + tag_ix_src * tag_size() + sizeof(tag_header_t),
                             data, len);
            ERR_RET(res);
            res = tag_write(block_ix_dst, tag_ix_dst, evict_tag_info[tag_ix_src].id, TAG_WRITTEN, data, len);
            ERR_RET(res);
        }
        else
        {
            continue;
        }
        tag_ix_dst++;
    }
    _dbg("evicted %d tags\n", tag_ix_dst);

    // mark destination as data block, transform source to spare block
    word_t new_seq_nbr = seq_nbr_new();
    res = block_write_seq_nbr(block_ix_dst, new_seq_nbr);
    ERR_RET(res);
    res = block_write_data_flag(block_ix_dst);
    ERR_RET(res);
    res = block_erase(block_ix_src, BLOCK_TYPE_SPARE);
    ERR_RET(res);

    // update state
    sys.current_block_ix = block_ix_dst;
    sys.current_tag_ix = tag_ix_dst;
    sys.spare_block_ix = block_ix_src;
    sys.max_seq_nbr = new_seq_nbr;
    if (sys.min_seq_nbr == bhdr_src.seq_nbr)
    {
        // just erased the minimum seq_nbr, dig out new minimum from all blocks
        word_t max_diff = 0;
        for (uint8_t b = 0; b < sys.nbr_of_blocks; b++)
        {
            block_header_t bhdr;
            res = block_read_hdr(b, &bhdr);
            ERR_RET(res);
            if (block_type(&bhdr) != BLOCK_TYPE_DATA)
                continue;
            if (seq_nbr_is_newer(sys.max_seq_nbr, bhdr.seq_nbr) &&
                seq_nbr_diff(sys.max_seq_nbr, bhdr.seq_nbr) > max_diff)
            {
                max_diff = seq_nbr_diff(sys.max_seq_nbr, bhdr.seq_nbr);
                sys.min_seq_nbr = bhdr.seq_nbr;
            }
        }
    }
    _dbg("post-evict, spare block:%d, min seq:%d, max_seq:%d\n", sys.spare_block_ix, sys.min_seq_nbr, sys.max_seq_nbr);
    return 0;
}

int nvmtnvj_gc(void)
{
    if (sys.state == STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    if (sys.state == STATE_MOUNTED_INCONSISTENT)
        return ERR_NVMTNVJ_FS_ABORTED;
    int res;
    uint32_t evict_block_ix;
    tag_evict_info_t evict_tag_info[sys.tags_per_block];
    res = block_find_evict_candidate(&evict_block_ix, evict_tag_info);
    ERR_RET(res);
    if (evict_block_ix == UNDEF_IX)
        return ERR_NVMTNVJ_FULL;
    res = block_evict(evict_block_ix, evict_tag_info, sys.spare_block_ix);
    return res;
}

static int prepare_for_new_entry(void)
{
    if (sys.state == STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    if (sys.state == STATE_MOUNTED_INCONSISTENT)
        return ERR_NVMTNVJ_FS_ABORTED;
    if (sys.current_tag_ix < sys.tags_per_block)
        return 0;

    // find next free block
    uint32_t free_block_ix;
    word_t seq_nbr;
    int res = block_alloc(&free_block_ix, &seq_nbr);
    if (res >= 0)
    {
        uint32_t free_tag_ix;
        // should in all cases always return tag index 0, but keep this for symmetry
        res = tag_find_next_free_in_block(free_block_ix, &free_tag_ix);
        ERR_RET(res);
        // found free
        sys.current_block_ix = free_block_ix;
        sys.current_tag_ix = free_tag_ix;
        sys.max_seq_nbr = seq_nbr;
        return 0;
    }
    if (res != ERR_NVMTNVJ_NOENT)
        return res;

    // no free, gc
    return nvmtnvj_gc();
}

int nvmtnvj_write(uint16_t tag_id, const uint8_t *src, uint8_t size)
{
    int res = prepare_for_new_entry();
    ERR_RET(res);
    res = tag_write(sys.current_block_ix, sys.current_tag_ix, tag_id, TAG_WRITTEN, src, size);
    ERR_RET(res);
    sys.current_tag_ix++;
    return 0;
}

int nvmtnvj_read(uint16_t tag_id, uint8_t *dst)
{
    if (sys.state == STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    if (sys.state == STATE_MOUNTED_INCONSISTENT)
        return ERR_NVMTNVJ_FS_ABORTED;
    return tag_find_and_read(tag_id, dst);
}

int nvmtnvj_size(uint16_t tag_id)
{
    if (sys.state == STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    if (sys.state == STATE_MOUNTED_INCONSISTENT)
        return ERR_NVMTNVJ_FS_ABORTED;
    uint8_t tmp_buf[sys.max_value_size];
    return tag_find_and_read(tag_id, tmp_buf);
}

int nvmtnvj_delete(uint16_t tag_id)
{
    if (sys.state == STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    if (sys.state == STATE_MOUNTED_INCONSISTENT)
        return ERR_NVMTNVJ_FS_ABORTED;
    uint8_t tmp_buf[sys.max_value_size];
    int res = tag_find_and_read(tag_id, tmp_buf);
    if (res == ERR_NVMTNVJ_NOENT)
        return 0; // no need to delete
    res = prepare_for_new_entry();
    ERR_RET(res);
    res = tag_write(sys.current_block_ix, sys.current_tag_ix, tag_id, TAG_DELETED, NULL, 0);
    ERR_RET(res);
    sys.current_tag_ix++;
    return 0;
}

int nvmtnvj_format(uint32_t sector_start, uint8_t sectors_per_block, uint8_t block_count, uint8_t max_value_size)
{
    if (block_count < 2 || sectors_per_block < 1)
        return ERR_NVMTNVJ_INVAL;

    // check that all sectors exist and are of same size
    int sect_size = 0;
    uint32_t sector_count = sectors_per_block * block_count;
    for (uint32_t s = sector_start; s < sector_start + sector_count; s++)
    {
        int cur_sect_size = flash_get_sector_size(s);
        if (cur_sect_size < 0)
            return cur_sect_size;
        if (sect_size && sect_size != cur_sect_size)
            return ERR_NVMTNVJ_NONUNIFORM;
        sect_size = cur_sect_size;
    }
    if (sect_size % CONFIG_NVMTNVJ_FLASH_WORD_SIZE != 0)
        return ERR_NVMTNVJ_FATAL; // sector size is not aligned with configuration for flash word size

    sys.state = STATE_UNMOUNTED;
    sys.max_value_size = max_value_size;
    sys.starting_sector = sector_start;
    sys.nbr_of_blocks = block_count;
    sys.sectors_per_block = sectors_per_block;

    uint32_t block_size = sect_size * sectors_per_block;
    uint32_t tags_per_block = (block_size - ALIGNW(sizeof(block_header_t))) / tag_size();
    if (tags_per_block == 0)
        return ERR_NVMTNVJ_FATAL;

    sys.state = STATE_UNMOUNTED;

    // erase all and write headers
    int res = 0;
    for (uint8_t b = 0; res == 0 && b < sys.nbr_of_blocks; b++)
    {
        // make last block spare
        block_type_t t = BLOCK_TYPE_DATA_FREE;
        if (b == sys.nbr_of_blocks - 1)
            t = BLOCK_TYPE_SPARE;
        res = block_erase(b, t);
    }
    return res;
}

int nvmtnvj_fix(void)
{
    if (sys.state == STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    if (sys.state == STATE_MOUNTED)
        return 0; // allow calling fix even if unnecessary
    int res;
    if (sys.evict_block_ix != UNDEF_IX && (sys.spare_block_ix != UNDEF_IX || sys.unknown_block_ix != UNDEF_IX))
    {
        // eviction aborted during moving live tags
        // clean aborted spare block
        uint32_t block_ix_to_make_spare = sys.spare_block_ix != UNDEF_IX ? sys.spare_block_ix : sys.unknown_block_ix;
        res = block_erase(block_ix_to_make_spare, BLOCK_TYPE_SPARE);
        ERR_RET(res);
        sys.spare_block_ix = block_ix_to_make_spare;

        // evict the marked evicted block to cleaned spare block
        uint32_t sorted_block_list[sys.nbr_of_blocks];
        word_t sorted_seq_nbr_list[sys.nbr_of_blocks];
        sorted_blocks_t sorted_blocks = {
            .blocks = sorted_block_list,
            .seq_nbrs = sorted_seq_nbr_list,
            .count = 0};
        res = blocks_sort(&sorted_blocks);
        ERR_RET(res);

        tag_evict_info_t tag_info[sys.tags_per_block];
        uint32_t sorted_evict_block_ix = UNDEF_IX;
        for (uint32_t b = 0; b < sorted_blocks.count; b++)
        {
            if (sorted_blocks.blocks[b] == sys.evict_block_ix)
            {
                sorted_evict_block_ix = b;
                break;
            }
        }
        if (sorted_evict_block_ix == UNDEF_IX)
            return ERR_NVMTNVJ_FATAL;

        res = block_find_freeables(sorted_evict_block_ix, &sorted_blocks, tag_info);
        ERR_RET(res);

        res = block_evict(sys.evict_block_ix, tag_info, sys.spare_block_ix);
        ERR_RET(res);
    }
    else if (sys.spare_block_ix == UNDEF_IX && sys.current_block_ix != UNDEF_IX &&
             ((sys.evict_block_ix != UNDEF_IX && sys.unknown_block_ix == UNDEF_IX) ||
              (sys.evict_block_ix == UNDEF_IX && sys.unknown_block_ix != UNDEF_IX)))
    {
        // eviction of live tags done, but old evicting block not yet made spare
        uint32_t block_ix_to_make_spare = sys.evict_block_ix != UNDEF_IX ? sys.evict_block_ix : sys.unknown_block_ix;
        res = block_erase(block_ix_to_make_spare, BLOCK_TYPE_SPARE);
        ERR_RET(res);
        sys.spare_block_ix = block_ix_to_make_spare;
        // refresh cursor so first write after fix needs no lookups
        res = tag_find_next_free_in_block(sys.current_block_ix, &sys.current_tag_ix);
        if (res == ERR_NVMTNVJ_NOENT)
        {
            sys.current_tag_ix = sys.tags_per_block;
            res = 0;
        }
        ERR_RET(res);
    }
    else
    {
        return ERR_NVMTNVJ_FATAL;
    }
    sys.state = STATE_MOUNTED;
    return 0;
}

int nvmtnvj_unmount(void)
{
    if (sys.state == STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    sys.state = STATE_UNMOUNTED;
    return 0;
}

int nvmtnvj_mount(uint32_t sector_start, uint8_t max_lookahead_sectors)
{
    int res;
    block_header_t bhdr;

    if (sys.state != STATE_UNMOUNTED)
        return ERR_NVMTNVJ_MOUNT;
    sys.starting_sector = sector_start;

    // find first valid block header
    uint32_t phys_sector_start = sector_start - 1;
    for (uint32_t s = sector_start; s <= sector_start + max_lookahead_sectors; s++)
    {
        res = flash_read(s, 0, (uint8_t *)&bhdr, sizeof(block_header_t));
        ERR_RET(res);
        if (bhdr.descr.magic != MAGIC)
            continue;
        phys_sector_start = s;
        break;
    }
    if (phys_sector_start == sector_start - 1)
        return ERR_NVMTNVJ_NOFS; // no block header found in any sector
    if (phys_sector_start != sector_start &&
        phys_sector_start - sector_start != bhdr.descr.sectors_per_block)
        return ERR_NVMTNVJ_NOFS; // sectors per block mismatch

    sys.nbr_of_blocks = bhdr.descr.nbr_of_blocks;
    sys.sectors_per_block = bhdr.descr.sectors_per_block;
    sys.max_value_size = bhdr.descr.max_value_size;

    // sector size validation
    int sect_size = flash_get_sector_size(sector_start);
    if (sect_size < 0)
        return sect_size;
    sys.sector_size = sect_size;
    if (sect_size % CONFIG_NVMTNVJ_FLASH_WORD_SIZE != 0)
        return ERR_NVMTNVJ_FATAL; // sector size is not aligned with configuration for flash word size
    uint32_t block_size = sect_size * sys.sectors_per_block;
    sys.tags_per_block = (block_size - ALIGNW(sizeof(block_header_t))) / tag_size();
    if (sys.tags_per_block == 0)
        return ERR_NVMTNVJ_FATAL; // this prevents the purpose of this module

    // check fs consistency
    uint32_t data_min_seq_block_ix = UNDEF_IX;
    uint32_t data_max_seq_block_ix = UNDEF_IX;
    uint32_t unknown_block_ix = UNDEF_IX;
    uint32_t spare_block_ix = UNDEF_IX;
    uint32_t evict_block_ix = UNDEF_IX;
    uint8_t data_block_count = 0;
    sys.min_seq_nbr = SEQ_NBR_UNWRITTEN;
    sys.max_seq_nbr = SEQ_NBR_UNWRITTEN;
    for (uint8_t b = 0; b < sys.nbr_of_blocks; b++)
    {
        res = block_read_hdr(b, &bhdr);
        ERR_RET(res);
        switch (block_type(&bhdr))
        {
        case BLOCK_TYPE_DATA:
            if (sys.max_seq_nbr == SEQ_NBR_UNWRITTEN || bhdr.seq_nbr > sys.max_seq_nbr)
            {
                sys.max_seq_nbr = bhdr.seq_nbr;
                data_max_seq_block_ix = b;
            }
            if (sys.min_seq_nbr == SEQ_NBR_UNWRITTEN || bhdr.seq_nbr < sys.min_seq_nbr)
            {
                sys.min_seq_nbr = bhdr.seq_nbr;
                data_min_seq_block_ix = b;
            }
            data_block_count++;
            break;
        case BLOCK_TYPE_DATA_FREE:
            data_block_count++;
            break;
        // there can be only one (of each) by design - else borked beyond repair
        case BLOCK_TYPE_EVICTING:
            if (evict_block_ix != UNDEF_IX)
                return ERR_NVMTNVJ_NOFS;
            evict_block_ix = b;
            break;
        case BLOCK_TYPE_SPARE:
            if (spare_block_ix != UNDEF_IX)
                return ERR_NVMTNVJ_NOFS;
            spare_block_ix = b;
            break;
        case BLOCK_TYPE_UNKNOWN:
            if (unknown_block_ix != UNDEF_IX)
                return ERR_NVMTNVJ_NOFS;
            unknown_block_ix = b;
            break;
        }
    }
    if (data_block_count < sys.nbr_of_blocks - 2 || data_block_count > sys.nbr_of_blocks - 1)
        return ERR_NVMTNVJ_NOFS;
    // cannot have all
    if (unknown_block_ix != UNDEF_IX && spare_block_ix != UNDEF_IX && evict_block_ix != UNDEF_IX)
        return ERR_NVMTNVJ_NOFS;
    // cannot have none
    if (unknown_block_ix == UNDEF_IX && spare_block_ix == UNDEF_IX && evict_block_ix == UNDEF_IX)
        return ERR_NVMTNVJ_NOFS;

    sys.spare_block_ix = spare_block_ix;
    sys.evict_block_ix = evict_block_ix;
    sys.unknown_block_ix = unknown_block_ix;

    if (sys.min_seq_nbr == SEQ_NBR_UNWRITTEN)
    {
        // no data pages, only data_free, just formatted - make our data page
        res = block_alloc(&data_max_seq_block_ix, &sys.max_seq_nbr);
        ERR_RET(res);
        sys.min_seq_nbr = sys.max_seq_nbr;
        data_min_seq_block_ix = data_max_seq_block_ix;
    }

    // sequence number wrap guard
    if (seq_nbr_is_newer(sys.min_seq_nbr, sys.max_seq_nbr))
    {
        word_t tmp = sys.min_seq_nbr;
        sys.min_seq_nbr = sys.max_seq_nbr;
        sys.max_seq_nbr = tmp;
        uint32_t btmp = data_min_seq_block_ix;
        data_min_seq_block_ix = data_max_seq_block_ix;
        data_max_seq_block_ix = btmp;
    }
    sys.current_block_ix = data_max_seq_block_ix;

    // sort out mount state
    if (data_block_count == sys.nbr_of_blocks - 1 && spare_block_ix != UNDEF_IX)
        sys.state = STATE_MOUNTED;
    else if ((data_block_count == sys.nbr_of_blocks - 2 && evict_block_ix != UNDEF_IX && spare_block_ix != UNDEF_IX) ||
             (data_block_count == sys.nbr_of_blocks - 2 && evict_block_ix != UNDEF_IX && unknown_block_ix != UNDEF_IX) ||
             (data_block_count == sys.nbr_of_blocks - 1 && evict_block_ix != UNDEF_IX) ||
             (data_block_count == sys.nbr_of_blocks - 1 && unknown_block_ix != UNDEF_IX))
        sys.state = STATE_MOUNTED_INCONSISTENT;

    sys.current_tag_ix = UNDEF_IX;
    if (sys.state == STATE_MOUNTED)
    {
        res = tag_find_next_free_in_block(sys.current_block_ix, &sys.current_tag_ix);
        if (res == ERR_NVMTNVJ_NOENT)
        {
            sys.current_tag_ix = sys.tags_per_block;
            res = 0;
        }
        ERR_RET(res);
    }

    _dbg("sys.state:                %d\n", sys.state);
    _dbg("sys.starting_sector:      %d\n", sys.starting_sector);
    _dbg("sys.sector_size:          %d\n", sys.sector_size);
    _dbg("sys.sectors_per_block:    %d\n", sys.sectors_per_block);
    _dbg("sys.nbr_of_blocks:        %d\n", sys.nbr_of_blocks);
    _dbg("sys.tags_per_block:       %d\n", sys.tags_per_block);
    _dbg("sys.max_value_size:       %d\n", sys.max_value_size);
    _dbg("sys.current_block_ix:     %d\n", sys.current_block_ix);
    _dbg("sys.current_tag_ix:       %d\n", sys.current_tag_ix);
    _dbg("sys.spare_block_ix:       %d\n", sys.spare_block_ix);
    _dbg("sys.evict_block_ix:       %d\n", sys.evict_block_ix);
    _dbg("sys.unknown_block_ix:     %d\n", sys.unknown_block_ix);
    _dbg("sys.min_seq_nbr:          %d\n", sys.min_seq_nbr);
    _dbg("sys.max_seq_nbr:          %d\n", sys.max_seq_nbr);

    switch (sys.state)
    {
    case STATE_MOUNTED:
        return 0;
    case STATE_MOUNTED_INCONSISTENT:
        return ERR_NVMTNVJ_FS_ABORTED;
    default:
        return ERR_NVMTNVJ_NOFS;
    }
}

void nvmtnvj_init(void)
{
    sys.state = STATE_UNMOUNTED;
}

#if NVMTNVJ_TEST
// expose some privates to ease unittests
int nvmtnvj_test_tags_per_block(void)
{
    return sys.tags_per_block;
}

int nvmtnvj_test_dump(void)
{
    int res;
    block_header_t bhdr;
    for (uint8_t b = 0; b < sys.nbr_of_blocks; b++)
    {
        res = block_read_hdr(b, &bhdr);
        ERR_RET(res);
        switch (block_type(&bhdr))
        {
        case BLOCK_TYPE_DATA:
            _dbg("block %d\tDATA, seq %08x\n", b, bhdr.seq_nbr);
            for (uint32_t t = 0; t < sys.tags_per_block; t++)
            {
                tag_header_t thdr;
                res = tag_read_hdr_in_block(b, t, &thdr);
                ERR_RET(res);
                switch (thdr.state)
                {
                case TAG_FREE:
                    _dbg("\t%d\tFREE\n", t);
                    break;
                case TAG_WRITTEN:
                    _dbg("\t%d\tWRIT %04x\tsz:%d\tchk:%02x\n", t, thdr.id, thdr.len, thdr.chk);
                    break;
                case TAG_DELETED:
                    _dbg("\t%d\tDELE %04x\n", t, thdr.id);
                    break;
                }
            }
            break;
        case BLOCK_TYPE_DATA_FREE:
            _dbg("block %d\tFREE\n", b);
            break;
        case BLOCK_TYPE_EVICTING:
            _dbg("block %d\tEVICT\n", b);
            break;
        case BLOCK_TYPE_SPARE:
            _dbg("block %d\tSPARE\n", b);
            break;
        case BLOCK_TYPE_UNKNOWN:
            _dbg("block %d\tUNKNOWN\n", b);
            break;
        }
    }
    return 0;
}

#endif
