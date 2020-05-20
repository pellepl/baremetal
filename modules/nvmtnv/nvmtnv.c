/* Copyright (c) 2020 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */
#include "nvmtnv.h"
#include "flash_driver.h"

#ifndef NVMTNV_DBG
#include "minio.h"
#define NVMTNV_DBG(...) printf( __VA_ARGS__ )
#endif

#define _dbg(...) NVMTNV_DBG( __VA_ARGS__ )

#define MAGIC               0xb0ba

// sector state transitions
// DATA        SPARE
// DATA (full) SPARE
// EVICT       SPARE
//(EVICT       ERASING) (evict interrupted, erase spare sector to restart, but interrupted again)
// EVICT       DATA     (evict done)
// ERASING     DATA
// SPARE       DATA
#define STATE_SECT_SPARE    0xaa5555aa
#define STATE_SECT_DATA     0xaa550000
#define STATE_SECT_EVICT    0x00000000
#define STATE_SECT_ERASING  0xffffffff

#define STATE_TAG_FREE      0xffffffff
#define STATE_TAG_WRITING   0xfafaffff
#define STATE_TAG_WRITTEN   0xfafaafaf
#define STATE_TAG_CHANGING  0x0000afaf
#define STATE_TAG_DELETED   0x00000000
#define STATE_TAG_MASK_FREE         (1<<0)
#define STATE_TAG_MASK_WRITING      (1<<1)
#define STATE_TAG_MASK_WRITTEN      (1<<2)
#define STATE_TAG_MASK_CHANGING     (1<<3)
#define STATE_TAG_MASK_DELETED      (1<<4)

// tag state transitions, writing tag A, overwrite tag A
// A
// FREE      FREE
// WRITING   FREE
// WRITTEN   FREE       (A defined)
// 
// A         A'
// WRITTEN   FREE
// CHANGING  FREE
// CHANGING  WRITING
// CHANGING  WRITTEN    (A' defined)
// DELETED   WRITTEN
//
// 1 if WRITING only => no value, mark DELETED
// 2 if WRITTEN => value
// 3 if CHANGING only  => value, but copy out same value to new node
// 4 if CHANGING and WRITING => select CHANGING, mark WRITING as DELETED, then apply 3
// 5 if CHANGING and WRITTEN => select WRITTEN, mark CHANGING as DELETED

#define TAGLOC(sector, tagix)   (uint32_t)( (((sector)&0xffff)<<16) | ((tagix)&0xffff))
#define SECTOR(tagloc)          (uint32_t)( ((tagloc)&0xffff0000)>>16 )
#define TAGIX(tagloc)           (uint32_t)( ((tagloc)&0xffff) )
#define TAGLOC_NONE             (uint32_t)(0xffffffff)

typedef struct {
    uint32_t state;
    uint16_t magic;
    uint8_t fill;
    uint8_t max_value_size;
    uint16_t sect_nbr;
    uint16_t sect_count;
} tnv_sector_header_t;

typedef struct {
    uint32_t state;
    uint8_t size;
    uint8_t fill;
    uint16_t tag;
} tnv_header_t;

static struct {
    enum { UNMOUNTED = 0, MOUNTED } mount_state;
    enum { OK = 0, EVICTING, EVICTING_FIXING, ERASE, ERASING } consistency_state;
    uint32_t sect_start;
    uint16_t sect_count;
    uint32_t sect_spare;
    uint32_t sect_evict_abort, sect_erase_abort;
    uint8_t max_value_size;
    uint32_t tags_per_sect;
    uint32_t free, dele;
} sys = {0};

static int update_tag(tnv_header_t *src_hdr, uint32_t src_tagloc, uint32_t dst_tagloc, const uint8_t *new_data, uint8_t new_size);

static int state_ok(void) {
    if (sys.mount_state != MOUNTED) return ERR_NVMTNV_MOUNT;
    if (sys.consistency_state != OK) return ERR_NVMTNV_FS_ABORTED;
    return 0;
}

static int tag_write_hdr(uint32_t tagloc, tnv_header_t *thdr) {
    uint32_t sect = SECTOR(tagloc);
    uint32_t offs = sizeof(tnv_sector_header_t) + TAGIX(tagloc)*(sizeof(tnv_header_t) + sys.max_value_size);
    return flash_write(sect, offs, (uint8_t *)thdr, sizeof(tnv_header_t));
}

