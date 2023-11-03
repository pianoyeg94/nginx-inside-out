#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_SLAB_PAGE       0
#define NGX_SLAB_BIG        1


#if (NGX_PTR_SIZE == 4)

#define NGX_SLAB_PAGE_START  0x80000000 /* 32 bits with the highest bit set */

#define NGX_SLAB_MAP_SHIFT   16         // TODO!!!!!!!!!!!!!!!!!!!!!

#else /* (NGX_PTR_SIZE == 8) */

#define NGX_SLAB_PAGE_START  0x8000000000000000 /* 64 bits with the highest bit set */
#define NGX_SLAB_MAP_MASK    0xffffffff00000000 /* 32 highest bits set */
#define NGX_SLAB_MAP_SHIFT   32                 // TODO!!!!!!!!!!!!!!!!!!!!!

#endif


/* get start address of slab slots first slab page, just after the last byte of the slab pool struct) */
#define ngx_slab_slots(pool)                                                  \
    (ngx_slab_page_t *) ((u_char *) (pool) + sizeof(ngx_slab_pool_t))

/** 
 * Get address of data page, derived from page header struct,
 * on first allocation of a single page at index 0 will be equal 
 * to (pool)->start 
 */
#define ngx_slab_page_addr(pool, page)                                        \
    ((((page) - (pool)->pages) << ngx_pagesize_shift)                         \
     + (uintptr_t) (pool)->start)


#if (NGX_DEBUG_MALLOC)

#define ngx_slab_junk(p, size)      ngx_memset(p, 0xA5, size)

#elif (NGX_HAVE_DEBUG_MALLOC)

#define ngx_slab_junk(p, size)                                                \
    if (ngx_debug_malloc)           ngx_memset(p, 0xA5, size)

#else

#define ngx_slab_junk(p, size)

#endif


static ngx_uint_t  ngx_slab_max_size;    /* 2KiB on Linux */
static ngx_uint_t  ngx_slab_exact_size;  /* 64, TODO!!!!! what unit is this? */
static ngx_uint_t  ngx_slab_exact_shift; /* On Linux we get `ngx_slab_exact_shift` equal to 7 */


void ngx_slab_sizes_init(void) {
    ngx_uint_t  n;

    ngx_slab_max_size = ngx_pagesize / 2;                            /* on Linux this will be 2KiB (4KiB / 2) */
    ngx_slab_exact_size = ngx_pagesize / (8 * sizeof(uintptr_t));    /* 64, TODO!!!!! what unit is this? */
    for (n = ngx_slab_exact_size; n >>= 1; ngx_slab_exact_shift++) {  /* On Linux we get `ngx_slab_exact_shit` equal to 7 */
        /* void */
    }
}


