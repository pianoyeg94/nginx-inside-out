#ifndef _NGX_SHMTX_H_INCLUDED_
#define _NGX_SHMTX_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/**
 * Posix semaphore
 * ===============
 * A semaphore is an integer whose value is never allowed to fall below zero.
 * Two operations can be performed on semaphores: increment the semaphore value 
 * by one via `sem_post`; and decrement the semaphore value by one via `sem_wait`. 
 * If the value of a semaphore is currently zero, then a `sem_wait` operation will 
 * block until the value becomes greater than zero.
 * 
 * POSIX semaphores come in two forms: named semaphores and unnamed semaphores.
 * 
 * 
 * Named semaphores
 * ---------------------------------------------
 * A named semaphore is identified by a name of the form /somename; that is, a null-
 * terminated string of up to `NAME_MAX-4` (i.e., 251) characters consisting of an 
 * initial slash, followed by one or more characters, none of which are slashes. 
 * 
 * The `sem_open` function creates a new named semaphore or opens an existing named 
 * semaphore. After the semaphore has been opened, it can be operated on using 
 * `sem_post` and `sem_wait`. When a process has finished using the semaphore, it can 
 * use `sem_close` to close the semaphore. When all processes have finished using the 
 * semaphore, it can be removed from the system using `sem_unlink`.
 * 
 * 
 * Unnamed semaphores (memory-based semaphores)
 * ---------------------------------------------
 * An unnamed semaphore does not have a name. Instead the semaphore is placed in a region 
 * of memory that is shared between multiple threads (a thread-shared semaphore) or processes 
 * (a process-shared semaphore). A thread-shared semaphore is placed in an area of memory shared 
 * between the threads of a process, for example, a global variable. A process-shared semaphore 
 * must be placed in a shared memory region.
 * 
 * Before being used, an unnamed semaphore must be initialized using `sem_init`. It can then be 
 * operated on using `sem_post` and `sem_wait`. When the semaphore is no longer required, and 
 * before the memory in which it is located is deallocated, the semaphore should be destroyed 
 * using `sem_destroy`.
 * 
 * 
 * POSIX named semaphores have kernel persistence: if not removed by `sem_unlink`, a semaphore 
 * will exist until the system is shut down.
 * 
 * On Linux, named semaphores are created in a virtual file system, normally mounted under `/dev/shm`, 
 * with names of the form `sem.somename`. (This is the reason that semaphore names are limited to 
 * `NAME_MAX-4` rather than `NAME_MAX` characters.)
 * 
 * Since Linux 2.6.19, ACLs can be placed on files under this directory, to control object permissions 
 * on a per-user and per-group basis.
 * 
 * 
 * Notes
 * ---------------------------------------------
 * System V semaphores (`semget`, `semop`, etc.) are an older semaphore API. POSIX semaphores provide a 
 * simpler, and better designed interface than System V semaphores; on the other hand POSIX semaphores 
 * are less widely available (especially on older systems) than System V semaphores.
 */

/**
 * Container for `ngx_shmtx_t`s (process shared mutex) `lock` word 
 * and posix semaphore current waiters count `wait` word.
 * 
 * In case of 'ngx_shm_t' (process shared memory area) occupies
 * the first 2 words of the mmaped chunk to which 'ngx_shm_t'
 * points to through its `addr` attribute.
 * 
 * In case `ngx_slab_pool_t` is embedded at the start of the struct 
 * itself.
 */
typedef struct {
    ngx_atomic_t    lock;       /* volatile unsigned long */
#if (NGX_HAVE_POSIX_SEM)
    ngx_atomic_t    wait;       /* volatile unsigned long */
#endif
} ngx_shmtx_sh_t;


typedef struct {
#if (NGX_HAVE_ATOMIC_OPS)
    /**
     * Machine word used for the process shared mutex's 
     * spin lock part.
     * 
     * If this shared mutex is used with 'ngx_shm_t' 
     * (process shared memory area), points to the
     * 1st word of the mmaped chunk to which 'ngx_shm_t'
     * points to through its `addr` attribute.
     * 
     * If this shared mutex is used with `ngx_slab_pool_t`,
     * points to 1st word of the `ngx_slab_pool_t` struct.
     */
    ngx_atomic_t    *lock;      /* pointer to volatile unsigned long */
#if (NGX_HAVE_POSIX_SEM)
    /**
     * Machine word used for posix semaphore current waiters 
     * count `wait` word.
     * 
     * Used by mutex unlock routine to determine if it
     * needs to do a `sem_post` to release one of the 
     * waiters.
     * 
     * If this shared mutex is used with 'ngx_shm_t' 
     * (process shared memory area), points to the
     * 2nd word of the mmaped chunk to which 'ngx_shm_t'
     * points to through its `addr` attribute.
     * 
     * If this shared mutex is used with `ngx_slab_pool_t`,
     * points to 2nd word of the `ngx_slab_pool_t` struct.
     */
    ngx_atomic_t    *wait;      /* pointer to volatile unsigned long */

    /**
     * If set to 1, signals that the posix semaphore
     * was initialized and is ready for use.
     */
    ngx_uint_t       semaphore; 

    /**
     * Posix semaphore word.
     */
    sem_t            sem;       /* union of `char __size[__SIZEOF_SEM_T]` and `long int __align`, on 64-bit __SIZEOF_SEM_T is 4 bytes, size of union is 8 bytes */
#endif
#else 
    ngx_fd_t         fd; /* TODO!!!!! file-based lock? */
    u_char          *name;
#endif
    /**
     * Log base 2 of this value is used to calculate 
     * the amount of spin-lock passes on attempts to 
     * acquire this shared mutex, before blocking on 
     * the posix sempahore.
     * 
     * Each spin-lock pass starts from 1 call to the
     * assembly "PAUSE" instruction, the number of "PAUSE" 
     * instructions grows on each pass exponentially.
     */
    ngx_uint_t       spin;
} ngx_shmtx_t;


ngx_int_t ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr, u_char *name);
void ngx_shmtx_destroy(ngx_shmtx_t *mtx);
ngx_uint_t ngx_shmtx_trylock(ngx_shmtx_t *mtx);
void ngx_shmtx_lock(ngx_shmtx_t *mtx);
void ngx_shmtx_unlock(ngx_shmtx_t *mtx);
ngx_uint_t ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid);


#endif /* _NGX_SHMTX_H_INCLUDED_ */