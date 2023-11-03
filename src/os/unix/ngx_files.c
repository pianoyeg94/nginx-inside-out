#include <ngx_config.h>
#include <ngx_core.h>



ssize_t ngx_read_file(ngx_file_t *file, u_char *buf, size_t size, off_t offset) {
    ssize_t n;

    /* TODO!!!!!: ngx_log_debug4(NGX_LOG_DEBUG_CORE, file->log, 0,
                   "read: %d, %p, %uz, %O", file->fd, buf, size, offset);  */

#if (NGX_HAVE_PREAD) /* from auto config, depends on the system on which nginx is configured, Linux has pread */
    /* https://man7.org/linux/man-pages/man2/pread.2.html , unistd.h */
    n = pread(file->fd, buf, size, offset);  /* the file offset is not changed, that's why when using pread we do not change the file->sys_offset */
    if (n == -1) {
        /* TODO!!!!! ngx_log_error(NGX_LOG_CRIT, file->log, ngx_errno,
                      "pread() \"%s\" failed", file->name.data); */
        return NGX_ERROR;
    }
#else 
/* TODO!!!!! */
#endif

    file-> offset += n;

    return n;
}