static int tag_read_hdr(uint32_t tagloc,  tnv_header_t *thdr) {
    uint32_t sect = SECTOR(tagloc);
    uint32_t offs = sizeof(tnv_sector_header_t) + TAGIX(tagloc)*(sizeof(tnv_header_t) + sys.max_value_size);
    return flash_read(sect, offs, (uint8_t *)thdr, sizeof(tnv_header_t));
}

static int find_tag(uint16_t tag, uint32_t tagloc_exclude, uint8_t state_mask, uint32_t *tagloc, tnv_header_t *thdr) {
    int res;
    _dbg("find tag %04x, exclude tl %08x, states %02x\n", tag, tagloc_exclude, state_mask);
    for (uint32_t s = sys.sect_start; s < sys.sect_start + sys.sect_count; s++) {
        if (s == sys.sect_spare) continue;
        for (uint32_t t = 0; t < sys.tags_per_sect; t++) {
            if (tagloc_exclude != TAGLOC_NONE && TAGLOC(s,t) == tagloc_exclude) {
                continue;
            }
            res = tag_read_hdr(TAGLOC(s,t), thdr);
            if (res < 0) return res;
            if ((thdr->state == STATE_TAG_FREE && (state_mask & STATE_TAG_MASK_FREE)) ||
                (thdr->state == STATE_TAG_WRITING && (state_mask & STATE_TAG_MASK_WRITING)) ||
                (thdr->state == STATE_TAG_WRITTEN && (state_mask & STATE_TAG_MASK_WRITTEN)) ||
                (thdr->state == STATE_TAG_CHANGING && (state_mask & STATE_TAG_MASK_CHANGING)) ||
                (thdr->state == STATE_TAG_DELETED && (state_mask & STATE_TAG_MASK_DELETED))) {
                if (thdr->tag == tag || thdr->state == STATE_TAG_FREE || thdr->state == STATE_TAG_DELETED) {
                    _dbg("found @ tl %08x, state %08x\n", TAGLOC(s,t), thdr->state);
                    if (tagloc) *tagloc = TAGLOC(s,t);
                    return 0;
                }
            }
        }
    }
    if (tagloc) *tagloc = TAGLOC_NONE;
    return 0;
}


static int evict(uint32_t sector_to, uint32_t sector_from) {
    _dbg("evict sector %08x, free %d, dele %d\n", sector_from, sys.free, sys.dele);
    int res;
    tnv_sector_header_t shdr_from, shdr_to;
    res = flash_read(sector_from, 0, (uint8_t *)&shdr_from, sizeof(tnv_sector_header_t));
    if (res < 0) return res;
    res = flash_read(sector_to, 0, (uint8_t *)&shdr_to, sizeof(tnv_sector_header_t));
    if (res < 0) return res;

    // set FROM state to EVICT
    if (shdr_from.state != STATE_SECT_EVICT) {
        shdr_from.state = STATE_SECT_EVICT;
        res = flash_write(sector_from, 0, (uint8_t *)&shdr_from, sizeof(tnv_sector_header_t));
        if (res < 0) return res;
    }

    // move data
    uint32_t new_t = 0;
    for (uint32_t t = 0; t < sys.tags_per_sect; t++) {
        tnv_header_t thdr;
        res = tag_read_hdr(TAGLOC(sector_from,t), &thdr);
        if (res < 0) return res;
        char move = 0;
        _dbg("  tag %04x, state %02x\n", thdr.tag, thdr.state);
        if (thdr.state != STATE_TAG_FREE) {
            sys.free++;
        } else if (thdr.state == STATE_TAG_DELETED) {
            sys.dele--;
        }
        if (thdr.state == STATE_TAG_WRITTEN) {
            move = 1;
        } else if (thdr.state == STATE_TAG_CHANGING) {
            tnv_header_t wrt_hdr;
            uint32_t wrt_tagloc;
            res = find_tag(thdr.tag, sector_from, STATE_TAG_MASK_WRITTEN, &wrt_tagloc, &wrt_hdr);
            if (res < 0) return res;
            if (wrt_tagloc == TAGLOC_NONE) move = 1;
        }

        if (move) {
            _dbg("  move %04x\n", thdr.tag);
            sys.free--;
            thdr.state = STATE_TAG_WRITTEN;
            res = update_tag(&thdr, TAGLOC(sector_from, t), TAGLOC(sector_to, new_t++), 0, 0);
            if (res < 0) return res;
        }
    }

    // all data moved, set TO state to DATA
    shdr_to.state = STATE_SECT_DATA;
    res = flash_write(sector_to, 0, (uint8_t *)&shdr_to, sizeof(tnv_sector_header_t));
    if (res < 0) return res;

    // erase FROM, FROM state is ERASING
    res = flash_erase(sector_from);
    if (res < 0) return res;

    // set FROM state to SPARE
    shdr_from.state = STATE_SECT_SPARE;
    res = flash_write(sector_from, 0, (uint8_t *)&shdr_from, sizeof(tnv_sector_header_t));
    if (res < 0) return res;

    sys.sect_spare = sector_from;

    _dbg("evicted sector %08x, free %d, dele %d\n", sector_from, sys.free, sys.dele);

    return 0;
}

