#ifndef _MINIO_H_
#define _MINIO_H_

// Teeny-weeny implementation of stdlib, stdio, etc

#include <stdarg.h>
#include "types.h"
#include "uart_driver.h"

#define printf(f, ...) do { fprintf(UART_STD, f, ## __VA_ARGS__); } while (0);

void itoa(int v, char *dst, int base);
void v_printf(uint32_t hdl, const char *format, va_list arg_p);
void fprintf(uint32_t hdl, const char *format, ...);
void fprint_mem(uint32_t hdl, uint8_t *data, uint32_t len);
int atoi(const char *s);
int strtol(const char *s, const char **endptr, int base);
int strlen(const char *str);
void *memset(void *p, int v, uint32_t num);
void *memcpy(const void *src, void *dst, uint32_t num);
int memcmp(const void *str1, const void *str2, uint32_t count);
int strcmp(const char *s1, const char *s2);

#endif // _MINIO_H_
