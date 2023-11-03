#include <ngx_config.h>
#include <ngx_core.h>


static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);


ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log) {
    ngx_pool_t  *p;

    /**
     * Allocates `size` bytes and returns the address of the allocated memory.
     * The address of the allocated memory will be a multiple of alignment, 
     * which must be a power of two and a multiple of sizeof(void *), 16 in this case.
     * This address can later be successfully passed to free(3).
     * 
     * TODO!!!!! is the 16-byte alignment somehow related to C requiring the stack
     * pointer to be 16-byte aligned (required by System V Application Binary Interface AMD64)?
     * Check out the file named "stack-alignment.md" at the root of the project.
     * 
     * Returns zero on success or -EINVAL or -ENOMEM. EINVAL is returned if the alignment argument 
     * was not a power of two, or was not a multiple of sizeof(void *). ENOMEM is returned if 
     * there was insufficient memory to fulfill the allocation request.
     * 
     * Will normally allocate a block from the heap, like malloc, whereas mmap will always go to the operating system.
     * 
     * If size is 0, then the value returned is either NULL or a unique pointer value.
     */
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(ngx_pool_t); /* TODO!!!!! address of last byte of `ngx_pool_t` fields to which this `ngx_pool_data_t` belongs? After this address start a chunk of free memory belonging to this pool? */
    p->d.end = (u_char *) p + size; /* TODO!!!!! address of last byte of memory allocated by this pool? */
    p->d.next = NULL; /* TODO!!!!! */
    p->d.failed = NULL; /* TODO!!!!! */
    /* TODO!!!!! so basically memory to allocate from is denoted by the range (p->d.last, p->d.end)?  */

    size = size - sizeof(ngx_pool_t); /* TODO!!!!! size of memory chunk belonging to this pool used for allocation? */
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL; /* TODO!!!!! max allocation is either capped by the pool's free space of by an OS page */

    p->current = p; /* TODO!!!!! */
    p->chain = NULL; /* TODO!!!!! */
    p->large = NULL; /* TODO!!!!! */
    p->cleanup = NULL; /* TODO!!!!! */
    p->log = log;

    return p;
}


/**
 * 
 */
void ngx_destroy_pool(ngx_pool_t *pool) {
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    /**
     * TODO!!!!! add more details.
     * Cycle through "main" pool's singly-linked list of pointers to `ngx_pool_cleanup_t` structs, 
     * which are themselves allocated from this same pool (small allocations), and call their 
     * corresponding cleanup handlers against their corresponding opaque data pointers allocated from 
     * this pool as well (small or large allocations, depending on the size specified at registration time; 
     * cast to required type by the cleanup handlers).
     */
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "run cleanup: %p", c);
            c->handler(c->data);
        }
    }

#if (NGX_DEBUG)

    /**
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool,
     * so log all the messages before actually free()ing.
     */

    for (l = pool->large; l; l = l->next) {
        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p, unused: %uz", p, p->d.end - p.d->last);

        if (n== NULL) {
            break;
        }
    }

#endif 

    /**
     * Free all chunks of memory owned by the singly-linked list of `ngx_pool_large_t` structs (some of these 
     * structures may have their associated chunks freed already by manually calling `ngx_pfree()`).
     */
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    /**
     * Free the "main" pool and its singly-linked list of pools (`ngx_pool_t` structures),
     * which will also clean up any `ngx_pool_large_t` or `ngx_pool_cleanup_t` structures
     * associated with this pool, because they were allocated from this same pool (small allocations).
     */
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(pool);
        
        if (n == NULL) {
            break;
        }
    }
}


void ngx_reset_pool(ngx_pool_t *pool) {
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) { 
        if(l->alloc) {
            ngx_free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t); /* why isn't (u_char *) p + sizeof(ngx_pool_data_t)?  */
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL; /* since all `ngx_pool_data_t`s were cleared, and `ngx_pool_large_t` reside inside them */
}


/* ngx_palloc if allocation is less than or equal `pool->max`, allocates via small object allocater and aligns by word-size  */
void *ngx_palloc(ngx_pool_t *pool, size_t size) {
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 1); /* word-size aligned */
    }
#endif

    return ngx_palloc_large(pool, size);
}


/* same as ngx_palloc, but doesn't do word-size aligning for small allocations  */
void *ngx_pnalloc(ngx_pool_t *pool, size_t size) {
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }
#endif

    return ngx_palloc_large(pool, size);
}


/**
 * ngx_palloc_small allocates from a singly-linked list of pools, which can own arbitrary-sized memory 
 * chunks, the size of which is specified at "main" pool instantiation time (`ngx_create_pool()`), but 
 * can only fulfill allocation requests, that are less than or equal to "main" pool's `max` attribute, 
 * which is itself equal to pool's size or 4KB at max.
 */
