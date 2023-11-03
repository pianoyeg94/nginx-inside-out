#ifndef _NGX_ERRNO_H_INCLUDED_
#define _NGX_ERRNO_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef int               ngx_err_t; /* errno from <errno.h> is of type int */

#define NGX_EINTR         EINTR


#define ngx_errno                  errno
#define ngx_set_errno(err)         errno = err

#endif /* _NGX_ERRNO_H_INCLUDED_ */