/*
 * serdes.h
 *
 * Simplistic memory to memory serializer/deserializer.
 * Platform (endianess) independent.
 * Same functions apply regardless serializing or deserializing. This means
 * source and target can use the same code for sending/receiving structs.
 *
 *  Created on: Oct 11, 2018
 *      Author: petera
 */

#ifndef _SERDES_H_
#define _SERDES_H_

#include <stdint.h>
#ifndef _serdes_memcpy
#include <string.h>
#endif

typedef struct serdes_ctx {
  union {
    uint8_t *wr_ptr;
    const uint8_t *rd_ptr;
  };
  uint32_t len;
  uint32_t offs;
  enum {
    SD_CTX_READ,
    SD_CTX_WRITE
  } state;
} serdes_ctx_t;

#define SERDES_LENGTH_NO_BOUNDS     (0xffffffff)

#define SERDES_OK                   (0)
#define SERDES_ERR_ARG              (-1)
#define SERDES_ERR_BOUNDS           (-2)

#ifndef _serdes_memcpy
#define _serdes_memcpy(d,s,l) memcpy((d),(s),(l))
#endif

/**
 * Initializes a serdes context for deserialization, i.e. reading from memory.
 * @param ctx       an uninitialized context
 * @param data      the data to deserialize
 * @param data_len  the length of data to deserialize, or
 *                  SERDES_LENGTH_NO_BOUNDS to skip length checks
 */
void serdes_init_context_read(serdes_ctx_t *ctx, const uint8_t *data, uint32_t data_len);

/**
 * Initializes a serdes context for serialization, i.e. writing to memory.
 * @param ctx       an uninitialized context
 * @param buffer    the buffer where serialized data is placed
 * @param buf_len   maximum size of buffer, or
 *                  SERDES_LENGTH_NO_BOUNDS to skip length checks
 */
void serdes_init_context_write(serdes_ctx_t *ctx, uint8_t *buffer, uint32_t buf_len);

/**
 * Serializes x to memory or deserializes memory into x, uint8_t
 */
int serdes_codec_u8(serdes_ctx_t *ctx, uint8_t *x);

/**
 * Serializes x to memory or deserializes memory into x, uint16_t
 */
int serdes_codec_u16(serdes_ctx_t *ctx, uint16_t *x);

/**
 * Serializes x to memory or deserializes memory into x, uint32_t
 */
int serdes_codec_u32(serdes_ctx_t *ctx, uint32_t *x);

/**
 * Serializes x to memory or deserializes memory into x, blob
 */
int serdes_codec_blob(serdes_ctx_t *ctx, uint8_t *x, uint32_t len);

/**
 * Returns current offset
 */
int serdes_get_offset(serdes_ctx_t *ctx);

#endif /* _SERDES_H_ */