void ngx_slab_init(ngx_slab_pool_t *pool) {
    u_char           *p;
    size_t            size;
    ngx_int_t         m;
    ngx_uint_t        i, n, pages;
    ngx_slab_page_t  *slots, *page;

    /**
     * TODO!!!!!
     * 
     * `pool->min_shift` is used only in `ngx_init_zone_pool()` 
     * and is set to 3, so `pool->min_size` is set to 8 bytes.
     */
    pool->min_size = (size_t) 1 << pool->min_shift;

    /**
     * Get pointer to first slab page header, which resides just 
     * after the last byte of the slab pool struct.
     */
    slots = ngx_slab_slots(pool);                          

    /**
     * Cast pointer to first slab page header to a raw memory bytes 
     * pointer. 
     */
    p = (u_char *) slots;

    /**
     * Get size of the whole shared memory zone, which will accomodate
     * an array of slab page headers, an array of slab page stats and
     * the actual data pages.
     */
    size = pool->end - p;                                  

    /**
     * If NGX_DEBUG_MALLOC is set, initializes the whole shared memory zone 
     * to 0xA5 bytes, otherwise is a no-op macro.
     */
    ngx_slab_junk(p, size);                                

    /**
     * TODO!!!!!
     * 
     * `ngx_pagesize_shift` is set to 12 (log base 2 of 4096 byte page),
     * so `n` will be equal to 12 - 3 = 9.
     */
    n = ngx_pagesize_shift - pool->min_shift;              

    for (i = 0; i < n; i++) { /* TODO!!!!! 9 what?  */

        /**
         * TODO!!!!!
         * 
         * Only "next" is used in list head, which points to the page 
         * header struct itself.
         */

        slots[i].slab = 0;          /* TODO!!!!! number of slab pages? */           
        slots[i].next = &slots[i];
        slots[i].prev = 0;          /* TODO!!!!! previous slab header? */
    }

    /**
     * TODO!!!!!
     * 
     * Point to memory just after the first 9 slab page headers 
     * (page stats array will start from here).
     */
    p += n * sizeof(ngx_slab_page_t);                      

    /**
     * Initialize 9 element page stats array with zeroed out page 
     * stats structs.
     */
    pool->stats = (ngx_slab_stat_t *) p;
    ngx_memzero(pool->stats, n * sizeof(ngx_slab_stat_t)); 

    /**
     * 
     */
    p += n * sizeof(ngx_slab_stat_t);                      /* point past slab stats array */

    size -= n * ((sizeof(ngx_slab_page_t)) + sizeof(ngx_slab_stat_t)); /* get remaining size after setting up the slab headers array and the slab stats array */

    pages = (ngx_uint_t) (size / (ngx_pagesize + sizeof(ngx_slab_page_t))); /* calculate number of pages consiting of data and slab page header that can fit after slab stats array */

    pool->pages = (ngx_slab_page_t *) p; /* get pointer to first element of slab page list */
    ngx_memzero(pool->pages, pages * sizeof(ngx_slab_page_t));

    page = pool->pages; /* pointer to first slab page header after slab page stats */

    /* only "next" is used in list head */
    pool->free.slab = 0;
    pool->free.next = page;               /* free points to the slab page header after slab page stats, this header points to page? */
    pool->free.prev = 0;

    page->slab = pages;                   /* TODO!!!!! number of pages consiting of data and slab page header that can fit after slab stats array? */
    page->next = &pool->free;             /* TODO!!!!!points to it's slab page header? */
    page->prev = (uintptr_t) &pool->free;

    pool->start = ngx_align_ptr(p + (pages * sizeof(ngx_slab_page_t)), ngx_pagesize); /* align pointer to shared pages start address up to a multiple of 4096 bytes on Linux */

    m = pages - ((pool->end - pool->start) / ngx_pagesize); /* TODO!!!!! is there enough place for the actual data pages, or is there less memory left for data pages than page headers  */
    if (m > 0) {
        pages -= m;
        page->slab = pages; /* too many page headers, we've got not as much data pages */
    }

    pool->last = pool->pages + pages; /* TODO!!!!! */
    pool->pfree = pages;

    pool->log_nomem = 1;
    pool->log_ctx = &pool->zero;
    pool->zero = '\0';
}

// first alloc, requested size 1KiB
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size) {
    size_t             s;
    uintptr_t          p, m, mask;
    ngx_uint_t         i, slot, shift;
    ngx_slab_page_t   *page, *slots;


    /**
     * Size class        Slot     Shift
     * 8-15 bytes        0        3
     * 16-31 bytes       1        4
     * 32-63 bytes       2        5
     * 64-127 bytes      3        6
     * 128-255 bytes     4        7
     * 256-511 bytes     5        8
     * 512-1023 bytes    6        9
     * 1024-2047 bytes   7        10
     * 2048-4095 bytes   8        11             
     */

    if (size > pool->min_size) {
        shift = 1;
        for (s = size -1; s >> 1; shift++) { /* void */ } /* for first and second alloc with request size of 1KiB shift becomes 10 */
        slot = shift - pool->min_shift;                   /* 10 - 3, 7th page index */
    } else {
        shift = pool->min_size;
    }

    pool->stats[slot].reqs++

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab alloc: %uz slot: %ui", size, slot);

    slots = ngx_slab_slots(pool);

    /** 
     * On first allocation returns pointer to slots[slot], 8th slot page header points to it .
     * 
     * On 2nd-4th allocation returns pointer to page allocated on first allocation.
     */
    page = slots[slot].next; 

    if (page->next != page) {
        /* on 2nd -4th allocation page->next is &slots[slot] and page is equal to page allocated on first allocation */

        if (shift < ngx_slab_exact_shift) { /* on 2nd-4th allocation of a 1KiB chunk shift is 10 and ngx_slab_exact_shift is  7 */
            // TODO!!!!!!!!!!!!!!!!!!!!!!!!!
        } else if (shift == ngx_slab_exact_shift) { /* on 2nd-4th allocation of a 1KiB chunk shift is 10 and ngx_slab_exact_shift is  7 */

            for (m = 1, i = 0; m; m <<= 1, i++) { /* 64 iterations, m has it's bits set to 1 from right to left with each new iteration */
                // TODO!!!!!!
            }

        } else { /* shift > ngx_slab_exact_shift */
            
            /* on 2nd-4th allocation of a 1KiB chunk shift is 10 and ngx_slab_exact_shift is  7 */

            mask = ((uintptr_t) 1 << (ngx_pagesize >> shift)) - 1; /* on 2nd-4th allocation of a 1KiB chunk has the 4 lower bits set */
            mask << NGX_SLAB_MAP_SHIFT;                            /* on 2nd-4th allocation of a 1KiB chunk has bits 33 to 36 set */

            for (m = (uintptr_t) 1 << NGX_SLAB_MAP_SHIFT, i = 0; m & mask; m << 1, i++) { /* 4 iterations for set bits 33 to 36 in mask */
                if (page->slab & m) {
                    /* we skip bits 33 to 35 on 2nd and 3rd allocations, because page->slab has bits 33-35 set from previous allocations of a 1KiB chunk */
                    continue;
                }

                page->slab |= m; /* on 4th iteration set 36th bit in page->slab, signifying that the second 1KiB from this page is about to be allocated */

                if ((page->slab & NGX_SLAB_MAP_MASK) == mask) {
                    /* True only 4th allocation, because page->slab has bits 33 to 36 set */
                    // prev = ngx_slab_page_prev()
                } 

                /** 
                 * ngx_slab_page_addr returns (pool)->start and for i = 1, shift = 10 i << shift return s 1024, 
                 * so p points to the second 1024 bytes chunk
                 */
                p = ngx_slab_page_addr(pool, page) + (i << shift);

                pool->stats[slot].used++;

                goto done;
            }
        }
    }

    page = ngx_slab_alloc_pages(pool, 1); /* "pop" page header struct for page at index 0 */

    if (page) {
        if (shift < ngx_slab_exact_shift) {
           // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        } else if (shift == ngx_slab_exact_shift) {
            // TODO!!!!!!!!!!!!!!!!!!!!!!
        } else { /* shift > ngx_slab_exact_shift */

            /* for first alloc with request size of 1KiB shift 10 is greater than ngx_slab_exact_shift 7 */

            page->slab = ((uintptr_t) 1 << NGX_SLAB_MAP_SHIFT) | shift; /* 33rd bit set and 2nd and 4th bits set TODO!!!!! */
            page->next = &slots[slot];                                  /* on first alloc with requested size 1KiB, this will be the pointer to 8th page */
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_BIG;       /* on first alloc with requested size 1KiB, this will be the address of the 8th page with the lowest bit set */

            slots[slot].next = page; // TODO!!!!! point 8th pages next atrribute at allocated page

            pool->stats[slot].total += ngx_pagesize >> shift;           /* +4 on first alloc with requested size 1KiB (4 * 1KiB in 4096 bytes page) */

            p = ngx_slab_page_addr(pool, page);                         /* on first allocation of a single page at index 0 will be equal to (pool)->start */

            pool->stats[slot].used++;                                   /* 1 out of 4 1KiB chunks used */

            goto done;
        }
    }

    // TODO!!!!!!!!!

done:

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab alloc: %p", (void *) p);

    return (void *) p;
}


