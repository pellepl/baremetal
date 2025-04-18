/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "minio.h"

#ifndef MINIO_STD_API_REPLACE

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
// generates string ascii as capitals
#define NUM_FLAG_CAPITALS           (1<<5)

#ifndef MINIO_MAX_HDL_B4_MEM
#define MINIO_MAX_HDL_B4_MEM        (0x100)
#endif // MINIO_MAX_HDL_B4_MEM

unsigned int minio_putchar(unsigned int hdl, char c) {
    if (hdl >= MINIO_MAX_HDL_B4_MEM)
        *(((char *)(intptr_t)hdl++)) = c;
    else {
#ifdef MINIO_PUTCHAR
        MINIO_PUTCHAR(hdl, c);
#else
#  ifdef CONFIG_UART
        uart_putchar(hdl, c);
#  else
#    warning Please define macro MINIO_PUTCHAR(handle, character)
#  endif
#endif
    }
    return hdl;
}

static unsigned int print_string(unsigned int hdl, const char *str, unsigned int end) {
    char cc;
    if (end == 0) {
        while ((cc = *str++) != 0) {
            hdl = minio_putchar(hdl, cc);
        }
    } else {
        while ((cc = *str++) != 0) {
            hdl = minio_putchar(hdl, cc);
            if (hdl >= MINIO_MAX_HDL_B4_MEM && hdl >= end) break;
        }
    }
    return hdl;
}

