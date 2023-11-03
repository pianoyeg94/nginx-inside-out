#include <ngx_config.h>
#include <ngx_core.h>


#if (NGX_HAVE_ATOMIC_OPS)


ngx_int_t ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr, u_char *name) {
    mtx->lock = &addr->lock; /* TODO!!!!! */

    if (mtx->spin == (ngx_uint_t) - 1) { /* TODO!!!!! */
        return NGX_OK; 
    }

#if (NGX_HAVE_POSIX_SEM)

    mtx->wait = &addr->wait; /* TODO!!!!! */

    /**
     * `sem_init` initializes the unnamed semaphore at the address pointed to 
     * by the first argument (`semt_t *sem`). 
     * 
     * The last argument (`unsigned int value`) specifies the initial value 
     * for the semaphore.
     * 
     * The middle argument (`int pshared`) indicates whether this semaphore 
     * is to be shared between the threads of a process, or between processes.
     * If `pshared` has the value 0, then the semaphore is shared between the 
     * threads of a process, and should be located at some address that is 
     * visible to all threads.
     * 
     * If `pshared` is nonzero, then the semaphore is shared between processes, 
     * and should be located in a region of shared memory  (Since a child created 
     * by `fork` inherits its parent's memory mappings, it can also access the 
     * semaphore.) Any process that can access the shared memory region can operate 
     * on the semaphore using `sem_post`, `sem_wait`, etc.
     * 
     * Returns 0 on success; on error, -1 is returned, and `errno` is set to indicate 
     * the error.
     * 
     * Errors:
     * EINVAL : value exceeds `SEM_VALUE_MAX`.
     * ENOSYS : `pshared` is nonzero, but the system does not support process-shared 
     *           semaphores.
     * 
     * TODO!!!!! is `&mtx->sem` from a shared memory pool?
     */
    if (sem_init(&mtx->sem, 1, 0) == -1) { /* initialize an unnamed process-shared semaphore using the address of `mtx->sem` to 0 */
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "sem_init failed"); /* TODO */
    } else {
        mtx->semaphore = 1; /* TODO!!!!! indicates that POSIX sempahore is being used? */
    }
#endif

    return NGX_OK;
}


void ngx_shmtx_destroy(ngx_shmtx_t *mtx) {
#if (NGX_HAVE_POSIX_SEM)

    if (mtx->semaphore) {

        /**
         * `sem_destroy` destroys the unnamed semaphore at the address pointed to by sem.
         * 
         * Only a semaphore that has been initialized by `sem_init` should be destroyed using 
         * `sem_destroy`.
         * 
         * Destroying a semaphore that other processes or threads are currently blocked on 
         * (in `sem_wait`) produces undefined behavior.
         * 
         * Using a semaphore that has been destroyed produces undefined results, until the 
         * semaphore has been reinitialized using `sem_init`.
         * 
         * Returns 0 on success; on error, -1 is returned, and `errno` is set to indicate the error.
         * 
         * Errors: 
         * EINVAL : `sem` is not a valid semaphore.
         * 
         * Notes: 
         * An unnamed semaphore should be destroyed with `sem_destroy` before the memory in which it 
         * is located is deallocated. Failure to do this can result in resource leaks on some implementations.
         */
        if (sem_destroy(&mtx->sem) == -1) {
            ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "sem_destory() failed");
        }
    }

#endif
}


ngx_uint_t ngx_shmtx_trylock(ngx_shmtx_t *mtx) {
    return (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid));
}


void ngx_shmtx_lock(ngx_shmtx_t *mtx) {
    ngx_uint_t         i, n;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx lock");

    for (;;) {

        if (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid)) {
            /* shared mutex is free */
            return;
        }

        if (ngx_ncpu > 1) { /* no point in spinning if machine has only one cpu */

            for (n = 1; n < mtx->spin; n <<= 1) { /* execute spin-wait loop for log base 2 of `mtx->spin` */

                for (i = 0; i < n; i++) { /* spinning cycles increase exponentially on each pass */
                    /**
                     * PAUSE notifies the CPU that this is a spinlock wait loop so memory 
                     * and cache accesses may be optimized. PAUSE may actually stop CPU 
                     * for some time to save power. Older CPUs decode it as NOP, so you 
                     * don't have to check if its supported. Older CPUs will simply do 
                     * nothing (NOP).
                     * 
                     * The intel manual and other information available state that:
                     *     The processor uses this hint to avoid the memory order violation in most situations,
                     *     which greatly improves processor performance.
                     *     For this reason, it is recommended that a PAUSE instruction be placed
                     *     in all spin-wait loops. An additional function of the PAUSE instruction
                     *     is to reduce the power consumed by Intel processors.
                     * 
                     * Branch mispredictions and pipeline flushes:
                     *     https://stackoverflow.com/questions/4725676/how-does-x86-pause-instruction-work-in-spinlock-and-can-it-be-used-in-other-sc
                     * 
                     * https://stackoverflow.com/questions/12894078/what-is-the-purpose-of-the-pause-instruction-in-x86
                     */
                    ngx_cpu_pause();
                }

                if (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid)) {
                    /* spinlock wait loop success, shared mutex was relased */
                    return;
                }
            }
        }

