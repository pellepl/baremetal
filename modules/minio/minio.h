/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#ifndef _MINIO_H_
#define _MINIO_H_

// Teeny-weeny implementation of stdlib, stdio, etc

#include <stdarg.h>
#include "bmtypes.h"
#if CONFIG_UART
#  include "uart_driver.h"
#endif

#ifndef STR
#  define STR(x) _STR(x)
#  define _STR(x) #x
#endif

#ifdef MINIO_CUSTOM_INC
#  include STR(MINIO_CUSTOM_INC)
#endif

#ifdef MINIO_STD_API_REPLACE
#  include <string.h>
#  include <stdio.h>
#else
#  ifdef MINIO_STD_API_COEXIST
#    include <string.h>
#  else
#    define fprintf(i, f, ...) minio_fprintf((i), (f), ## _VA_ARGS__)
#    define printf(f, ...) do { minio_fprintf(UART_STD, f, ## __VA_ARGS__); } while (0);
#    define itoa(v, d, b) minio_itoa((v), (d), (b))
#    define vn_printf(h, f, c, l) minio_vn_printf((h), (f), (c), (l))
#    define sprintf(s, f, ...) minio_sprintf((s), (f), ## __VA_ARGS__)
#    define snprintf(s, n, f, ...) minio_snprintf((s), (n), (f), ## __VA_ARGS__)
#    define fprint_mem(h, d, l) minio_fprint_mem((h), (d), (l))
#    define atoi(s) minio_atoi((s))
#    define strtol(s, e, b) minio_strtol((s),(e),(b))
#    define strstr(s, t) minio_strstr((s),(t))
#    define strlen(s) minio_strlen((s))
#    define memset(p,v,n) minio_memset((p),(v),(n))
#    define memcpy(d,s,n) minio_memcpy((d),(s),(n))
#    define memcmp(s,t,c) minio_memcmp((s),(t),(c))
#    define strcmp(s,t) minio_strcmp((s),(t))
#    define strncpy(d,s,n) minio_strncpy((d),(s),(n))
#    define memmove(d,s,n) minio_memmove((d),(s),(n))
#  endif // !MINIO_STD_API_COEXIST


char *minio_itoa(int v, char *dst, int base);
int minio_vn_printf(unsigned int hdl, const char *format, unsigned int count, va_list arg_p);
void minio_fprintf(unsigned int hdl, const char *format, ...);
int minio_sprintf(char *s, const char *format, ...);
int minio_snprintf(char *s, unsigned int n, const char *format, ...);
void minio_fprint_mem(unsigned int hdl, uint8_t *data, unsigned int len);
int minio_atoi(const char *s);
long minio_strtol(const char *s, char **endptr, int base);
unsigned int minio_strlen(const char *str);
void *minio_memset(void *p, int v, unsigned int num);
void *minio_memcpy(void *dst, const void *src, unsigned int num);
int minio_memcmp(const void *str1, const void *str2, unsigned int count);
int minio_strcmp(const char *s1, const char *s2);
char *minio_strncpy(char *dst, const char *src, unsigned int num);
void *minio_memmove(void *dst, const void *src, unsigned int num);
const char *minio_strstr(const char *s1, const char *s2);
// override this if wanted, defaults to uart_putchar, return hdl
unsigned int minio_putchar(unsigned int hdl, char c) __attribute__ ((weak));
#endif // MINIO_STD_API_REPLACE
#endif // _MINIO_H_