static int _isspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int u_atoin(const char *str, const char **endptr, int base) {
    uint8_t flags = 0;
    while (_isspace(*str)) {
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
            }
            else if (*str == 'b' || *str == 'B') {
                base = 2;
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

char *minio_itoa(int v, char *dst, int base) {
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

int minio_atoi(const char *s) {
    return u_atoin(s, 0, 0);
}

long minio_strtol(const char *s, char **endptr, int base) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    return u_atoin(s, (char **)endptr, base);
#pragma GCC diagnostic pop
}

int minio_vn_printf(unsigned int hdl, const char *format, unsigned int count, va_list arg_p) {
    char c;
    char buf[32*2 + 4];
    int esc = 0;
    int numerator = 0;
    int flen = 0;
#   ifdef MINIO_FORMAT_FLOAT
    int fnumerator = 0;
    int fraclen = 3;
#   endif
    uint8_t flags = 0;
    unsigned int ohdl = hdl;
    if (hdl >= MINIO_MAX_HDL_B4_MEM && count) count += hdl;
    while ((c = *format++) != 0) {
        if (esc) {
#           ifdef MINIO_FORMAT_FLOAT
            if (fnumerator) {
                if (c >= '0' && c <= '9') {
                    fraclen = fraclen * 10 + (c - '0');
                    continue; // numerator, goto next char
                } else {
                    // non numerator, back to escape mode
                    if ((uint32_t)fraclen > sizeof(buf) - 1) {
                        fraclen = sizeof(buf) - 1;
                    }
                    fnumerator = 0;
                }
            } else
#           endif
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
#           ifdef MINIO_FORMAT_FLOAT
            case '.':
                fraclen = 0;
                fnumerator = 1;
                continue; // jump to next char, expect fractional numerator
#           endif
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
            case '%': hdl = minio_putchar(hdl, '%'); break;
            case 'c': hdl = minio_putchar(hdl, (char)va_arg(arg_p, int)); break;
            case 's': hdl = print_string(hdl, va_arg(arg_p, char *), count); break;

            case 'd':
            case 'i': {
                int v = va_arg(arg_p, int);
                if (v < 0) {
                    v = -v;
                    flags |= NUM_FLAG_NEGATE;
                }
                u_itoan(v, &buf[0], 10, flen, flags);
                hdl = print_string(hdl, buf, count);
                break;
            }
            case 'u': {
                uint32_t v = va_arg(arg_p, uint32_t);
                u_itoan(v, &buf[0], 10, flen, flags);
                hdl = print_string(hdl, buf, count);
                break;
            }
            case 'X':
                flags |= NUM_FLAG_CAPITALS;
                // fall through
                //no break
            case 'x': {
                uint32_t v = va_arg(arg_p, uint32_t);
                u_itoan(v, &buf[0], 16, flen, flags);
                hdl = print_string(hdl, buf, count);
                break;
            }
#           ifdef MINIO_FORMAT_FLOAT
            case 'f': {
                double v = va_arg(arg_p, double);
                if (v < 0) {
                    v = -v;
                    flags |= NUM_FLAG_NEGATE;
                }
                unsigned long mul = 1;
                for (int i = 0; i < fraclen; i++) mul *= 10;
                int whole = (int)v;
                double tmp = (v - whole) * mul;
                unsigned long frac = (unsigned long)tmp;
                double diff = tmp - frac;
                if (diff > 0.5) {
                    ++frac;
                    // handle rollover, e.g. case 0.99 with prec 1 is 1.0
                    if (frac >= mul) {
                        frac = 0;
                        ++whole;
                    }
                } else if ((diff == 0.5) && ((frac == 0U) || (frac & 1U))) {
                    // if halfway, round up if odd, OR if last digit is 0
                    ++frac;
                }

                u_itoan(whole, &buf[0], 10, flen, flags);
                int ilen = minio_strlen(&buf[0]);
                buf[ilen] = '.';
                u_itoan(frac, &buf[ilen+1], 10, fraclen, 0);
                hdl = print_string(hdl, buf, minio_strlen(&buf[0]));
                break;
            }
#           endif
            default: hdl = minio_putchar(hdl, '?'); break;
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
            hdl = minio_putchar(hdl, c);
        }
        if (count && hdl >= MINIO_MAX_HDL_B4_MEM && hdl >= count) {
            break;
        }
    }
    if (hdl >= MINIO_MAX_HDL_B4_MEM){
        hdl = minio_putchar(hdl, 0);
    }
    return hdl - ohdl;
}

void minio_fprintf(unsigned int hdl, const char *format, ...) {
    va_list arg_p;
    va_start(arg_p, format);
    minio_vn_printf(hdl, format, 0, arg_p);
    va_end(arg_p);
}

int minio_sprintf(char *s, const char *format, ...) {
    unsigned int hdl = (intptr_t)s;
    va_list arg_p;
    va_start(arg_p, format);
    unsigned int ret = minio_vn_printf(hdl, format, 0, arg_p);
    va_end(arg_p);
    return ret;
}

int minio_snprintf(char *s, unsigned int n, const char *format, ...) {
    if (n == 0) return 0;
    unsigned int hdl = (intptr_t)s;
    va_list arg_p;
    va_start(arg_p, format);
    int ret = minio_vn_printf(hdl, format, n-1, arg_p);
    va_end(arg_p);
    return ret;
}

void minio_fprint_mem(unsigned int hdl, uint8_t *data, unsigned int len) {
    while (len) {
        uint32_t rem = len >= 16 ? 16 : len;
        uint32_t fill = 16-rem;
        uint32_t i;
        for (i = 0; i < rem; i++) {
            minio_fprintf(hdl, "%02x ", data[i]);
        }
        for (i = 0; i < fill+1; i++) {
            minio_fprintf(hdl, "   ");
        }
        for (i = 0; i < rem; i++) {
            uint8_t c = data[i];
            if (c <= ' ' || c > 127) c = '.';
            minio_fprintf(hdl, "%c", c);
        }
        data += rem;
        len -= rem;
        minio_fprintf(hdl, "\n");
    }
}

void *minio_memset(void *p, int v, unsigned int num) {
    uint8_t *pp = (uint8_t *)p;
    while (num--) {
        *pp++ = v;
    }
    return p;
}

void *minio_memcpy(void *dst, const void *src, unsigned int num)
{
    const uint8_t *psrc = (const uint8_t *)src;
    uint8_t *pdst = (uint8_t *)dst;
    while (num--) {
        *pdst++ = *psrc++;
    }
    return dst;
}

unsigned int minio_strlen(const char *str) {
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int minio_strcmp(const char* s1, const char* s2) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    while (*p1 && *p1==*p2) {
        ++p1;
        ++p2;
    }

    return (*p1 > *p2) - (*p2  > *p1);
}

const char *minio_strstr(const char* str, const char* sub) {
    int len = minio_strlen(sub);
    int ix = 0;
    char c;
    while ((c = *str++) != 0 && ix < len) {
        if (c == sub[ix]) {
            ix++;
        } else {
            ix = 0;
        }
    }
    if (ix == len) {
        return (const char *)(str - len - 1);
    } else {
        return 0;
    }
}

char *minio_strncpy(char *dst, const char *src, unsigned int num) {
    char *r = dst;
    while (num-- && (*dst++ = *src++));
    return r;
}

int minio_memcmp(const void *str1, const void *str2, unsigned int count) {
    const unsigned char *s1 = (const unsigned char*)str1;
    const unsigned char *s2 = (const unsigned char*)str2;

    while (count-- > 0) {
        if (*s1++ != *s2++) {
           return s1[-1] < s2[-1] ? -1 : 1;
        }
    }
    return 0;
}

void *minio_memmove(void *dst, const void *src, unsigned int num) {
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

static const char tbl_b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned int minio_base64_enc(char *dst, const char *src, unsigned int num)
{
    unsigned char a,b,c;
    unsigned int olen = 0;
    while (num >= 3) {
        a = *src++;
        b = *src++;
        c = *src++;
        dst[olen++] = tbl_b64[a >> 2];
        dst[olen++] = tbl_b64[((a & 3) << 4) | (b >> 4)];
        dst[olen++] = tbl_b64[((b & 0x0f) << 2) | (c >> 6)];
        dst[olen++] = tbl_b64[(c & 0x3f)];
        num -= 3;
    }
    if (num == 0) return olen;
    a = b = 0;
    if (num == 2) b = src[1];
    if (num >= 1) a = src[0];
    dst[olen++] = tbl_b64[a >> 2];
    dst[olen++] = tbl_b64[((a & 3) << 4) | (b >> 4)];
    if (num == 1) return olen;
    dst[olen++] = tbl_b64[((b & 0x0f) << 2)];
    return olen;
}

static unsigned char _b64_ctoi(char c) {
  if (c >= 'A' && c <= 'Z') return c-'A';
  if (c >= 'a' && c <= 'z') return c-'a'+26;
  if (c >= '0' && c <= '9') return c-'0'+2*26;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

unsigned int minio_base64_dec(char *dst, const char *src, unsigned int num)
{
    unsigned char a,b,c,d;
    unsigned int olen = 0;
    while (num >= 4) {
        a = _b64_ctoi(*src++);
        b = _b64_ctoi(*src++);
        c = _b64_ctoi(*src++);
        d = _b64_ctoi(*src++);
        dst[olen++] = (a<<2) | (b>>4);
        dst[olen++] = (b<<4) | (c>>2);
        dst[olen++] = (c<<6) | d;
        num -= 4;
    }
    if (num <= 1) return olen;
    a = b = c = 0;
    if (num >= 2) {
        a = _b64_ctoi(*src++);
        b = _b64_ctoi(*src++);
        dst[olen++] = (a<<2) | (b>>4);
    }
    if (num == 2) return olen;
    c = _b64_ctoi(*src++);
    dst[olen++] = (b<<4) | (c>>2);
    return olen;
}


#endif // MINIO_STD_API_REPLACE
