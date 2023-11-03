#include <ngx_config.h>
#include <ngx_core.h>


static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, ngx_uint_t hexadecimal, ngx_uint_t width);
static u_char *ngx_sprintf_str(u_char *buf, u_char *last, u_char *src, size_t len, ngx_uint_t hexadecimal);

/**
 * ngx_cpystrn returns a pointer to the end of the copied string, 
 * may stop copying before `n` is reached 
 * if the source strings terminates prematurely with '\0' 
 */
u_char *ngx_cpystrn(u_char *dst, u_char *src, size_t n) {
    if (n == 0) {
        return dst;
    }

    while (--n) {
        *dst = *src;
        if (*dst == '\0') { /* source string terminated before the specified size `n` */
            return dst;
        }

        dst++;
        src++;
    }

    *dst = '\0'; /* terminate string */

    return dst;
}


/**
 * supported formats:
 *    %[0][width][x][X]O        off_t
 *    %[0][width]T              time_t
 *    %[0][width][u][x|X]z      ssize_t/size_t
 *    %[0][width][u][x|X]d      int/u_int
 *    %[0][width][u][x|X]l      long
 *    %[0][width|m][u][x|X]i    ngx_int_t/ngx_uint_t
 *    %[0][width][u][x|X]D      int32_t/uint32_t
 *    %[0][width][u][x|X]L      int64_t/uint64_t
 *    %[0][width|m][u][x|X]A    ngx_atomic_int_t/ngx_atomic_uint_t
 *    %[0][width][.width]f      double, max valid number fits to %18.15f
 *    %P                        ngx_pid_t
 *    %M                        ngx_msec_t
 *    %r                        rlim_t
 *    %p                        void *
 *    %[x|X]V                   ngx_str_t *
 *    %[x|X]v                   ngx_variable_value_t *
 *    %[x|X]s                   null-terminated string
 *    %*[x|X]s                  length and string
 *    %Z                        '\0'
 *    %N                        '\n'
 *    %c                        char
 *    %%                        %
 *
 *  reserved:
 *    %t                        ptrdiff_t
 *    %S                        null-terminated wchar string
 *    %C                        wchar
 */

u_char * ngx_cdecl ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...) {
    u_char   *p; /* pointer to next position in buffer after printing to it */
    va_list   args;

     /**
     * va_start macro
     * =============== 
     *   - will connect our argument list with some argument pointer
     *   - the list specified in va_list is the first argument and the second argument is the last fixed parameter
     */
    va_start(args, fmt);  /* initialize argument position */
    p = ngx_vslprintf(buf, last, fmt, args);
    /**
     * va_end macro
     * ===============
     *   - is used in situations when we would like to stop using the variable argument list (cleanup)
     */
    va_end(args);

    return p;
}

