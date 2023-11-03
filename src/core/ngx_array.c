#include <ngx_config.h>
#include <ngx_core.h>


ngx_array_t *ngx_array_create(ngx_pool_t *p, ngx_uint_t n, size_t size) {
    ngx_array_t *a;

    a = ngx_palloc(p, sizeof(ngx_array_t)); /* allocate memory from pool to store the actual array structure */
    if (a == NULL) {
        return NULL;
    }

    /**
     * Sets array's initial capacity to `n` (`nalloc` field), 
     * 
     *              size of each element to `size` (`size` field), 
     * 
     *              array length currently stored in the array to 0 (`nelts`),
     * 
     *              array's reference to the pool from which it was allocated to `p` (`pool` field) and
     *              
     *              allocates a contiguos zeroed chunk of memory of size (`n` * `size`) from the passed in pool `p` and 
     *              stores a pointer to it in array's `*elts` field.
     */
    if (ngx_array_init(a, p, n, size) != NGX_OK) {
        return NULL;
    }

    return a;
}


void ngx_array_destroy(ngx_array_t *a) {
    ngx_pool_t  *p;

    p = a->pool;

    if ((u_char *) a->elts + a->size * a->nalloc == p->d.last) {

        /**
         * Try our best to free the underlying backing array.
         * We can only achieve this only if the array was allocated
         * via the pool's "small object" allocator and it's the last
         * "small" allocation that was perfromed by the pool, in this
         * case "freeing" just means adjusting the pool's "start of free 
         * memory pointer", as if the backing array allocation never happened.
         */

        p->d.last -= a->size * a->nalloc;
    }

    if ((u_char *) a + sizeof(ngx_array_t) == p->d.last) {

         /**
         * Try our best to free space occupied by the actual `ngx_array_t` 
         * structure. We can only achieve this only if the structure is 
         * allocated via pool's "small object allocator", which it probably
         * always is, and:
         *   a) the structure was allocated just before it's underlying backing 
         *      array was, and the backing array was allocated via pool's "small 
         *      object allocator";
         *   b) or the backing array was allocated via pool's "large object allocator"
         *      and the actual structure is the last "small object" allocated from the 
         *      pool.
         * In both those cases "freeing" just means adjusting the pool's "start of free 
         * memory pointer", as if the struture allocation never happened.
         */

        p->d.last = (u_char *) a;
    }
}


void *ngx_array_push(ngx_array_t *a) {
    void        *elt, *new;
    size_t       size;
    ngx_pool_t  *p;

    if (a->nelts == a->nalloc) {

        /* the array is full, length is equal to capacity */

        /**
         * Calculate the amount of pooled memory used up by the backing array 
         * by multiplying the size of a single element by the current capacity 
         * of the array.
         */
        size = a->size * a->nalloc; 

        p = a->pool;

        if ((u_char *) a->elts + size == p->d.last && p->d.last + a->size <= p->d.end) {

            /**
             * The array allocation is the last in the pool, 
             * meaning no other allocations happened since 
             * we've last allocated memory for this array 
             * from this pool. We enforce this because we 
             * need a contiguos chunk of memory to store 
             * the backing array.
             * And there is space for allocation of at least 
             * one extra element (the previous allocation was 
             * made by the pool's "small object allocator", 
             * and just extending the array past it's current 
             * bound will not exceed the pool's current chunk 
             * of memory used for allocating "small objects"). 
             */
            p->d.last += a->size; /* just extend the array's bounds so that it can fit another element */
            a->nalloc++;          /* increment array's capacity by 1 */
        } else {

            /**
             * Allocate a new array, either someone else allocated 
             * from this same pool since we've last allocated space 
             * for this array, or there's not enought room left for 
             * an extra element in the pool's current chunk of memory 
             * used for allocating "small objects". 
             */


            /** 
             * Double array's current capacity, this may lead to allocating 
             * directly from the heap/OS if we hit pool's max size for a 
             * single allocation (pool's "large object allocator").
             */
            new = ngx_palloc(p, 2 * size); 
            if (new == NULL) {
                return NULL;
            }

            ngx_memcpy(new, a->elts, size); /* copy content over */
            a->elts = new;
            a->nalloc *= 2;                 /* double capacity */
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts; /* get pointer to next array's free slot */
    a->nelts; /* increment array's length, caller will store an element via the returned `elt` pointer to array's slot */

    return elt;
}

void *ngx_array_push_n(ngx_array_t *a, ngx_uint_t n) {
    void        *elt, *new;
    size_t       size;
    ngx_uint_t   nalloc;
    ngx_pool_t  *p;

    size = n * a->size; /* amount of memory required to store another `n` elements in the array */

    if (a->nelts + n > a->nalloc) {

        /**
         * The array is full, storing another `n` elements 
         * will exceed current capacity 
         */

         if ((u_char *) a->elts + size == p->d.last && p->d.last + a->size <= p->d.end) {

            /**
             * The array allocation is the last in the pool, 
             * meaning no other allocations happened since 
             * we've last allocated memory for this array 
             * from this pool. We enforce this because we 
             * need a contiguos chunk of memory to store 
             * the backing array.
             * And there is space for allocation of at least 
             * `n` extra elements (the previous allocation was 
             * made by the pool's "small object allocator", 
             * and just extending the array past it's current 
             * bound will not exceed the pool's current chunk 
             * of memory used for allocating "small objects"). 
             */

            p->d.last += size; /* just extend the array's bounds so that it can fit `n` extra elements */
            a->nalloc += n;    /* updated array's capacity */
        } else {

             /**
             * Allocate a new array, either someone else allocated 
             * from this same pool since we've last allocated space 
             * for this array, or there's not enought room left for 
             * `n` extra elements in the pool's current chunk of memory 
             * used for allocating "small objects".
             */

            nalloc = 2 * ((n >= a->nalloc) ? n : a->nalloc); /* either double the array's capacity or make it's capacity `2 * n`, whichever is greater */

            /** 
             * This may lead to allocating directly from the heap/OS 
             * if we hit pool's max size for a single allocation 
             * (pool's "large object allocator").
             */
            new = ngx_palloc(p, nalloc * a->size);
            if (new == NULL) {
                return NULL;
            }

            ngx_memcpy(new, a->elts, a->nelts * a->size); /* copy content over */
            a->elts = new;
            a->nalloc = nalloc; /* update capacity */
        }
    }

    elt = (u_char *) a->elts + a->size * a->nelts; /* get pointer to next free slot in the array, next `n` slots will be occupied by caller */
    a->nelts += n; /* update array's length */

    return elt;
}