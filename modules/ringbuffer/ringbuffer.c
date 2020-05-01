#include "ringbuffer.h"

#define RB_AVAIL(rix, wix) \
    (wix >= rix ? (wix - rix) : (rb->max_len - (rix - wix)))
#define RB_FREE(rix, wix) \
    (rb->max_len - (wix >= rix ? (wix - rix) : (rb->max_len - (rix - wix))) - 1)
#define RB_EMPTY(rix, wix) \
    (rix == wix)
#define RB_FULL(rix, wix) \
    ((rix == 0 && wix == rb->max_len-1) || wix == rix-1)

void ringbuffer_init(ringbuffer_t *rb, uint8_t *buffer, uint16_t max_len) {
    rb->max_len = max_len;
    rb->buffer = buffer;
    rb->r_ix = 0;
    rb->w_ix = 0;
}

int ringbuffer_getc(ringbuffer_t *rb, uint8_t *c) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;
    if (RB_EMPTY(rix, wix)) {
        return -1;
    }
    if (c) {
        *c = rb->buffer[rix];
    }
    if (rix >= rb->max_len-1) {
        rix = 0;
    } else {
        rix++;
    }
    rb->r_ix = rix;

    return 0;
}

int ringbuffer_putc(ringbuffer_t *rb, uint8_t c) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;
    if (RB_FULL(rix, wix)) {
        return -1;
    }
    rb->buffer[wix] = c;
    if (wix >= rb->max_len-1) {
        wix = 0;
    } else {
        wix++;
    }
    rb->w_ix = wix;

    return 0;
}

int ringbuffer_available(ringbuffer_t *rb) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;
    return RB_AVAIL(rix, wix);
}

int ringbuffer_clear(ringbuffer_t *rb) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;
    uint16_t avail = RB_AVAIL(rix, wix);
    rb->r_ix = 0;
    rb->w_ix = 0;
    return avail;
}

int ringbuffer_available_linear(ringbuffer_t *rb, uint8_t **ptr) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;

    if (rix == wix) {
        return 0;
    } else {
        uint16_t avail = RB_AVAIL(rix, wix);
        *ptr = &rb->buffer[rix];

        if (rix + avail < rb->max_len) {
            return avail;
        } else {
            return rb->max_len - rix;
        }
    }
}

int ringbuffer_free(ringbuffer_t *rb) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;
    return RB_FREE(rix, wix);
}

int ringbuffer_put(ringbuffer_t *rb, uint8_t *buf, uint16_t len) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;
    int to_write;
    if (RB_FULL(rix, wix)) {
        return -1;
    }
    uint16_t free = RB_FREE(rix, wix);
    if (len > free) {
        len = free;
    }
    to_write = len;
    if (wix + len >= rb->max_len) {
        uint16_t part = rb->max_len - wix;
        RB_MEMCPY(&rb->buffer[wix], buf, part);
        buf +=part;
        to_write -= part;
        wix = 0;
    }
    if (to_write > 0) {
        RB_MEMCPY(&rb->buffer[wix], buf, to_write);
        wix += to_write;
    }
    if (wix >= rb->max_len) {
        wix = 0;
    }
    rb->w_ix = wix;

    return len;
}

int ringbuffer_get(ringbuffer_t *rb, uint8_t *buf, uint16_t len) {
    uint16_t rix = rb->r_ix;
    uint16_t wix = rb->w_ix;
    int to_read;

    if (RB_EMPTY(rix, wix)) {
        return -1;
    }
    uint16_t avail = RB_AVAIL(rix, wix);
    if (len > avail) {
        len = avail;
    }
    to_read = len;
    if (rix + len >= rb->max_len) {
        uint16_t part = rb->max_len - rix;
    if (buf) {
        RB_MEMCPY(buf, &rb->buffer[rix], part);
        buf += part;
    }
    to_read -= part;
    rix = 0;
    }
    if (to_read > 0) {
        if (buf) {
            RB_MEMCPY(buf, &rb->buffer[rix], to_read);
        }
        rix += to_read;
    }
    if (rix >= rb->max_len) {
        rix = 0;
    }

    rb->r_ix = rix;

    return len;
}
