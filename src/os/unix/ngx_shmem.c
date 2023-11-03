#include <ngx_config.h>
#include <ngx_core.h>


#if (NGX_HAVE_MAP_ANON)

ngx_int_t ngx_shm_alloc(ngx_shm_t *shm) {
    shm->addr = (u_char *) mmap(NULL, shm->size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);

    if (shm->addr = MAP_FAILED) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno, "mmap(MAP_ANON|MAP_SHARED, %uz) failed", shm->size);
        return NGX_ERROR;
    }

    return NGX_OK;
}


void ngx_shm_free(ngx_shm_t *shm) {
    if (munmap((void *) shm->addr, shm->size) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno, "munmap(%p, %uz) failed", shm->addr, shm->size);
    }
}

#elif (NGX_HAVE_MAP_DEVZERO)

ngx_int_t ngx_shm_alloc(ngx_shm_t *shm) {
    ngx_fd_t  fd;

    /**
     * /dev/zero is a special file in Unix-like operating systems that provides as many 
     * null characters (ASCII NUL, 0x00) as are read from it. One of the typical uses is 
     * to provide a character stream for initializing data storage.
     * 
     * /dev/zero was introduced in 1988 by SunOS-4.0 in order to allow a mappable BSS segment 
     * for shared libraries using anonymous memory. HP-UX 8.x introduced the MAP_ANONYMOUS flag 
     * for mmap(), which maps anonymous memory directly without a need to open /dev/zero. Since 
     * the late 1990s, MAP_ANONYMOUS or MAP_ANON are supported by most UNIX versions, removing 
     * the original purpose of /dev/zero.
     */
    fd = open("/dev/zero", O_RDWR);    

    if (fd == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno, "open(\"/dev/zero\") failed");
        return NGX_ERROR;
    }

    shm->addr = (u_char *) mmap(NULL, shm->size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if (shm->addr == MAP_FAILED) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno, "mmap(/dev/zero, MAP_SHARED, %uz) failed", shm->size);
    }

    if (close(fd) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno, "close(\"/dev/zero\") failed");
    }

    return (shm->addr == MAP_FAILED) ? NGX_ERROR : NGX_OK;
}


void ngx_shm_free(ngx_shm_t *shm) {
    if (munmap((void *) shm->addr, shm->size) == -1) {
        ngx_log_error(NGX_LOG_ALERT, shm->log, ngx_errno, "munmap(%p, %uz)", shm->addr, shm->size);
    }
}

#elif (NGX_HAVE_SYSVSHM)

#include <sys/ipc.h>
#include <sys/shm.h>

// https://man7.org/linux/man-pages/man7/sysvipc.7.html
// https://man7.org/linux/man-pages/man2/shmget.2.html
// https://www.softprayog.in/programming/interprocess-communication-using-system-v-shared-memory-in-linux

// TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#endif