static int gc(void) {
    int res;
    tnv_header_t thdr;
    if (sys.free > 1) return 0;
    if (sys.dele == 0) return ERR_NVMTNV_FULL;
    // find sector to evict
    uint32_t max_score = 0;
    uint32_t max_score_sect = 0;
    for (uint32_t s = sys.sect_start; s < sys.sect_start + sys.sect_count; s++) {
        if (s == sys.sect_spare) continue;
        uint32_t score = 0;
        for (uint32_t t = 0; t < sys.tags_per_sect; t++) {
            res = tag_read_hdr(TAGLOC(s,t), &thdr);
            if (res < 0) return res;
            if (thdr.state == STATE_TAG_DELETED || thdr.state == STATE_TAG_WRITTEN) {
                score++;
            }
        }
        if (score > max_score) {
            max_score = score;
            max_score_sect = s;
        }
    }
    if (max_score == 0) {
        _dbg("gc didn't find evict candidate\n");
        return ERR_NVMTNV_FULL;
    }
    _dbg("gc evict sector %08x to %08x\n", max_score_sect, sys.sect_spare);
    res = evict(sys.sect_spare, max_score_sect);
    return res;
}

// Creates a new tag, updates a tag, moves a tag. Does not invoke GC.
// @param src_hdr       src_hdr.tag must be defined
// @param src_tagloc    if not TAGLOC_NONE, the source will be treated as being modified
// @param dst_tagloc    if TAGLOC_NONE a free tag is found automatically, only set during eviction
// @param new_data      data for new or modified tag, if null data is copied from src_tagloc
// @param new_size      size for new or modified tag, if null size is copied from src_tagloc
static int update_tag(tnv_header_t *src_hdr, uint32_t src_tagloc, uint32_t dst_tagloc, const uint8_t *new_data, uint8_t new_size) {
    int res;
    tnv_header_t free_hdr, dst_hdr;

    if ((new_data == 0 || new_size == 0) && src_tagloc == TAGLOC_NONE) return -1;


    char is_evict = SECTOR(dst_tagloc) == sys.sect_spare;
    _dbg("upd tag %04x, free %d, dele %d %s\n", src_hdr->tag, sys.free, sys.dele, is_evict ? "EVICT" : "");
    dst_hdr.tag = src_hdr->tag;
    dst_hdr.size = new_size == 0 ? src_hdr->size : new_size;

    // need destination, find free tag space
    if (dst_tagloc == TAGLOC_NONE) {
        if (!is_evict && src_tagloc != TAGLOC_NONE && sys.free == 0) {
            return ERR_NVMTNV_FULL;
        }
        if (!is_evict && src_tagloc == TAGLOC_NONE && sys.free <= 1) {
            // always keep a free slot for tags being modified
            return ERR_NVMTNV_FULL;
        }
        res = find_tag(0, TAGLOC_NONE, STATE_TAG_MASK_FREE, &dst_tagloc, &free_hdr);
        if (res < 0) return res;
        if (dst_tagloc == TAGLOC_NONE) return ERR_NVMTNV_FULL;
        _dbg("upd tag %04x found free tl %08x\n", src_hdr->tag, dst_tagloc);
    }

    // if there is a written source location, set that to changing
    if (!is_evict && src_tagloc != TAGLOC_NONE && src_hdr->state == STATE_TAG_WRITTEN) {
        src_hdr->state = STATE_TAG_CHANGING;
        _dbg("upd tag %04x changing tl %08x\n", src_hdr->tag, src_tagloc);
        res = tag_write_hdr(src_tagloc, src_hdr);
        if (res < 0) return res;
    }

    // set and write dst header as writing
    if (!is_evict) {
        dst_hdr.state = STATE_TAG_WRITING;
        _dbg("upd tag %04x writing tl %08x\n", src_hdr->tag, dst_tagloc);
        res = tag_write_hdr(dst_tagloc, &dst_hdr);
        if (res < 0) return res;
        sys.free--;
    }

    // write data
    if (new_data && new_size) {
        _dbg("upd tag %04x new data size %d tl %08x\n", src_hdr->tag, new_size, dst_tagloc);
        // data provided by caller
        res = flash_write(SECTOR(dst_tagloc),
            sizeof(tnv_sector_header_t) + TAGIX(dst_tagloc)*(sizeof(tnv_header_t) + sys.max_value_size) + sizeof(tnv_header_t),
            new_data, new_size);
        if (res < 0) return res;
    } else {
        // no data, take source tnv data
        _dbg("upd tag %04x old data size %d tl %08x\n", src_hdr->tag, src_hdr->size, dst_tagloc);
        uint8_t buf[src_hdr->size];
        res = flash_read(SECTOR(src_tagloc),
            sizeof(tnv_sector_header_t) + TAGIX(src_tagloc)*(sizeof(tnv_header_t) + sys.max_value_size) + sizeof(tnv_header_t),
            buf, src_hdr->size);
        if (res < 0) return res;
        res = flash_write(SECTOR(dst_tagloc),
            sizeof(tnv_sector_header_t) + TAGIX(dst_tagloc)*(sizeof(tnv_header_t) + sys.max_value_size) + sizeof(tnv_header_t),
            buf, src_hdr->size);
        if (res < 0) return res;
    }

    // finalize dst header
    _dbg("upd tag %04x written tl %08x\n", src_hdr->tag, dst_tagloc);
    dst_hdr.state = STATE_TAG_WRITTEN;
    res = tag_write_hdr(dst_tagloc, &dst_hdr);
    if (res < 0) return res;

    // if there is a source location, set that to deleted
    if (!is_evict && src_tagloc != TAGLOC_NONE && src_hdr->state != STATE_TAG_FREE && src_hdr->state != STATE_TAG_DELETED) {
        _dbg("upd tag %04x deleted tl %08x\n", src_hdr->tag, src_tagloc);
        src_hdr->state = STATE_TAG_DELETED;
        res = tag_write_hdr(src_tagloc, src_hdr);
        if (res < 0) return res;
        sys.dele++;
    }

    return 0;
}

