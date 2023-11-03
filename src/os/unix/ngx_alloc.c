#include <ngx_config.h>
#include <ngx_core.h>


void *ngx_alloc(size_t size, ngx_log_t *log) {
    void *p;

    p = malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "malloced(%uz) failed", size);
    }

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}

void *ngx_calloc(size_t size, ngx_log_t *log) {
    void *p;

    p = ngx_alloc(size, log);

    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

/**
 * The function posix_memalign() allocates size bytes and places the address of the allocated memory in *memptr.  
 * The address of the allocated memory will be a multiple of alignment, which must be a power of two and a multiple of sizeof(void *).
 * This address can later be successfully passed to free(3).  If size is 0, then the value placed in *memptr is either NULL 
 * or a unique pointer value.
 * 
 * posix_memalign() returns zero on success, or one of the error values listed in the next section on failure.  
 * The value of errno is not set.  On Linux (and other systems), posix_memalign() does not modify memptr on failure.  
 * A requirement standardizing this behavior was added in POSIX.1-2008 TC2.
 * 
 * posix_memalign() verifies that alignment matches the requirements detailed above.  
 * memalign() may not check that the alignment argument is correct.
 * 
 * POSIX requires that memory obtained from posix_memalign() can be freed using free(3).  
 * Some systems provide no way to reclaim memory allocated with memalign() (because one can pass to free(3) only a pointer 
 * obtained from malloc(3), while, for example, memalign() would call malloc(3) and then align the obtained value).  
 * The glibc implementation allows memory obtained from any of these functions to be reclaimed with free(3). 
 * 
 * The glibc malloc(3) always returns 8-byte aligned memory addresses, so these functions are needed only if you require larger alignment values.
 * 
 * Whereas malloc gives you a chunk of memory that could have any alignment (the only requirement is that it must be aligned for the 
 * largest primitive type that the implementation supports), posix_memalign gives you a chunk of memory that is guaranteed to have 
 * the requested alignment.
 * The result of malloc does not have just "any" alignment. It is guaranteed to be suitably aligned for any native type 
 * on the system (int, long, double, structs, etc.). 
 * 
 * Is there a good reason to prefer posix_memalign over mmap, which allocates aligned memory?
 * posix_memalign will normally allocate a block from the heap, whereas mmap will always go to the operating system. 
 * If, for example, you wanted to allocate many cache line aligned blocks of memory, then posix_memalign is much preferred. 
 * It's the same reason to prefer malloc over mmap.
 * 
 * On recent x86 architectures a cache-line, which is the smallest amount of data that can fetched from memory to cache, is 64 bytes. 
 * Suppose your structure size is 56 bytes, you have a large array of them. When you lookup one element, the CPU will need to issue 
 * 2 memory requests (it might issue 2 requests even if it is in the middle of the cacheline). That is bad for performance, 
 * as you have to wait for memory, and you use more cache, which ultimately gives a higher cache-miss ratio. 
 * In this case it is not enough to just use posix_memalign, but you should pad or compact your structure to be on 64 byte boundaries.
 * 
 * As memalign is obsolete (ref: man page), only the difference between malloc() and posix_memalign() will be described here. 
 * malloc() is 8-byte aligned (eg, for RHEL 32-bit), but for posix_memalign(), alignment is user-definable. 
 * To know the use of this, perhaps one good example is setting memory attribute using mprotect(). To use mprotect(), the memory pointer 
 * must be PAGE aligned. And so if you call posix_memalign() with pagesize as the alignment, then the returned pointer can easily 
 * submit to mprotect() to set the read-write-executable attributes. (for example, after you copy the data into the memory pointer, 
 * you can set it to read-only attribute to protect it from being modified). "malloc()"'s returned pointer cannot be use here.
 * 
 * When you use posix_memalign in GNU C, you should be careful that second parameter should not only be a power of two, 
 * but also be a multiple of sizeof (void*). Note that this requirement is different from the one in the memalign function, 
 * which requires only power of two.
 * 
 * ERRORS:
 *   - EINVAL : the alignment argument was not a power of two, or was not a multiple of sizeof(void *);
 *   
 *   - ENOMEM : there was insufficient memory to fulfill the allocation request.
 */

#if (NGX_HAVE_POSIX_MEMALIGN)

void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log) {
    void *p;
    int err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
        ngx_log_error(NGX_LOG_EMERG, log, err, "posix_memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0, "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (NGX_HAVE_MEMALIGN)

void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log) {
    void *p;

    p = memalign(alignment, size);

    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "memalign(%uz, %uz) failed", alignment, size);
    }

    ngx_log_debug3(NGX_LOG_DEBUG_ALLOC, log, 0, "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif