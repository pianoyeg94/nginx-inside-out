#ifndef _NGX_FILE_H_INCLUDED_
#define _NGX_FILE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

struct ngx_file_s {
    ngx_fd_t                   fd;
    ngx_str_t                  name;
    ngx_file_info_t            info;

    off_t                      offset;
    off_t                      sys_offset; /* TODO!!!!! */

    ngx_log_t                 *log;

    
/* https://www.nginx.com/blog/thread-pools-boost-performance-9x/ */
/* should be compiled with the --with-threads argument to the configure script, which will 
   call the  /auto/threads script, doesn't work on windows, check it out,
   should also link with pthreads */
#if (NGX_THREADS || NGX_COMPAT)   /*          TODO!!!!! */
    ngx_int_t                (*thread_handler)(ngx_thread_task_t *task,
                                               ngx_file_t *file);
    void                      *thread_ctx;
    ngx_thread_task_t         *thread_task;
#endif

/* auto/unix script if [ $NGX_FILE_AIO = YES ] */
#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
    ngx_event_aio_t           *aio;
#endif

    unsigned                   valid_info:1; /* 1 bit bit field, TODO!!!!! */
    unsigned                   directio:1;   /* 1 bit bit field, bypass OS IO cache, TODO!!!!! */
};

#endif /* _NGX_FILE_H_INCLUDED_ */