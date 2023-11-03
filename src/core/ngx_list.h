#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t;

struct ngx_list_part_s {
    void             *elts;  /* list part backing array */
    ngx_uint_t        nelts; /* list part length */
    ngx_list_part_t  *next;  /* pointer to next list part in singly-linked list of parts */
};


typedef struct {
    ngx_list_part_t  *last;   /* last part in possible singly-linked list of parts, also the current parent for append operations */
    ngx_list_part_t   part;   /* first part in possible singly-linked list of parts */
    size_t            size;   /* list element size */
    ngx_uint_t        nalloc; /* capacity of a single list part */
    ngx_pool_t       *pool;   /* pool, from which the list, all of it's parts and part backing arrays are allocated */
} ngx_list_t;



ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size) {
    list->part.elts = ngx_palloc(pool, n * size); /* first part's backing array */
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;     /* list part length */
    list->part.next = NULL;   /* start from a single part */
    list->last = &list->part; /* last part in singly-linked list of parts and also the current part for append operations */
    list->size = size;        /* list element size */
    list->nalloc = n;         /* capacity of a sinle part */
    list->pool = pool;        /* pool, from which the list, all of it's parts and part backing arrays are allocated */

    return NGX_OK;
}


/**
 * the iteration through the list:
 * 
 * part = &list.part; // get first part in singly-linked list of list parts
 * data = part->elts; // part's backing array
 * 
 * for (i = 0 ;; i++) {
 *  
 *     if (i>=part->nelts) {
 *        
 *         // iterated over all elements in current part, switch to next part if any
 *         
 *         if (part->next == NULL) {
 *             break;
 *         }
 * 
 *         part = part->next;
 *         data = part->elts;
 *         i = 0;
 *     }
 * 
 *     ... data[i] ...
 * }
 */


void *ngx_list_push(ngx_list_t *l);


#endif /* _NGX_LIST_H_INCLUDED_ */