/**
 * ngx_vslprintf:
 *   va_list macro
 *   -------------
 *     - used in situations in which we need to access optional parameters and it's an argument list
 *     - represents a data object used to hold the parameters corresponding to the ellipsis part of the parameter list
 * 
 *   va_arg macro
 *   ------------
 *     - will fetch the current argument connected to the argument list
 *     - we would need to know the type of the argument that we're reading
*/
u_char *ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args) {
    /** 
     * - *p: temporary storage got C string (character array);
     * - zero: the optional [0] part of the format specifier tells whether to use 0s or spaces to pad out a number to the requested width.
     */
    u_char                 *p, zero; 
    int                     d;         /* temporary storage for a single character */
    double                  f;         /* temporary storage for double */
    size_t                 slen;       /* string format specifier additional length parameter */
    int64_t                i64;        /* temporary storage for signed numbers */
    uint64_t               ui64, frac; /* temporary storage for unsigned numbers and fraction part of a double */
    ngx_msec_t             ms;         /* temporary storage for milliseconds (uint64) before they're converted to an int64 or uint64 for printing */
    /**
     * - width: width of a number;
     * - sign: number is signed or unsigned;
     * - hex: convert number to hex or string to hex byte representation (hex will convert the number to unsigned? TODO!!!!!);
     * - max_width: use max width for number representation (width for a 64-bit int representation);
     * - frac_width: width after the decimal point in a double;
     * - scale: 
     * - n:
     */
    ngx_uint_t             width, sign, hex, max_width, frac_width, scale, n; 
    ngx_str_t             *v; /* temporary storage for pointer to nginx string structure consisting of pointer to data and length */
    ngx_variable_value_t  *vv; /* temporary storage for pointer to ngxinx variable value, consisting of 4 1-bit flags, 28-bit length and a pointer to a buffer of bytes TODO!!!!! */

    while (*fmt && buf < last) {

        /**
         * the above check "buf < last" means that we could copy at least one character:
         * the plain character, "%%", "%c", and minus without the checking
         */
        if (*fmt == '%') { /* we've got a format specifier, parse it */
            i64 = 0;
            ui64 = 0;

            /**
             * parse the '[0]' part of the format specifier if any, 
             * zero or spaces for padding out a number to the requested width 
             */
            zero = (u_char) ((*++fmt == '0') ? '0' : ' '); 
            width = 0;                                     /* width of a number */
            sign = 1;                                      /* by default we expect a signed number, only if the format specifier contains 'u' we expect otherwise */   
            hex = 0;
            max_width = 0;
            frac_width = 0;
            slen = (size_t) -1;  

            while (*fmt >= '0' && *fmt <= '9') { /* if this is a number, parse out the specified width */
                width = width * 10 + (*fmt++ - '0');
            }


            for(;;) { /* parse the *, [width|m], [.width], [u], [x|X] parts of the format specifier  */
                switch (*fmt) {

                case 'u':
                    sign = 0; /* 'u' means unsigned */
                    fmt++;    /* go to next character of the format specifier */
                    continue; /* continue parsing the format specifier */

                case 'm':
                    max_width = 1; /* 'm' means use max width for number representation (width for a 64-bit int representation) */
                    fmt++;    /* go to next character of the format specifier */
                    continue; /* continue parsing the format specifier */

                case 'X':
                    hex = 2;  /* 'X' means convert number to upper-case hex or string to upper-case hex byte representation */
                    sign = 0; /* hex number cannot be negative */
                    fmt++;    /* go to next character of the format specifier */
                    continue; /* continue parsing the format specifier */

                case 'x':
                    hex = 1;  /* 'x' means convert number to lower-case hex or string to lower-case hex byte representation */
                    sign = 0; /* hex numbers cannot be negative */
                    fmt++;    /* go to next character of the format specifier */
                    continue; /* continue parsing the format specifier */

                case '.':
                    fmt++; /* skip the decimal point and start parsing out the fractional portion width of a double  */

                    while (*fmt >= '0' && *fmt <= '9') {
                        frac_width = frac_width * 10 + (*fmt++ - '0');
                    }

                    break; /* this is the last possible additional parameter to a double format specifier, proceed to parsing the type part of the specifier */

                case '*': /* string format specifier has 2 parameters: length and the charater array, parse out the length */
                    slen = va_arg(args, size_t); /* get string's length from argument, which should be of type `size_t` */
                    fmt++;    /* go to next character of the format specifier */
                    continue; /* continue parsing the format specifier */

                default: /* end of additional params to the format specifier, proceed to parsing the type part of the specifier */
                    break;
                }

                break; /* we've finished parsing the modifiers of the format specifier, so break out of the loop, and continue parsing the type part */
            } 


            switch (*fmt) { /* parse the type part of the format specifier */

            case 'V': /* we've got a pointer to an nginx string structure consisting of pointer to data and length */
                v = va_arg(args, ngx_str_t *);

                buf = ngx_sprintf_str(buf, last, v->data, v->len, hex);
                fmt++;

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */

            case 'v': /* we've got a pointer to an ngx_variable_value_t structure TODO!!!!! for string? why bit flags? */
                vv = va_arg(args, ngx_variable_value_t *);

                buf = ngx_sprintf_str(buf, last, vv->data, vv->len, hex);
                fmt++;

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */

            case 's': /* we've got a string type format specifier */
                p = va_arg(args, u_char *);

                buf = ngx_sprintf_str(buf, last, p, slen, hex);
                fmt++;    /* go to next character of the format specifier */

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */

            case 'O': /* we've got an `off_t` type format specifier */
                i64 = (int64_t) va_arg(args, off_t);
                sign = 1;
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'P': /* we've got a pid type which is an int */
                i64 = (int64_t) va_arg(args, ngx_pid_t);
                sign = 1;
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'T': /* we've got a time type which is an int64 */
                i64 = (int64_t) va_arg(args, time_t);
                sign = 1;
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'M': /* we've got an nginx milliseconds type which is either an int64 or a uin64*/
                ms = (ngx_msec_t) va_arg(args, ngx_msec_t);
                if ((ngx_msec_int_t) ms == -1) { /* check if should be converted to a signed int64 */
                    sign = 1;
                    i64 = -1;
                } else {
                    sign = 0;
                    ui64 = (uint64_t) ms;
                }
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'z': /* we've got an size_t or ssize_t depending on the 'u' modifier of the format specifier */
                if (sign) {
                    i64 = (int64_t) va_arg(args, ssize_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, size_t);
                }
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'i': /* we've got a signed or unsigned 64-bit integer depending on the 'u' modifier of the format specifier */
                if (sign) {
                    i64 = (int64_t) va_arg(args, ngx_int_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, ngx_uint_t);
                }

                if (max_width) { /* we've got the 'm' modifier of the format specifier, so pad out number to max width */
                    width = NGX_INT_T_LEN;
                }

                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'd': /* we've got either an int or an unsigned int depending on the 'u' modifier of the format specifier */
                if (sign) {
                    i64 = (int64_t) va_arg(args, int);
                } else {
                    ui64 = (uint64_t) va_arg(args, u_int);
                }
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'l': /* we've got either a long or an unsigned long depending on the 'u' modifier of the format specifier */
                if (sign) {
                    i64 = (int64_t) va_arg(args, long);
                } else {
                    ui64 = (uint64_t) va_arg(args, u_long);
                }
                break;  /* we've got a number type, go to printnting the number just after this switch case */

            case 'D': /* we've got either an int32 or an unsigned int32 depending on the 'u' modifier of the format specifier */
                if (sign) {
                    i64 = (int64_t) va_arg(args, int32_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, uint32_t);
                }
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'L': /* we've got either an int64 or an unsigned int64 depending on the 'u' modifier of the format specifier */
                if (sign) {
                    i64 = va_arg(args, int64_t);
                } else {
                    ui64 = va_arg(args, uint64_t);
                }
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'A': /* we've got an nginx atomic which is either a 64-bit integer or unsgined 64-bit integer */
                if (sign) {
                    i64 = (int64_t) va_arg(args, ngx_atomic_int_t);
                } else {
                    ui64 = (uint64_t) va_arg(args, ngx_atomic_uint_t);
                }

                if (max_width) { /* we've got the 'm' modifier of the format specifier, so pad out number to max width */
                    width = NGX_ATOMIC_T_LEN;
                }

                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'f': /* we've got a double */
                f = va_arg(args, double);

                if (f < 0) {
                    *buf++ = '-';
                    f = -f; /* convert to unsigned */
                }

                ui64 = (int64_t) f; /* extract the part before the decimal point */
                frac = 0;

                if (frac_width) { /* width of the fraction part (if any) extracted earlier after encounetring a dot (.) */

                    scale = 1; 
                    for (n = frac_width; n; n--) { /* determine the multiplier to get a uint64 from the fraction part */
                        scale *= 10;
                    }

                    /**
                     * - (f - (double) ui64) - get the fraction part (for example, 0.45);
                     * 
                     * - (f - (double) ui64) * scale - convert fraction part according to scale to a uint64 (for example, from 0.45 * 100 (scale) = 45),
                     *                                 or from 0.45 * 10 (scale) = 4.5;
                     * 
                     * - (uint64_t) and + 0.5 - if width was specified less then the actual fraction width, round the fraction up to the next whole number,
                     *                          0.45789 * 10 (scale) = 4.5789 + 0.5 = 5.0789 = (uint64_t) 5.0789 = 5.
                     *                          In this case the fraction part will be equal to 5 (rounded up)
                     */
                    frac = (uint64_t) ((f - (double) ui64) * scale + 0.5);
                    
                    /**
                     * Example:
                     *    f = 56.999998
                     *    scale = 10 (.width = 1)
                     *    ui64 = 56
                     *    (f - (double) ui64) = 0.999998
                     *    (f - (double) ui64) * scale = 9.99998
                     *    (f - (double) ui64) * scale + 0.5 = 9.99998 + 0.5 = 10.49998
                     *    (uint64_t) ((f - (double) ui64) * scale + 0.5) = 10 (means that by rounding up we got a whole number equal to 1)
                     */
                    if (frac == scale) { 
                        ui64++;
                        frac = 0;
                    }
                }

                buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width); /* write the part before the decimal dot */

                if (frac_width) { /* if there was fraction width specified as part of the format specifier */
                    if (buf < last) { /* if there's place left in buffer */
                        *buf++ = '.'; /* write out the decimal dot before printing the fractional part */
                    }

                    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width); /* write fraction and pad out with zeroes if neccesary */
                }

                fmt++; /* go to next character of the format specifier if any */

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */

            case 'r': /* we've got a system resource limit type (uint64) */
                i64 = (int64_t) va_arg(args, rlim_t);
                sign = 1;
                break; /* we've got a number type, go to printnting the number just after this switch case */

            case 'p': /* we've got a void pointer, which is an address */
                ui64 = (uintptr_t) va_arg(args, void *);
                hex = 2;                    /* address should be displayed in hex (upper-case) */
                sign = 0;                   /* address cannot be negative */
                zero = '0';                 /* address should be padded out with zeroes */
                width = 2 * sizeof(void *); /* 16 characters each of which is a nibble, so together for 8 bytes to represent an address */
                break; /* we've got a number type, go to printnting the number just after this switch case */
                
            case 'c': /* we've got a character */
                d = va_arg(args, int);
                *buf++ = (u_char) (d & 0xff); /* mask off only the lower byte (8 bits), write character to buffer */
                fmt++; /* go to next byte */

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */

            case 'Z': /* we've got a zero character format specifier */
                *buf++ = '\0';
                fmt++;
                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */
            case 'N': /* we've got a newline format specifier */
                *buf++ = LF;
                fmt++;

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */

            case '%': /* we've got an escape of the '%' character, just copy it to destination buffer */
                *buf++ = '%';
                fmt++;

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */

            default: /* we've got unknown format specifier just print it as is into the destination buffer */
                *buf++ = *fmt++;

                continue; /* to outer loop to parse next format specifier or just copy character to destination from the format specifier */
            }

            /**
             * Pprint types derived from number types: 
             *   - off_t;
             *   - ngx_pid_t;
             *   - time_t;
             *   - ngx_msec_t;
             *   - size_t;
             *   - ssize_t;
             *   - ngx_int_t;
             *   - ngx_uint_t;
             *   - int;
             *   - u_int;
             *   - long;
             *   - u_long;
             *   - int32_t;
             *   - uint32_t;
             *   - int64_t;
             *   - uint64_t;
             *   - ngx_atomic_int_t;
             *   - ngx_atomic_uint_t;
             *   - rlim_t;
             *   - void * .
             */
            if (sign) {
                if (i64 < 0) { /* if signed and is negative, print the '-' separately and convert the number to positive and unsigned */
                    *buf++ = '-';
                    ui64 = (uint64_t) -i64;
                } else {
                    ui64 = (uint64_t) i64;
                }
            }
            /* if not signed, we've already got a valid ui64 */

            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width);

            fmt++;

            /* go to next format specifier or character which is just part of some text */
        } else { /* if not a format specifier, just copy character from `fmt` to `buf` */
            *buf++ = *fmt++; 
        }
    }

    return buf;
}