static int check_tag_consistency(void) {
    // check tnv header consistency
    uint32_t tagloc_changing = TAGLOC_NONE;
    uint32_t tagloc_writing = TAGLOC_NONE;
    int res = 0;
    sys.free = sys.dele = 0;
    for (uint32_t s = sys.sect_start; res >= 0 && s < sys.sect_start + sys.sect_count; s++) {
        tnv_sector_header_t shdr;
        res = flash_read(s, 0, (uint8_t *)&shdr, sizeof(shdr));
        if (res < 0) return res;
        if (shdr.state == STATE_SECT_SPARE) continue;
        for (uint32_t t = 0; res >= 0 && t < sys.tags_per_sect; t++) {
            tnv_header_t thdr;
            res = tag_read_hdr(TAGLOC(s,t), &thdr);
            if (res < 0) return res;
            switch (thdr.state) {
                case STATE_TAG_WRITTEN: break;
                case STATE_TAG_FREE:    sys.free++; break;
                case STATE_TAG_DELETED: sys.dele++; break;

                case STATE_TAG_WRITING:
                _dbg("tag cons found writing tag %04x @ tl %08x\n", thdr.tag, TAGLOC(s,t));
                if (tagloc_writing != TAGLOC_NONE) {
                    return ERR_NVMTNV_NOFS;
                }
                tagloc_writing = TAGLOC(s,t);
                break;

                case STATE_TAG_CHANGING: 
                _dbg("tag cons found changing tag %04x @ tl %08x\n", thdr.tag, TAGLOC(s,t));
                tagloc_changing = TAGLOC(s,t);
                break;

                default:
                _dbg("tag cons found bad state %08x tag %04x @ tl %08x, deleting\n", thdr.state, thdr.tag, TAGLOC(s,t));
                thdr.state = STATE_TAG_DELETED;
                res = tag_write_hdr(TAGLOC(s, t), &thdr);
                if (res < 0) return res;
                sys.dele++;
                break;
            }
        }
    }

    if (tagloc_changing != TAGLOC_NONE) {
        tnv_header_t thdr_changing, thdr_other;
        uint32_t tagloc_other;
        res = tag_read_hdr(tagloc_changing, &thdr_changing);
        if (res < 0) return res;
        res = find_tag(thdr_changing.tag, tagloc_changing, 
            STATE_TAG_MASK_WRITING | STATE_TAG_MASK_WRITTEN, &tagloc_other, &thdr_other);
        if (res < 0) return res;
        if (tagloc_other != TAGLOC_NONE) {
            if (thdr_other.state == STATE_TAG_WRITING) {
                // remove writing, prefer changing
                _dbg("tag cons changing tag %04x have writing @ tl %08x\n", thdr_changing.tag, tagloc_other);
                tagloc_writing = TAGLOC_NONE;
                thdr_other.state = STATE_TAG_DELETED;
                res = tag_write_hdr(tagloc_other, &thdr_other);
                if (res < 0) return res;
                sys.dele++;
                 // forget about the writing now so we can try to move the changing to state written below
                tagloc_other = TAGLOC_NONE;
            } else if (thdr_other.state == STATE_TAG_WRITTEN) {
                // remove changing, prefer written
                _dbg("tag cons changing tag %04x have written @ tl %08x\n", thdr_changing.tag, tagloc_other);
                thdr_changing.state = STATE_TAG_DELETED;
                res = tag_write_hdr(tagloc_changing, &thdr_changing);
                if (res < 0) return res;
                sys.dele++;
            }
        }

        // do not use "else if" here, tagloc_other might have changed, see above
        if (tagloc_other == TAGLOC_NONE) {
            // only this id with state changing, try to move to state written
            _dbg("tag cons changing tag %04x updating to written\n", thdr_changing.tag);
            if (sys.free > 0) {
                // possible
                res = update_tag(&thdr_changing, tagloc_changing, TAGLOC_NONE, 0, 0);
                if (res < 0) return res;
            } else if (sys.dele > 0) {
                // must perform a gc
                res = gc();
                if (res < 0) return res;
                res = update_tag(&thdr_changing, tagloc_changing, TAGLOC_NONE, 0, 0);
                if (res < 0) return res;
            } else {
                // nothing to do, accept having a tag with state changing
                // :(
                _dbg("tag cons changing tag %04x updating to written not possible\n", thdr_changing.tag);
            }
        }
    } 

    if (tagloc_writing != TAGLOC_NONE) {
        // writing only, no corresponding changing -> delete
        tnv_header_t thdr;
        res = tag_read_hdr(tagloc_writing, &thdr);
        if (res < 0) return res;
        _dbg("tag cons writing tag %04x delete\n", thdr.tag);
        thdr.state = STATE_TAG_DELETED;
        res = tag_write_hdr(tagloc_writing, &thdr);
        if (res < 0) return res;
        sys.free++;
    }

    return res;
}

