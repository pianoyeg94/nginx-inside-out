#ifndef _NGX_ARRAY_H_INCLUDED_
#define _NGX_ARRAY_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct {
    void        *elts;   /* pointer to start of the pooled chunk of memory used for the actual backing array */
    ngx_uint_t   nelts;  /* current length of the dynamic array, number of elements currently stored in the array */
    size_t       size;   /* size of elements stored in this dynamic array  */
    ngx_uint_t   nalloc; /* currernt capacity of the dynamic array */
    ngx_pool_t  *pool;   /* pool from which this dynamic array type and its backing array is allocated */
} ngx_array_t;


ngx_array_t *ngx_array_create(ngx_pool_t *p, ngx_uint_t n, size_t size);
void ngx_array_destroy(ngx_array_t *a);
void *ngx_array_push(ngx_array_t *a);
void *ngx_array_push_n(ngx_array_t *a, ngx_uint_t n);


static ngx_inline ngx_int_t ngx_array_init(ngx_array_t *array, ngx_pool_t *pool, ngx_uint_t n, size_t size) {
    /**
     * set "array->nelts" before "array->elts", otherwise MSVC (Microsoft Visual C++ compiler) 
     * thinks that "array->nelts" may be used without having been initialized.
     */

    array->nelts = 0;   /* no elements yet stored in the array */
    array->size = size; /* size of elements stored in this array TODO!!!!! is it so? */
    array->nalloc = n;  /* capacity of the array TODO!!!!! current or set in stone? */
    array->pool = pool; /* TODO!!!!! possibly for reallocation? */

    array->elts = ngx_palloc(pool, n * size); /* allocate a contiguos memory chunk from pool to store `size` `n`-sized elements */
    if (array->elts == NULL) {
        return NGX_ERROR; 
    }

    return NGX_OK;
}


#endif /* _NGX_ARRAY_H_INCLUDED_ */