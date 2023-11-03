#ifndef _NGX_CONFIG_H_INCLUDED_
#define _NGX_CONFIG_H_INCLUDED_

#include <ngx_auto_headers.h>

#if (NGX_LINUX)
#include <ngx_linux_config.h>
#endif 

#ifndef NGX_HAVE_SO_SNDLOWAT
#define NGX_HAVE_SO_SNDLOWAT     1
#endif

#if !(NGX_WIN32)

#define ngx_signal_helper(n)     SIG##n  
#define ngx_signal_value(n)      ngx_signal_helper(n) 

#define ngx_random               random /* stdlib.h */

/* TODO: #ifndef */
#define NGX_SHUTDOWN_SIGNAL      QUIT
#define NGX_TERMINATE_SIGNAL     TERM 
#define NGX_NOACCEPT_SIGNAL      WINCH 
#define NGX_RECONFIGURE_SIGNAL   HUP

#if (NG_LINUXTHREADS)
#define NGX_REOPEN_SIGNAL        INFO 
#define NGX_CHANGEBIN_SIGNAL     XCPU 
#else
#define NGX_REOPEN_SIGNAL        USR1
#define NGX_CHANGEBIN_SIGNAL     USR2
#endif /* (NG_LINUXTHREADS) */

/**
 * A dummy macros on Linux, on Windows are aliases for the `__cdecl` macro, 
 * which marks that a function should use the C calling convention and 
 * not the default Microsoft calling conventon: 
 *     https://en.wikipedia.org/wiki/X86_calling_conventions, used only  
 */
#define ngx_cdecl 
#define ngx_libc_cdecl

#endif /* !(NGX_WIN32) */

typedef intptr_t        ngx_int_t;    /* same as `long int`, intptr_t from unistd.h => types.h */
typedef uintptr_t       ngx_uint_t;   /* same as `unsigned long int`, `uintptr_t` from stdint.h */
typedef intptr_t        ngx_flag_t;   /* same as `long int`, intptr_t from unistd.h => types.h */

#define NGX_INT32_LEN   (sizeof("-2147483648") - 1) /* 10 decimal digits at max + 1 sign */
#define NGX_INT64_LEN   (sizeof("-9223372036854775808") - 1) /* 19 decimal digits at max - 1 for the sign */

#if (NGX_PTR_SIZE == 4) /* on 32-bit platforms */
#define NGX_INT_T_LEN   NGX_INT32_LEN
#define NGX_MAX_INT_T_VALUE  2147483647
#else                   /* on 64-bit platforms */ 
#define NGX_INT_T_LEN   NGX_INT64_LEN /* 19 decimal digits at max + 1 */
#define NGX_MAX_INT_T_VALUE  9223372036854775807 
#endif

#ifndef NGX_ALIGMENT
#define NGX_ALIGMENT   sizeof(unsigned long)    /* platform word */
#endif

#define ngx_align(d, a)    (((d) + (a - 1)) & ~(a-1)) /* example: a = 8; ~(a-1) = 0b000, by doing d + 0b111 and then ANDING with 0b000 we align by 8 */
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1)) /* same as ngx_align, but casts the result to an unsigned character pointer */


#define ngx_abort     abort /* stdlib.h */

/* TODO: platform specific: array[NGX_INVALID_ARRAY_INDEX] must cause SIGSEGV */
#define NGX_INVALID_ARRAY_INDEX 0x80000000 /* TODO!!!!! */

/* TODO: auto_conf: ngx_inline   inline __inline __inline__ */
#ifndef ngx_inline
#define ngx_inline      inline /* alias for inline hint, on different systems inline hint can be one of the three (inline, __inline, __inline__)  */
#endif

#ifndef INADDR_NONE  /* Solaris */
#define INADDR_NONE  ((unsigned int) -1)
#endif

#ifdef MAXHOSTNAMELEN
#define NGX_MAXHOSTNAMELEN  MAXHOSTNAMELEN
#else
#define NGX_MAXHOSTNAMELEN  256 /* TODO!!!!! */
#endif

#define NGX_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#define NGX_MAX_INT32_VALUE   (uint32_t) 0x7fffffff

#if (NGX_COMPAT) /* TODO!!!!! */

#define NGX_COMPAT_BEGIN(slots)  uint64_t spare[slots];
#define NGX_COMPAT_END

#else

#define NGX_COMPAT_BEGIN(slots) /* no-op */
#define NGX_COMPAT_END

#endif

#endif /* _NGX_CONFIG_H_INCLUDED_ */
