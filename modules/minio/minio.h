/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _MINIO_H_
#define _MINIO_H_

// Teeny-weeny implementation of stdlib, stdio, etc

#include <stdarg.h>
#include "types.h"
#include "uart_driver.h"

#define printf(f, ...) do { fprintf(UART_STD, f, ## __VA_ARGS__); } while (0);


char *itoa(int v, char *dst, int base);
int vn_printf(unsigned int hdl, const char *format, unsigned int count, va_list arg_p);
void fprintf(unsigned int hdl, const char *format, ...);
int sprintf(char *s, const char *format, ...);
int snprintf(char *s, unsigned int n, const char *format, ...);
void fprint_mem(unsigned int hdl, uint8_t *data, unsigned int len);
int atoi(const char *s);
long strtol(const char *s, char **endptr, int base);
unsigned int strlen(const char *str);
void *memset(void *p, int v, unsigned int num);
void *memcpy(void *dst, const void *src, unsigned int num);
int memcmp(const void *str1, const void *str2, unsigned int count);
int strcmp(const char *s1, const char *s2);
char *strncpy(char *dst, const char *src, unsigned int num);
void *memmove(void *dst, const void *src, unsigned int num);
// override this if wanted, defaults to uart_putchar, return hdl
unsigned int minio_putchar(unsigned int hdl, char c) __attribute__ ((weak));

#endif // _MINIO_H_