static int get_valid_tag(uint16_t tag, uint32_t *tagloc, tnv_header_t *thdr) {
    int res;
    _dbg("valid tag %04x\n", tag);
    res = find_tag(tag, TAGLOC_NONE, 
        STATE_TAG_MASK_WRITTEN | STATE_TAG_MASK_CHANGING, tagloc, thdr);
    if (res < 0) return res;
    if (*tagloc == TAGLOC_NONE) {
        _dbg("valid tag %04x not found\n", tag);
        return ERR_NVMTNV_NOENT;
    }
    if (thdr->state == STATE_TAG_CHANGING) {
        _dbg("valid tag %04x found, changing\n", tag);
        uint32_t tagloc_wrt;
        tnv_header_t thdr_wrt;
        res = find_tag(tag, TAGLOC_NONE, STATE_TAG_MASK_WRITTEN, &tagloc_wrt, &thdr_wrt);
        if (res < 0) return res;
        if (tagloc_wrt != TAGLOC_NONE) {
            _dbg("valid tag %04x found, changing but have written, delete changing\n", tag);
            thdr->state = STATE_TAG_DELETED;
            res = tag_write_hdr(*tagloc, thdr);
            if (res < 0) return res;
            sys.dele++;
            thdr->tag = thdr_wrt.tag;
            thdr->size = thdr_wrt.size;
            thdr->state = thdr_wrt.state;
            *tagloc = tagloc_wrt;
        }
    }
    return 0;
}

