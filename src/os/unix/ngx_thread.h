#ifndef _NGX_THREAD_H_INCLUDED_
#define _NGX_THREAD_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

#if (NGX_THREADS)

    /* TODO!!!!! */

#else 

#define ngx_log_tid           0
#define NGX_TID_T_FMT         "%d"

#endif


#endif /* _NGX_THREAD_H_INCLUDED_ */