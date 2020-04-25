/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "minio.h"

static const char *I_BASE_ARR_L = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char *I_BASE_ARR_U = "0123456789ABCDEFGHIJKLMNOPQRTSUVWXYZ";

// number is negative
#define NUM_FLAG_NEGATE             (1<<0)
// omits ending string with a zero
#define NUM_FLAG_NO_ZERO_END        (1<<1)
// fills zero prefixes with ' '
#define NUM_FLAG_FILL_SPACE         (1<<2)
// forces a '+' or a '-' before string
#define NUM_FLAG_FORCE_SIGN         (1<<3)
// adds base signature, e.g. "0x", "0b", etc
#define NUM_FLAG_BASE_SIG           (1<<4)
// generates string ascii as captials
#define NUM_FLAG_CAPITALS           (1<<5)

void minio_putchar(unsigned int hdl, char c) {
    uart_putchar(hdl, c);
}

static void print_string(uint32_t hdl, const char *str) {
    char cc;
    while ((cc = *str++) != 0) {
        minio_putchar(hdl, cc);
    }
}

static int isspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int u_atoin(const char *str, const char **endptr, int base) {
    uint8_t flags = 0;
    while (isspace(*str)) {
        str++;
    }
    if (*str == '-') {
        flags = NUM_FLAG_NEGATE;
        str++;
    } else if (*str == '+') {
        str++;
    }
    if (base == 0) {
        if (*str == '0') {
            str++;
            if (*str == 'x' || *str == 'X') {
                base = 16;
                str++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    }

    uint32_t value = 0;
    char c;
    while ((c = *str++) != 0) {
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c-'0';
        } else if (c >= 'A' && c <= 'Z') {
            digit = c-'A'+10;
        } else if (c >= 'a' && c <= 'z') {
            digit = c-'a'+10;
        } else {
            break;
        }
        if (digit >= base) {
            break;
        }
        value = (value*base) + digit;
    }

    if (endptr) {
        *endptr = str;
    }
    return flags == NUM_FLAG_NEGATE ? -value : value;
}

static void u_itoan(uint32_t v, char *dst, int base, int num, int flags) {
    // check that the base is valid
    if (base < 2 || base > 36) {
        if ((flags & NUM_FLAG_NO_ZERO_END) == 0) {
            *dst = '\0';
        }
        return;
    }

    const char *arr = (flags & NUM_FLAG_CAPITALS) ? I_BASE_ARR_U : I_BASE_ARR_L;
    char *ptr = dst, *ptr_o = dst, tmp_char;
    int tmp_value;
    int ix = 0;
    char zero_char = flags & NUM_FLAG_FILL_SPACE ? ' ' : '0';
    do {
        tmp_value = v;
        v /= base;
        if (tmp_value != 0 || (tmp_value == 0 && ix == 0)) {
            *ptr++ = arr[(tmp_value - v * base)];
        } else {
            *ptr++ = zero_char;
        }
        ix++;
    } while ((v && num == 0) || (num != 0 && ix < num));

    if (flags & NUM_FLAG_BASE_SIG) {
        if (base == 16) {
            *ptr++ = 'x';
            *ptr++ = '0';
        } else if (base == 8) {
            *ptr++ = '0';
        } else if (base == 2) {
            *ptr++ = 'b';
        }
    }

    // apply sign
    if (flags & NUM_FLAG_NEGATE) {
        *ptr++ = '-';
    } else if (flags & NUM_FLAG_FORCE_SIGN) {
        *ptr++ = '+';
    }

    // end
    if (flags & NUM_FLAG_NO_ZERO_END) {
        ptr--;
    } else {
        *ptr-- = '\0';
    }
    // reverse the whole string
    while (ptr_o < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr_o;
        *ptr_o++ = tmp_char;
    }
}

char *itoa(int v, char *dst, int base) {
    if (base == 10) {
        if (v < 0) {
            u_itoan(-v, dst, base, 0, NUM_FLAG_NEGATE);
        } else {
            u_itoan(v, dst, base, 0, 0);
        }
    } else {
        u_itoan((unsigned int)v, dst, base, 0, 0);
    }
    return dst;
}

int atoi(const char *s) {
    return u_atoin(s, 0, 0);
}

long strtol(const char *s, char **endptr, int base) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    return u_atoin(s, (char **)endptr, base);
#pragma GCC diagnostic pop
}