static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, ngx_uint_t hexadecimal, ngx_uint_t width) {
    /**
     * we need temp[NGX_INT64_LEN] only,
     * but icc issues the warning
     */
    u_char         *p, temp[NGX_INT64_LEN + 1];  
    size_t          len;
    uint32_t        ui32;
    static u_char   hex[] = "0123456789abcdef";
    static u_char   HEX[] = "0123456789ABCDEF";

    p = temp + NGX_INT64_LEN; /* since we are writing and converting the decimal digits going from left to right, start at the end and go backwards 
                                (may require padding on the left) */

    if (hexadecimal == 0) {
        if (ui64 <= (uint64_t) NGX_MAX_UINT32_VALUE) { /* for uint32s */
            /*
             * To divide 64-bit numbers and to find remainders
             * on the x86 platform gcc and icc call the libc functions
             * [u]divdi3() and [u]moddi3(), they call another function
             * in its turn. On FreeBSD it is the qdivrem() function,
             * its source code is about 170 lines of the code.
             * The glibc counterpart is about 150 lines of the code.
             *
             * For 32-bit numbers and some divisors gcc and icc use
             * a inlined multiplication and shifts. For example,
             * unsigned "i32 / 10" is compiled to
             *
             *     (i32 * 0xCCCCCCCD) >> 35
             */
            ui32 = (uint32_t) ui64; /* we've got a number that fits into a uint32_t, cast it so we can abuse the above mentioned compiler optimization */

            do {
                *--p = (u_char) (ui32 % 10 + '0'); /* get the right-most decimal digit, convert it to ASCII and write it to the buffer */
            } while(ui32 /= 10); /* drop the right-most digit and while we have decimal digits left */
        } else { /* no compiler optimizations for uint64s */
            do {
                *--p = (u_char) (ui64 % 10 + '0'); /* get the right-most decimal digit, convert it to ASCII and write it to the buffer */
            } while(ui64 /= 10); /* drop the right-most digit and while we have decimal digits left */
        }
    } else if (hexadecimal == 1) { /* lower-case hexadecimal characters */
        do {
            /* the "(uint32_t)" cast disables the BCC's warning */
            *--p = hex[(uint32_t) (ui64 & 0xf)]; /* mask off the right most nibble which gives as the hexadecimal index number */
        } while (ui64 >>= 4); /* each hexadecimal digit is a nibble from the binary representation of the original decimal digit, while we non-zero have nibbles left */
    } else { /* hexadecimal == 2, upper-case hexadecimal characters */
         do {
            /* the "(uint32_t)" cast disables the BCC's warning */
            *--p = HEX[(uint32_t) (ui64 & 0xf)]; /* mask off the right most nibble which gives as the hexadecimal index number */
        } while (ui64 >>= 4); /* each hexadecimal digit is a nibble from the binary representation of the original decimal digit, while we non-zero have nibbles left */
    }

    /* zero or space padding */
    len = (temp + NGX_INT64_LEN) - p; /* get the number of bytes the raw decimal or hexadecimal number required to be converted to ASCII */
    while (len++ < width && buf < last) { /* `len++ < width` pad out from the left until length of the number's ASCII representation meets the required width */ 
        *buf++ = zero;
    }

    /* number safe copy */
    len = (temp + NGX_INT64_LEN) - p; /* get the number of bytes the raw decimal or hexadecimal number required to be converted to ASCII */
    if (buf + len > last) { /* we don't have enough space in the buffer to write the full ASCII represenation of the number, after the width padding was applied */
        len = last - buf; /* only copy part of the ASCII represenation of the number */
    }

    /* copies the ASCII decimal/hexadecimal represenation of the number into buffer and return a u_char pointer that points into buf just after the copied number */
    return ngx_cpymem(buf, p, len);
}

