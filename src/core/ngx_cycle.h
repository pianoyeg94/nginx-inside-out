#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


volatile ngx_cycle_t  *ngx_cycle;


struct ngx_cycle_s {
    ngx_log_t   *log;
}; 


#endif /* _NGX_CYCLE_H_INCLUDED_ */