int nvmtnv_mount(uint32_t sector_start) {
    int res;
    tnv_sector_header_t shdr, shdr_n;
    tnv_sector_header_t *shdr_ref;
    int sect_size = flash_get_sector_size(sector_start);
    if (sect_size < 0) return sect_size;
    res = flash_read(sector_start, 0, (uint8_t *)&shdr, sizeof(tnv_sector_header_t));
    if (res < 0) return res;
    res = flash_read(sector_start+1, 0, (uint8_t *)&shdr_n, sizeof(tnv_sector_header_t));
    if (res < 0) return res;

    // find reference start sector
    if (shdr.magic == 0xffff) {
        // might be aborted erase
        if (shdr_n.magic != MAGIC || shdr_n.sect_nbr != 1) {
            _dbg("mount nofs, first sector erased, second have no magic\n");
            return ERR_NVMTNV_NOFS;
        }
        sys.sect_start = sector_start;
        shdr_ref = &shdr_n;
    } else if (shdr.magic != MAGIC || shdr.sect_nbr != 0) {
        _dbg("mount nofs, first sector have bad magic %04x or sect_nbr %04x\n", shdr.magic, shdr.sect_nbr);
        return ERR_NVMTNV_NOFS;
    } else {
        shdr_ref = &shdr;
        sys.sect_start = sector_start;
    }

    // check sector header consistency
    sys.sect_count = shdr_ref->sect_count;
    sys.max_value_size = shdr_ref->max_value_size;
    sys.sect_spare = 0xffffffff;
    sys.sect_erase_abort = 0xffffffff;
    sys.sect_evict_abort = 0xffffffff;
    for (uint32_t s = sector_start; s < sector_start + sys.sect_count; s++) {
        res = flash_read(s, 0, (uint8_t *)&shdr, sizeof(shdr));
        if (res < 0) return res;
        if (shdr.magic == 0xffff) {
            if (sys.sect_erase_abort != 0xffffffff) {
                _dbg("mount nofs sect %08x, multiple aborted sectors\n", s);
                return ERR_NVMTNV_NOFS; // more than one aborted erase sector
            }
            sys.sect_erase_abort = s;
        } else if (shdr.magic != MAGIC) {
            _dbg("mount nofs sect %08x, bad magic\n", s);
            return ERR_NVMTNV_NOFS; // bad magic
        } else {
            if (shdr.sect_nbr != s - sector_start) {
                _dbg("mount nofs sect %08x, invalid sect nbr\n", s);
                return ERR_NVMTNV_NOFS; // invalid sector nbr
            }
            if (shdr.sect_count != sys.sect_count) {
                _dbg("mount nofs sect %08x, invalid sect count\n", s);
                return ERR_NVMTNV_NOFS; // invalid sector count
            }
            if (shdr.state == STATE_SECT_SPARE) {
                if (sys.sect_spare != 0xffffffff) {
                    _dbg("mount nofs sect %08x, multiple spares\n", s);
                    return ERR_NVMTNV_NOFS; // more than one spare sector
                }
                sys.sect_spare = s;
            } else if (shdr.state == STATE_SECT_EVICT) {
                if (sys.sect_evict_abort != 0xffffffff) {
                    _dbg("mount nofs sect %08x, multiple aborted evict sectors\n", s);
                    return ERR_NVMTNV_NOFS; // more than one aborted evict sector
                }
                sys.sect_evict_abort = s;
            } else if (shdr.state != STATE_SECT_DATA) {
                _dbg("mount nofs sect %08x, invalid state\n", s);
                return ERR_NVMTNV_NOFS; // invalid state
            }
        }
    }
    sys.tags_per_sect = (sect_size - sizeof(tnv_sector_header_t)) / (sizeof(tnv_header_t) + sys.max_value_size);

    if (sys.sect_evict_abort == 0xffffffff && sys.sect_erase_abort == 0xffffffff && sys.sect_spare != 0xffffffff) {
        // all ok, expected
        _dbg("mount sectors consistency ok\n");
        sys.consistency_state = OK;
    }
    else if (sys.sect_evict_abort != 0xffffffff && sys.sect_spare != 0xffffffff && sys.sect_erase_abort == 0xffffffff) {
        // ISSUE 1: found sectors state EVICT and SPARE, and NO ERASING
        // aborted eviction 
        // erase spare sector, mark it as spare and restart eviction 
        sys.consistency_state = EVICTING;
        _dbg("mount sectors consistency aborted eviction\n");
    }
    else
    if (sys.sect_evict_abort != 0xffffffff && sys.sect_erase_abort != 0xffffffff && sys.sect_spare == 0xffffffff) {
        // ISSUE1.5: found sectors state EVICT and ERASING, and NO SPARE
        // aborted eviction, and aborted fix of ISSUE1
        // reerase erasing sector, mark it as spare again and restart eviction
        sys.consistency_state = EVICTING_FIXING;
        _dbg("mount sectors consistency aborted fix of aborted eviction\n");
    }
    else
    if (sys.sect_evict_abort != 0xffffffff && sys.sect_erase_abort == 0xffffffff && sys.sect_spare == 0xffffffff) {
        // ISSUE 2: found sector state EVICT, and NO ERASING or SPARE
        // eviction done, but evicted sector not erased and set to spare
        // erase evicted sector and set as spare
        sys.consistency_state = ERASE;
        _dbg("mount sectors consistency aborted evict cleanup\n");
    }
    else
    if (sys.sect_evict_abort == 0xffffffff && sys.sect_spare == 0xffffffff && sys.sect_erase_abort != 0xffffffff) {
        // ISSUE 3: found sector state ERASING, and NO EVICT or SPARE
        // eviction done, abort during erase of evicted sector
        // reerase erasing sector and set as spare
        sys.consistency_state = ERASING;
        _dbg("mount sectors consistency aborted evict erase\n");
    } else {
        _dbg("mount nofs unknown state\n");
        return ERR_NVMTNV_NOFS; // invalid state
    }

    if (sys.consistency_state == OK) {
        res = check_tag_consistency();
        if (res < 0) return res;
    }

    sys.mount_state = MOUNTED;

    return 0;
}

