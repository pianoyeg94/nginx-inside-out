#ifndef _NGX_STRING_H_INCLUDED_
#define _NGX_STRING_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct {
    size_t      len;
    u_char     *data;
} ngx_str_t;


typedef struct {
    unsigned    len:28;

    unsigned    valid:1;
    unsigned    no_cacheable:1;
    unsigned    not_found:1;
    unsigned    escape:1;

    u_char      *data;
} ngx_variable_value_t; /* TODO!!!! why bit fields? 28-bit length plus 4-bit flag form a uint32 */

#define ngx_string(str)     { sizeof(str) - 1, (u_char *) str }     
#define ngx_null_string     { 0, NULL }


#define ngx_strlen(s)       strlen((const char *) s)


/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
#define ngx_memset(buf, c, n)     (void) memset(buf, c, n)


#if (NGX_MEMCPY_LIMIT) /* TODO!!!!! seems like not defined anywhere anymore, even at configure time */

/* TODO!!!!! */

#else

/**
 * gcc3, msvc, and icc7 compile memcpy() to the inline "rep movs":
 *   - movs: https://c9x.me/x86/html/file_module_x86_id_203.html
 *   - rep:  https://docs.oracle.com/cd/E19455-01/806-3773/instructionset-64/index.html
 * gcc3 compiles memcpy(d, s, 4) to the inline "mov"es.
 * icc8 compile memcpy(d, s, 4) to the inline "mov"es or XMM moves.
 */
/* vanilla memcpy, but returns void instead of void* */
#define ngx_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
/* does a memcpy, moves pointer to the end of the copied block, 
   casts void pointer to a u_char pointer  */
#define ngx_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n)) 

#endif

u_char *ngx_cpystrn(u_char *dst, u_char *src, size_t n);
u_char * ngx_cdecl ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
u_char *ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);

#endif /* _NGX_STRING_H_INCLUDED_ */