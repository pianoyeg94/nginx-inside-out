#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t; /* TODO!!!!! */

typedef struct ngx_buf_s  ngx_buf_t;

struct ngx_buf_s {
    u_char          *pos;        /* TODO!!!!!! */
    u_char          *last;       /* TODO!!!!!! */
    off_t            file_pos;   /* TODO!!!!!! */
    off_t            file_last;  /* TODO!!!!!! */

    u_char          *start;      /* start of buffer */
    u_char          *end;        /* end of buffer */
    ngx_buf_tag_t    tag;        /* TODO!!!!!! */
    ngx_file_t      *file;       /* TODO!!!!!! */
    ngx_buf_t       *shadow;     /* TODO!!!!!! */

    /* the buf's content could be changed */
    unsigned         temporary:1; /* TODO!!!!!! */

    /*
     * the buf's content is in a memory cache or in read only memory
     * and must not be changed
     */
    unsigned         memory:1;   /* TODO!!!!!! */


    /* the buf's content is mmap()ed and must not be changed */
    unsigned         mmap:1;      /* TODO!!!!!! */

    unsigned         recycled:1;       /* TODO!!!!!! */
    unsigned         in_file:1;        /* TODO!!!!!! */
    unsigned         flush:1;          /* TODO!!!!!! */
    unsigned         sync:1;           /* TODO!!!!!! */
    unsigned         last_buf:1;       /* TODO!!!!!! */
    unsigned         last_in_chain:1;  /* TODO!!!!!! */

    unsigned         last_shadow:1;    /* TODO!!!!!! */
    unsigned         temp_file:1;      /* TODO!!!!!! */

    /* STUB */ int   num;              /* TODO!!!!!! */
}; 


struct ngx_chain_s {                   /* TODO!!!!!! */
    ngx_buf_t    *buf;                 /* TODO!!!!!! */
    ngx_chain_t  *next;                /* TODO!!!!!! */
};


// TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))


#endif /* _NGX_BUF_H_INCLUDED_ */