static u_char *ngx_sprintf_str(u_char *buf, u_char *last, u_char *src, size_t len, ngx_uint_t hexadecimal) {
    static u_char   hex[] = "0123456789abcdef";
    static u_char   HEX[] = "0123456789ABCDEF";

    if (hexadecimal == 0) {
        if (len == (size_t) -1) { /* requested to try and copy the whole string into buffer, copy character by character */
            /**
             * copy until 0 string terminator or there's no more space left in buffer, can't use memcpy, 
             * because don't know the length and we don't want to make an extra `strlen` call 
             * which would require and additional character array iteration 
             */
            while(*src && buf < last) { 
                *buf++ = *src++;
            }
        } else { /* requested to copy just `len` characters from string into buffer */
            len = ngx_min((size_t) (last-buf), len); /* copy either `len` characters or if there's less space in buffer, copy less */
            buf = ngx_cpymem(buf, src, len); /* copy part of the source string and make `buf` point to just after the copied string */
        }
    } else if (hexadecimal == 1) { /* lower-case hexadecimal characters, encode string as a sequence of hexadecimal byte representations */
        if (len == (size_t) -1) { /* requested to try and copy the whole string into buffer, convert each character to hex and copy character by character */
            while (*src && buf < last - 1) { /* copy until 0 string terminator or there's no more space left in buffer for 2 additional bytes */
                *buf++ = hex[*src >> 4]; /* get the 1st nibble of the character byte and convert to hex character */
                *buf++ = hex[*src++ & 0xf]; /* (& 0xf) mask of the lower nibble of the byte, get the 2nd nibble of the character byte and convert to hex character */
            }
        } else { /* requested to convert each character to hex and copy just `len` characters from string into buffer */
            while(len-- && buf < last -1) { /* copy either `len` characters or if there's less space in buffer, copy less */
                *buf++ = hex[*src >> 4]; /* get the 1st nibble of the character byte and convert to hex character */
                *buf++ = hex[*src++ & 0xf]; /* (& 0xf) mask of the lower nibble of the byte, get the 2nd nibble of the character byte and convert to hex character */
            }
        }
    } else { /* hexadecimal == 2, upper-case hexadecimal characters, check out lower-case hex abover for detailed explanation of the code below */
        if (len == (size_t) -1) {
            while (*src && buf < last - 1) {
                *buf++ = HEX[*src >> 4];
                *buf++ = HEX[*src++ & 0xf];
            }

        } else {
            while (len-- && buf < last - 1) {
                *buf++ = HEX[*src >> 4];
                *buf++ = HEX[*src++ & 0xf];
            }
        }
    }

    return buf;
}