static ngx_slab_page_t *ngx_slab_alloc_pages(ngx_slab_pool_t *pool, ngx_uint_t pages) {
    ngx_slab_page_t   *page, *p;

    /**
     * On first allocation of a single page there will be only one iteration
     * with page = pool->free.next, where page->next == &pool->free.
     */
    for (page = pool->free.next; page != &pool->free; page = page->next) { 
        if (page->slab >= pages) {

            /** 
             * On first allocation of a single page, 
             * page->slab will be greater than pages, 
             * which is equal to 1.  
             * 
             * page->slab is TODO!!!!!
             */

            if (page->slab > pages) {
                 /**
                 * On first allocation with pages = 1, point last
                 * page header's prev attribute to page at index 1.
                 */
                page[page->slab - 1].prev = (uintptr_t) &page[pages];

                /**
                 * On first allocation with pages = 1, update page at index 1 
                 * to have the number of available free pages decremented by 1 
                 */
                page[pages].slab = page->slab - pages; 

                /**
                 * On first allocation with pages = 1, update page at index 1 
                 * to point at &pool->free through the next attribute.
                 */
                page[pages].next = page->next;


                /**
                 * On first allocation with pages = 1, update page at index 1 
                 * to point at &pool->free through the prev attribute.
                 */
                page[pages].prev = page->prev;


                p = (ngx_slab_page_t *) page->prev; /* cast uintptr_t to pointer to page header, which is &pool->free on first allocation */
                p->next = page->next;               /* point (&pool->free)'s next attribute back to itself */
                page->next->prev = page->prev;      /* point (&pool->free)'s prev attribute back to itself */
            } else {
                // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            }

            page->slab = pages | NGX_SLAB_PAGE_START; // TODO!!!!!!!!!!!!!!! updates page at index 0 (sets slab atrribute of allocated page to 0)
            /**
             * On first allocation page at index 0 gets allocated, 
             * prune all of its pointer attributes 
             */
            page->next = NULL;                        
            page->prev = NGX_SLAB_PAGE; /* set to 0 */

            pool->pfree -= pages; /* decrement count of free pages */

            if (--pages == 0) {
                /** 
                 * On first allocation just the 0th page is allocated 
                 * so return it straight away 
                 */
                return page;
            }

            // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }
    }
}