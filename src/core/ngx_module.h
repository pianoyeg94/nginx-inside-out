#ifndef _NGX_MODULE_H_INCLUDED_
#define _NGX_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>


struct ngx_module_s {
    ngx_uint_t            ctx_index; /* TODO!!!!! */
    ngx_uint_t            index;     /* TODO!!!!! */

    char                 *name;

    ngx_uint_t            spare0;    /* TODO!!!!! */
    ngx_uint_t            spare1;    /* TODO!!!!! */

    ngx_uint_t            version;
    const char           *signature; /* TODO!!!!! */

    void                 *ctx;       /* TODO!!!!! */
};


#endif /* _NGX_MODULE_H_INCLUDED_ */