void v_printf(unsigned int hdl, const char *format, va_list arg_p) {
    char c;
    char buf[32*2 + 4];
    int esc = 0;
    int numerator = 0;
    int flen = 0;
    uint8_t flags = 0;
    while ((c = *format++) != 0) {
        if (esc) {
            if (numerator) {
                if (c >= '0' && c <= '9') {
                    flen = flen * 10 + (c - '0');
                    continue; // numerator, goto next char
                } else {
                    // non numerator, back to escape mode
                    if ((uint32_t)flen > sizeof(buf) - 1) {
                        flen = sizeof(buf) - 1;
                    }
                    numerator = 0;
                }
            }
            switch (c) {
            case '+':
                flags |= NUM_FLAG_FORCE_SIGN;
                continue; // jump to next char
            case '#':
                flags |= NUM_FLAG_BASE_SIG;
                continue; // jump to next char
            case '0': 
                flen = 0;
                numerator = 1;
                continue; // jump to next char, expect numerator
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                flags |= NUM_FLAG_FILL_SPACE;
                flen = c - '0';
                numerator = 1;
                continue; // jump to next char, expect numerator
            case '%': minio_putchar(hdl, '%'); break;
            case 'c': minio_putchar(hdl, (char)va_arg(arg_p, int)); break;
            case 's': print_string(hdl, va_arg(arg_p, char *)); break;

            case 'd':
            case 'i': {
                int v = va_arg(arg_p, int);
                if (v < 0) {
                    v = -v;
                    flags |= NUM_FLAG_NEGATE;
                }
                u_itoan(v, &buf[0], 10, flen, flags);
                print_string(hdl, buf);
                break;
            }
            case 'u': {
                uint32_t v = va_arg(arg_p, uint32_t);
                u_itoan(v, &buf[0], 10, flen, flags);
                print_string(hdl, buf);
                break;
            }
            case 'X':
                flags |= NUM_FLAG_CAPITALS;
                // fall through
                //no break
            case 'x': {
                uint32_t v = va_arg(arg_p, uint32_t);
                u_itoan(v, &buf[0], 16, flen, flags);
                print_string(hdl, buf);
                break;
            }
            default: minio_putchar(hdl, '?'); break;
            }
            // turn off escape mode
            esc = 0;
        } else {
            if (c == '%') {
                flags = 0;
                numerator = 0;
                flen = 0;
                esc = 1;
                continue;
            }
            minio_putchar(hdl, c);
        }
    }
}

void fprintf(unsigned int hdl, const char *format, ...) {
    va_list arg_p;
    va_start(arg_p, format);
    v_printf(hdl, format, arg_p);
    va_end(arg_p);
}

void fprint_mem(unsigned int hdl, uint8_t *data, unsigned int len) {
    while (len) {
        uint32_t rem = len >= 16 ? 16 : len;
        uint32_t fill = 16-rem;
        uint32_t i;
        for (i = 0; i < rem; i++) {
            fprintf(hdl, "%02x ", data[i]);
        }
        for (i = 0; i < fill+1; i++) {
            fprintf(hdl, "   ");
        }
        for (i = 0; i < rem; i++) {
            uint8_t c = data[i];
            if (c <= ' ' || c > 127) c = '.';
            fprintf(hdl, "%c", c);
        }
        data += rem;
        len -= rem;
        fprintf(hdl, "\n");
    }
}

void *memset(void *p, int v, unsigned int num) {
    uint8_t *pp = (uint8_t *)p;
    while (num--) {
        *pp++ = v;
    }
    return p;
}

void *memcpy(void *dst, const void *src, unsigned int num)
{
    const uint8_t *psrc = (const uint8_t *)src;
    uint8_t *pdst = (uint8_t *)dst;
    while (num--) {
        *pdst++ = *psrc++;
    }
    return dst;
}

unsigned int strlen(const char *str) {
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int strcmp(const char* s1, const char* s2) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (*p1 && *p1==*p2) {
        ++p1;
        ++p2;
    }

    return (*p1 > *p2) - (*p2  > *p1);
}

char *strncpy(char *dst, const char *src, unsigned int num) {
    char *r = dst;
    while (num-- && (*dst++ = *src++));
    return r;
}

int memcmp(const void *str1, const void *str2, unsigned int count) {
    const unsigned char *s1 = (const unsigned char*)str1;
    const unsigned char *s2 = (const unsigned char*)str2;

    while (count-- > 0) {
        if (*s1++ != *s2++) {
           return s1[-1] < s2[-1] ? -1 : 1;
        }
    }
    return 0;
}

void *memmove(void *dst, const void *src, unsigned int num) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (num--) *d++ = *s++;
    } else {
      const uint8_t *ls = s + (num-1);
      uint8_t *ld = d + (num-1);
      while (num--) *ld-- = *ls--;
    }
    return dst;
}

