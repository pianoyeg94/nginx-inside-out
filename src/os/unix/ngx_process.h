#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_


#include <ngx_core.h>


typedef pid_t       ngx_pid_t;


#ifndef ngx_log_pid
#define ngx_log_pid  ngx_pid
#endif


#if (NGX_HAVE_SCHED_YIELD) 

/**
 * Causes the calling thread to relinquish the CPU.
 * The thread is moved to the end of the queue for its static
 * priority and a new thread gets to run.
 * 
 * In the Linux implementation, sched_yield() always succeeds.
 * 
 * If the calling thread is the only thread in the highest priority
 * list at that time, it will continue to run after a call to
 * sched_yield().
 */

#define ngx_sched_yield()   sched_yield()
#else

/**
 * Suspend execution for 1 microsecond.
 * 
 * The usleep() function suspends execution of the calling thread
 * for (at least) usec microseconds. The sleep may be lengthened
 * slightly by any system activity or by the time spent processing
 * the call or by the granularity of system timers.
 * 
 * When calling usleep the process/thread gives CPU to another 
 * process/thread for the given amount of time.
 */

#define ngx_sched_yield()   usleep(1)
#endif


#endif /* _NGX_PROCESS_H_INCLUDED_ */