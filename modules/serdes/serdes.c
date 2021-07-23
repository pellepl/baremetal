/*
 * serdes.c
 *
 *  Created on: Oct 11, 2018
 *      Author: petera
 */

#include "serdes.h"

#define dbg(...)

#define check_sanity(_ctx, _buf, _l) \
    if ((_ctx) == 0 || (_buf) == 0) return SERDES_ERR_ARG; \
    if ((_ctx)->len != SERDES_LENGTH_NO_BOUNDS && \
        (_ctx)->offs + (_l) > (_ctx)->len + 1) return SERDES_ERR_BOUNDS

void serdes_init_context_read(serdes_ctx_t *ctx, const uint8_t *data, uint32_t len)
{
  ctx->state = SD_CTX_READ;
  ctx->rd_ptr = data;
  ctx->len = len;
  ctx->offs = 0;
}

void serdes_init_context_write(serdes_ctx_t *ctx, uint8_t *data, uint32_t len)
{
  ctx->state = SD_CTX_WRITE;
  ctx->wr_ptr = data;
  ctx->len = len;
  ctx->offs = 0;
}

int serdes_codec_u8(serdes_ctx_t *ctx, uint8_t *x)
{
  dbg("serdes eat 1 byte of %d\n", ctx->len - ctx->offs);
  check_sanity(ctx, x, sizeof(uint8_t));
  if (ctx->state == SD_CTX_READ) {
    *x = *ctx->rd_ptr++;
  } else {
    *ctx->wr_ptr++ = *x;
  }
  ctx->offs += sizeof(uint8_t);
  return SERDES_OK;
}

int serdes_codec_u16(serdes_ctx_t *ctx, uint16_t *x)
{
  dbg("serdes eat 2 bytes of %d\n", ctx->len - ctx->offs);
  check_sanity(ctx, x, sizeof(uint16_t));
  if (ctx->state == SD_CTX_READ) {
    *x = (ctx->rd_ptr[0] << 8)  |
          ctx->rd_ptr[1];
    ctx->rd_ptr += 2;
  } else {
    *ctx->wr_ptr++ = (*x) >> 8;
    *ctx->wr_ptr++ = (*x);
  }
  ctx->offs += sizeof(uint16_t);
  return SERDES_OK;
}

int serdes_codec_u32(serdes_ctx_t *ctx, uint32_t *x)
{
  dbg("serdes eat 4 bytes of %d\n", ctx->len - ctx->offs);
  check_sanity(ctx, x, sizeof(uint32_t));
  if (ctx->state == SD_CTX_READ) {
    *x = (ctx->rd_ptr[0] << 24) |
         (ctx->rd_ptr[1] << 16) |
         (ctx->rd_ptr[2] << 8)  |
          ctx->rd_ptr[3];
    ctx->rd_ptr += 4;
  } else {
    *ctx->wr_ptr++ = (*x) >> 24;
    *ctx->wr_ptr++ = (*x) >> 16;
    *ctx->wr_ptr++ = (*x) >> 8;
    *ctx->wr_ptr++ = (*x);
  }
  ctx->offs += sizeof(uint32_t);
  return SERDES_OK;
}

int serdes_codec_blob(serdes_ctx_t *ctx, uint8_t *x, uint32_t len)
{
  dbg("serdes eat %d bytes of %d\n", len, ctx->len - ctx->offs);
  check_sanity(ctx, x, len);
  if (len > 0) {
    if (ctx->state == SD_CTX_READ) {
      _serdes_memcpy(x, ctx->rd_ptr, len);
    } else {
      _serdes_memcpy(ctx->wr_ptr, x, len);
    }
    ctx->wr_ptr += len;
    ctx->offs += len;
  }
  return SERDES_OK;
}

int serdes_get_offset(serdes_ctx_t *ctx)
{
  if (ctx == 0) return SERDES_ERR_ARG;
  return (int)ctx->offs;
}