static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align) {
    u_char      *m;
    ngx_pool_t  *p;

    /** 
     * The "main" pool knows about the `current` pool 
     * used for allocation (could be one of many within 
     * the linked list of pools owned by this "main" pool 
     * through `pool.d->next`). 
     */
    p = pool->current; 

    do {
        m = p->d.last; /* pointer to the start of free space within the pool we're dealing with, updated after each allocation */

        if (align) {
            /**
             * Align current pointer into the memory chunk owned by the pool we're currently looking at 
             * by machine word size (could be unaligned after last allocation request fulfilled by this pool).
             */
            m = ngx_align_ptr(m, NGX_ALIGMENT); 
        }

        if ((size_t) (p->d.end - m) >= size) { 
            /* if there's enough space in pool's memory chunk to fulfill the allocation request*/
            p->d.last = m + size; /* the alloction itself, just move pointer up by the requested size */

            return m; /* return pointer to the start of the allocated memory chunk */
        }

        p = p->d.next; /* current pool cannot fulfill allocation request, try next one in linked list of pools if any */

    } while(p);

    /**
     * None of the pools in the singly-linked list of pools 
     * were able to accomodate the requested allocation, 
     * allocate a new pool from the heap and append it 
     * to the list, possibly pointing the `pool->current` 
     * pointer at a pool further down the line, in case
     * some of the pools have failed to fulfill allocation
     * requests more than 4 times.
     * 
     * The new pool unlike the "main" pool will start
     * serving allocations from memory position just after
     * the embedded `ngx_pool_data_t` struct.
     */
    return ngx_palloc_block(pool, size);
}


/**
 * ngx_palloc_block (1) allocates new pools from the heap, (2) appends them 
 * to the "main" pool's singly-linked list of pools, (3) possibly updates 
 * "main" pool's `current` pointer into this linked list, so that it points 
 * to another pool further down the line, skipping pools that have failed to 
 * serve more than 4 allocation requests, (4) and serves the current allocation 
 * request from the newly instantiated pool.
 */
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size) {
    u_char      *m;       
    size_t       psize;    
    ngx_pool_t  *p, *new; 


    /**
     * Allocate a chunk of memory of the same size as the one originally used by the passed in "main" pool 
     * for fulfilling allocation requests (lays out in memory right after the "main" pool struct itself, with 
     * its upper bound recorded in `pool->d.end`).
     * 
     * The passed in "main" pool was instantiated via `ngx_create_pool()` and thus holds the metadata 
     * for all of the pools owned by it.
     * 
     * Part of this size will be used to accomodate the new pool's embedded `ngx_pool_data_t` struct,
     * which holds pointers to the start and end of the memory chunk (start is updated with each allocation 
     * from this pool), a counter of how many times we failed to allocate from this pool (used to adjust 
     * the `current` pointer of the "main" pool, if it will point at this pool some time in the future and 
     * this pool fails to fulfill allocation requests more than 4 times) and a pointer to a possible future 
     * pool down the line.
     * 
     * BUT unlike the "main" pool this pool will use all the space taken up by the rest of the metadata attributes 
     * for allocation. This means the memory chunk used for allocation will lay out in memory right after the embedded 
     * `ngx_pool_data_t` struct, instead of right after the pool struct itself.
     */
    psize = (size_t) (pool->d.end - (u_char *) pool);

    /**
     * Allocates `size` bytes and returns the address of the allocated memory.
     * The address of the allocated memory will be a multiple of alignment, 
     * which must be a power of two and a multiple of sizeof(void *), 16 in 
     * this case. This address can later be successfully passed to free(3).
     * 
     * TODO!!!!! is the 16-byte alignment somehow related to C requiring the stack
     * pointer to be 16-byte aligned (required by System V Application Binary Interface AMD64)?
     * Check out the file named "stack-alignment.md" at the root of the project.
     *
     * 
     * Returns zero on success or -EINVAL or -ENOMEM. EINVAL is returned if 
     * the alignment argument  was not a power of two, or was not a multiple 
     * of sizeof(void *). ENOMEM is returned if there was insufficient memory 
     * to fulfill the allocation request.
     * 
     * Will normally allocate a block from the heap, like malloc, whereas mmap 
     * will always go to the operating system.
     * 
     * If size is 0, then the value returned is either NULL or a unique pointer value.
     */
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log); /* allocate memory for new pool's embedded `ngx_pool_data_t` struct and the rest will be used to fulfill allocation requests */
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m; /* cast raw memory pointer to pool type to be able to set attributes within the embedded `ngx_pool_data_t` struct */

    new->d.end = m + psize; /* get pool data bounds in place */
    new->d.next = NULL;
    new->d.failed = 0;

    /**
     * unlike the "main" pool new pools owned by it will ovewrite their other 
     * metadata attributes and serve allocation requests right after the embedded 
     * `ngx_pool_data_t` struct.
     */
    m += sizeof(ngx_pool_data_t); 
    m = ngx_align_ptr(m, NGX_ALIGMENT); /* align by machine word size, we cannot trust `ngx_pool_data_t` to be word-size aligned (although it is on 64-bit systems) */
    new->d.last = m + size; /* fulfill allocation request overriding other metadata attributes, next allocation will start at `new->d.last` position */

    /**
     * This function is called when the current pool isn't
     * able to fulfill a given allocation request, because
     * there's no space left in it, or because the request
     * is to large to fit in the left over space of the `current`
     * pool. So cycle through the linked list of pools looking
     * for pools that failed to serve more than 4 allocation requests, 
     * make the pool right after the last such pool the new `current` pool.
     * If no such pools are encountered leave the `current` pool pointer untouched.
     */
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) { 
            pool->current = p->d.next; 
        }
    }

    p->d.next = new; /* append new pool to tail of the linked list of pools */

    return m;
}
   

