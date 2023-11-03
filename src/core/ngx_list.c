#include <ngx_config.h>
#include <ngx_core.h>


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size) {
    ngx_list_t  *list;

    list = ngx_palloc(pool, sizeof(ngx_list_t)); /* allocate the actual list structure */
    if (list == NULL) {
        return NULL;
    }

    /**
     * - allocates first part's backing array from pool;
     * - points last part aka current part used to append new elements to the first part;
     * - sets list element size to passed in `size` argument
     * - sets current part's capacity to `n`
     * - save pool pointer in list's structure
     */
    if (ngx_list_init(list, pool, n, size) != NGX_OK) {
        return NULL;
    }

    return list;
}


void *ngx_list_push(ngx_list_t *l) {
    void             *elt;
    ngx_list_part_t  *last;
    
    last = l->last;

    if (last->nelts == l->nalloc) {

        /**
         * length equals capacity, the last part is full, 
         * allocate a new list part with same capacity and 
         * a length of zero 
         */

        last = ngx_palloc(l->pool, sizeof(ngx_list_part_t));
        if (last == NULL) {
            return NULL;
        }

        last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {
            return NULL;
        }

        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }

    elt = (char *) last->elts + l->size * last->nelts; /* get next element pointer */
    last->nelts++;                                     /* increment current part's length */

    return elt;
}