int nvmtnv_read(uint16_t tag, uint8_t *dst, uint8_t max_size) {
    int res;
    tnv_header_t thdr;
    uint32_t tagloc;
    if ((res = state_ok())) return res;
    res = get_valid_tag(tag, &tagloc, &thdr);
    if (res < 0) return res;
    uint8_t size = max_size < thdr.size ? max_size : thdr.size;
    res = flash_read(SECTOR(tagloc),
        sizeof(tnv_sector_header_t) + TAGIX(tagloc)*(sizeof(tnv_header_t) + sys.max_value_size) + sizeof(tnv_header_t),
        dst, size);
    if (res < 0) return res;
    return size;
}

int nvmtnv_write(uint16_t tag, const uint8_t *src, uint8_t size) {
    int res;
    tnv_header_t thdr;
    uint32_t tagloc;
    if ((res = state_ok())) return res;
    res = get_valid_tag(tag, &tagloc, &thdr);
    if (res == ERR_NVMTNV_NOENT) {
        res = 0;
        tagloc = TAGLOC_NONE;
    } else if (res < 0) {
        return res;
    }

    // check if gc needed
    char do_gc = 0;
    if (tagloc == TAGLOC_NONE) {
        _dbg("write new tag %04x free:%d dele:%d\n", tag, sys.free, sys.dele);
        if (sys.free + sys.dele <= 1) {
            // always keep one slot for being able to modify tags in a full system
            return ERR_NVMTNV_FULL;
        } else if (sys.free <= 1) {
            do_gc = 1;
        }

    } else {
        _dbg("write upd tag %04x free:%d dele:d\n", tag, sys.free, sys.dele);
        if (sys.free + sys.dele == 0) {
            return ERR_NVMTNV_FULL;
        } else if (sys.free == 0) {
            do_gc = 1;
        }
    }
    if (do_gc) {
        _dbg("gc needed\n");
        res = gc();
        if (res >= 0 && tagloc != TAGLOC_NONE) {
            // find the original tag once again, might have moved
            res = get_valid_tag(tag, &tagloc, &thdr);
        }
        if (res < 0) return res;
    } else {
        _dbg("no gc needed\n");
    }

    if (size > sys.max_value_size) size = sys.max_value_size;
    thdr.tag = tag;
    res = update_tag(&thdr, tagloc, TAGLOC_NONE, src, size);
    if (res < 0) return res;
    return size;
}