#if (NGX_HAVE_POSIX_SEM)

        /**
         * Two operations can be performed on semaphores: increment the semaphore value 
         * by one via `sem_post`; and decrement the semaphore value by one via `sem_wait`. 
         * If the value of a semaphore is currently zero, then a `sem_wait` operation will 
         * block until the value becomes greater than zero.
         */

        if (mtx->semaphore) {
            (void) ngx_atomic_fetch_add(mtx->wait, 1);

            if (*mtx->lock == 0 && ngx_atomic_cmp_set(mtx->lock, 0, ngx_pid)) {
                (void) ngx_atomic_fetch_add(mtx->wait, -1);
            }

            ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx wait %uA", *mtx->wait);

            while (sem_wait(&mtx->sem) == -1) {

                /* Retry if interrupted by signal hanlder, otherwise continue */
 
                ngx_err_t   err;

                err = ngx_errno;

                if (err != NGX_EINTR) {
                    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, err, "sem_wait() failed while waiting on shmtx");
                    break;
                }
            }

            ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx awoke");

            continue;
        }

#endif

        ngx_sched_yield();
    } 
}


void ngx_shmtx_unlock(ngx_shmtx_t *mtx) {
    if (mtx->spin != (ngx_uint_t) -1) {
        ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx unlock");
    }

    /**
     * Unlock the lock before waking up one of the processes possibly blocked 
     * on the posix sempahore (if any) 
     */
    if (ngx_atomic_cmp_set(mtx->lock, ngx_pid, 0)) {

        /**
         * If there are processes blocked on the posix semaphore,
         * derement the count of waiting processes, increment
         * semaphore word via sem_post(), which unblocks one of
         * the blocked procesess, semaphore word drops down to 0 
         * again. 
         * 
         * If there was only one process to wake up, the semaphore
         * word will remain equal to 0 after that process releases
         * the lock.
         */

        ngx_shmtx_wakeup(mtx);
    }
}

/* TODO!!!!! in what scenarios is this used? */
ngx_uint_t ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid) {
    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx forced unlock");

    /* TODO!!!!!! */
    if (ngx_atomic_cmp_set(mtx->lock, pid, 0)) {
        ngx_shmtx_wakeup(mtx);
        return 1;
    }

    return 0;
}


static void ngx_shmtx_wakeup(ngx_shmtx_t *mtx) {
#if (NGX_HAVE_POSIX_SEM)
    ngx_atomic_uint_t   wait;

    if (!mtx->semaphore) {
        /**
         * Should be set to 1 if unix semaphore word was 
         * initialized successfully in ngx_shmtx_create().
         * 
         * Set to 0, meaning that an error occured when
         * initializing the semaphore. 
         */
        return;
    }

    for ( ;; ) {
        /**
         * Get the number of processes currently blocked on this semaphore.
         * 
         * Can be a relaxed-atomic load, because it's part of a spin-lock pattern,
         * the sequential-atomic CAS bellow compensates for the current relaxed
         * load.
         */
        wait = *mtx->wait; 

        if ((ngx_atomic_int_t) wait <= 0) {
            /* no process currently blocked on semaphore, no one to wake up */
            return;
        }

        if (ngx_atomic_cmp_set(mtx->wait, wait, wait - 1)) {
            /**
             * Successfully decremented waiting processes count, can proceed to wake up one of the blocked processes 
             * (realy a thread).
             * If the Process Scheduling option is supported, the process (realy a thread) to be unblocked shall be 
             * chosen in a manner appropriate to the scheduling policies and parameters in effect for the blocked threads. 
             * In the case of  the schedulers SCHED_FIFO and SCHED_RR, the highest priority waiting thread shall be unblocked, 
             * and if there is more than one highest priority thread blocked waiting for the semaphore, then the highest priority 
             * thread that has been waiting the longest shall be unblocked. 
             * If the Process Scheduling option is not defined, the choice of a thread to unblock is unspecified.
             */
            break;
        }
    }

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0, "shmtx wake %uA", wait);

    /**
     * Increment semaphore word, waking up one of the blocked processes (realy thread).
     * If the Process Scheduling option is supported, the process (realy a thread) to be unblocked shall be 
     * chosen in a manner appropriate to the scheduling policies and parameters in effect for the blocked threads. 
     * In the case of  the schedulers SCHED_FIFO and SCHED_RR, the highest priority waiting thread shall be unblocked, 
     * and if there is more than one highest priority thread blocked waiting for the semaphore, then the highest priority 
     * thread that has been waiting the longest shall be unblocked. 
     * If the Process Scheduling option is not defined, the choice of a thread to unblock is unspecified.
     */
    if (sem_post(&mtx->sem) == -1) { 
        ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, ngx_errno, "sem_post() failed while wake shmtx");
    }

#endif
}


#endif