/**
 * TODO!!!!! ngx_palloc_large - for allocations greater than pool's max size which is at most 4KB?
 * Linked list of `ngx_pool_large_t`s? 
 * Each containing a slab of memory greater than 4KB directly allocated 
 * from the heap/OS via malloc and a link to next `ngx_pool_large_t` in the list?
 * 
 * Tail is always with a NULL pointer in `.alloc` attribute?
*/
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size) {
    void              *p; /* TODO!!!!! */
    ngx_uint_t         n;
    ngx_pool_large_t  *large;

    p = ngx_alloc(size, pool->log); /* malloc with some logging added */
    if (p == NULL) {
        return NULL;
    }

    /**
     * Iterate through "main" pool's singly-linked list of `ngx_pool_large_t` 
     * structure's
     */
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {

            /**
             * TODO!!!!! found at least within the 1st 4 elements of the list a `ngx_pool_large_t` 
             * without any chunk of memory associated with it, 
             * make it own the just allocated slab and return a pointer to the start of that slab 
             */ 

            large->alloc = p;
            return p;
        }

        if (n++ > 3) { /* try finding an `ngx_pool_large_t` with no memory associated with it within only the first 4 elements of the linked list */
            break;
        }
    }

    /**
     * All first 4 `ngx_pool_large_t`s within the linked list have memory already associated with them, 
     * allocate a new one via pool's small object allocator (word aligned),
     * associate the just allocated slab of memory with it and insert it at the head of the list.
     */

    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p); /* just a macro alias for `free` */
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


/**
 * same as ngx_palloc_large, but allocates slab via posix_memalign aligned to alignment, 
 * and always allocates a new `ngx_pool_large_t` to associate the allocated slab with it,
 * and inserts it at the head of the linked list
 */
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment) {
    void              *p;
    ngx_pool_large_t  *large;

    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


/* only for large allocations */
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p) {
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
            ngx_free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


void *ngx_pcalloc(ngx_pool_t *pool, size_t size) {
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

/**
 * ngx_pool_cleanup_add allocates a new `ngx_pool_cleanup_t` struct from the
 * same pool that's passed into it, possibly allocates additional storage from 
 * the pool too, the size of which is determined by the `size` parameter, and inserts
 * the newly allocated `ngx_pool_cleanup_t` struct at the head of the singly-linked 
 * list of "cleanups". The caller may set the required cleanup handler and store
 * data that should be passed into the handler within the additionally allocted storage 
 * after this function returnes. Cleanup handlers are called in order of singly-linked
 * list iteration just before the pool gets destoryed.
 */
ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size) {
    ngx_pool_cleanup_t  *c;

    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t)); /* allocates via `ngx_palloc_small()` */
    if (c == NULL) {
        return NULL;
    }

    if (size) {

        /**  
         * May allocate via `ngx_palloc_small()` or `ngx_palloc_large()`,
         * depending whether `size` is greater than "main" pool's `max` 
         * field.   
         * 
         * Will be passed to `ngx_pool_cleanup_t` `handler` as the only argument, 
         * before this pool gets destroyed. The `handler` is set by the caller
         * after this function returns this `ngx_pool_cleanup_t` instance. The
         * caller may also store necessary data in the storage allocated here.
         */

        c->data = ngx_palloc(p, size); 
        if (c->data == NULL) {
            return NULL;
        }
    } else {
        c->data = NULL;
    }

    c->handler = NULL;    /*set by the caller after this function returns */
    c->next = p->cleanup; /* insert at the head */

    p->cleanup = c;       /* insert at the head */

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}