int nvmtnv_delete(uint16_t tag) {
    int res;
    tnv_header_t thdr;
    uint32_t tagloc;
    if ((res = state_ok())) return res;
    for (;;) {
        res = get_valid_tag(tag, &tagloc, &thdr);
        if (res == ERR_NVMTNV_NOENT) return 0;
        if (res < 0) return res;
        _dbg("delete tag %04x @ tl %08x\n", thdr.tag, tagloc);
        thdr.state = STATE_TAG_DELETED;
        res = tag_write_hdr(tagloc, &thdr);
        if (res < 0) return res;
        sys.dele++;
    }
    return res;
}

int nvmtnv_size(uint16_t tag) {
    int res;
    tnv_header_t thdr;
    uint32_t tagloc;
    if ((res = state_ok())) return res;
    res = get_valid_tag(tag, &tagloc, &thdr);
    if (res < 0) return res;
    return thdr.size;
}

int nvmtnv_gc(void) {
    int res;
    if ((res = state_ok())) return res;
    return gc();
}

static int remake_spare_sector(void) {
    int res;
    res = flash_erase(sys.sect_spare);
    if (res) return res;
    tnv_sector_header_t shdr = {
        .magic = MAGIC,
        .fill = 0xff,
        .max_value_size = sys.max_value_size,
        .sect_nbr = sys.sect_spare - sys.sect_start,
        .sect_count = sys.sect_count,
        .state = STATE_SECT_SPARE
    };
    res = flash_write(sys.sect_spare, 0, (uint8_t *)&shdr, sizeof(shdr));
    return res;
}

int nvmtnv_fix(void) {
    int res;
    if (sys.mount_state != MOUNTED) return ERR_NVMTNV_MOUNT;
    if (sys.consistency_state == OK) return 0;

    switch (sys.consistency_state) {
        case EVICTING: 
        case EVICTING_FIXING: {
            if (sys.consistency_state == EVICTING_FIXING) {
                sys.sect_spare = sys.sect_erase_abort;
            }
            res = remake_spare_sector();
            if (res) return res;
            res = evict(sys.sect_spare, sys.sect_evict_abort);
            if (res) return res;
        }
        break;
        case ERASE:
        case ERASING:
            if (sys.consistency_state == ERASE) {
                sys.sect_spare = sys.sect_evict_abort;
            } else {
                sys.sect_spare = sys.sect_erase_abort;
            }
            res = remake_spare_sector();
            if (res) return res;
        break;
        default:
        break;
    }

    res = check_tag_consistency();
    if (res < 0) return res;
    sys.consistency_state = OK;
    return 0;
}

int nvmtnv_format(uint32_t sector_start, uint16_t sector_count, uint8_t max_value_size) {
    if (sector_count < 2) return ERR_NVMTNV_INVAL;
    // check that all sectors exist and are of same size
    int prev_sect_size = 0;
    for (uint32_t s = sector_start; s < sector_start + sector_count; s++) {
        int sect_size = flash_get_sector_size(sector_start);
        if (sect_size < 0) return sect_size;
        if (prev_sect_size && prev_sect_size != sect_size) return ERR_NVMTNV_NONUNIFORM;
        prev_sect_size = sect_size;
    }
    // erase all and write header
    int res;
    for (uint32_t s = sector_start; s < sector_start + sector_count; s++) {
        _dbg("format sector %08x\n", s);
        res = flash_erase(s);
        if (res) return res;
        tnv_sector_header_t shdr = {
            .magic = MAGIC,
            .fill = 0xff,
            .max_value_size = max_value_size,
            .sect_nbr = s - sector_start,
            .sect_count = sector_count,
            .state = s < sector_start + sector_count - 1 ? STATE_SECT_DATA : STATE_SECT_SPARE
        };
        res = flash_write(s, 0, (uint8_t *)&shdr, sizeof(shdr));
        if (res < 0) return res;
    }
    sys.mount_state = UNMOUNTED;
    return 0;
}
