#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include "bmtypes.h"

#ifndef RB_MEMCPY
#if CONFIG_MINIO
#include "minio.h"
#endif
#define RB_MEMCPY(_dst, _src, _len) memcpy((_dst), (_src), (_len))
#endif

typedef struct {
    uint8_t *buffer;
    volatile uint16_t r_ix;
    volatile uint16_t w_ix;
    uint16_t max_len;
} ringbuffer_t;

/* Initiates a ring buffer */
void ringbuffer_init(ringbuffer_t *rb, uint8_t *buffer, uint16_t max_len);
/* Returns a character from ringbuffer
   @returns 0 if OK, else -1 if empty
 */
int ringbuffer_getc(ringbuffer_t *rb, uint8_t *c);
/* Puts a character into ringbuffer
   @returns 0 if OK, else -1 if full
 */
int ringbuffer_putc(ringbuffer_t *rb, uint8_t c);
/* Returns a region of data from ringbuffer
   @returns number of actual bytes returned or -1 if empty
 */
int ringbuffer_get(ringbuffer_t *rb, uint8_t *buf, uint16_t len);
/* Writes a region of data into ringbuffer.
   @param buf can be null, whereas the read pointer is simply advanced
   @returns number of actual bytes written or -1 if full
 */
int ringbuffer_put(ringbuffer_t *rb, uint8_t *buf, uint16_t len);
/* Returns current write capacity of ringbuffer */
int ringbuffer_free(ringbuffer_t *rb);
/* Returns current read capacity of ringbuffer */
int ringbuffer_available(ringbuffer_t *rb);
/* Returns linear read capacity and pointer to buffer.
   The read pointer can be advanced by calling ringbuffer_get with
   null argument for buf.
 */
int ringbuffer_available_linear(ringbuffer_t *rb, uint8_t **ptr);
/*  Empties ringbuffer  */
int ringbuffer_clear(ringbuffer_t *rb);

#endif /* _RINGBUFFER_H_ */
