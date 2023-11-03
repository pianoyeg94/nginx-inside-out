#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
/* TODO!!!!! */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

/* TODO!!!!!! */
#define NGX_DEFAULT_POOL_SIZE    (16 * 1024) /* 16KB */

/* TODO!!!!! */
#define NGX_POOL_ALIGNMENT       16


typedef void (*ngx_pool_cleanup_pt)(void *data);       /* TODO!!!!! */

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t; /* TODO!!!!! */

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler; /* TODO!!!!! */
    void                 *data;    /* TODO!!!!! */
    ngx_pool_cleanup_t   *next;    /* TODO!!!!! */
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;    /* TODO!!!!! */

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;    /* TODO!!!!! */
    void                 *alloc;   /* TODO!!!!! */
};


typedef struct {
    u_char               *last;    /* TODO!!!!! */
    u_char               *end;     /* TODO!!!!! */
    ngx_pool_t           *next;    /* TODO!!!!! */
    ngx_uint_t            failed;  /* TODO!!!!! */
} ngx_pool_data_t;


struct ngx_pool_s {
    ngx_pool_data_t       d;       /* TODO!!!!! */
    size_t                max;     /* TODO!!!!! */
    ngx_pool_t           *current; /* TODO!!!!! */
    ngx_chain_t          *chain;   /* TODO!!!!! */
    ngx_pool_large_t     *large;   /* TODO!!!!! */
    ngx_pool_cleanup_t   *cleanup; /* TODO!!!!! */
    ngx_log_t            *log;     /* TODO!!!!! */
};


typedef struct {
    ngx_fd_t              fd;      /* TODO!!!!! */
    u_char               *name;    /* TODO!!!!! */
    ngx_log_t            *log;     /* TODO!!!!! */
} ngx_pool_cleanup_file_t;


#endif /* _NGX_PALLOC_H_INCLUDED_ */