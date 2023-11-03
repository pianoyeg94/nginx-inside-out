#ifndef _NGX_FILES_H_INCLUDED_
#define _NGX_FILES_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

typedef int                      ngx_fd_t;
typedef struct stat              ngx_file_info_t;

#define NGX_INVALID_FILE         -1

/**
 * ngx_open_file uses the default `int open(const char *pathname, int flags, mode_t mode)` function from fcntl.h 
 * but accepts the `create` flag not as part of the `flags` parameter
 * but as a separate parameter.
 */
#define ngx_open_file(name, mode, create, access)                            \
    open((const  char *) name, mode|create, access)

#define ngx_open_file_n          "open()" /* name of function for including in log messsages */

/**
 * O_CREAT
 *     If pathname does not exist, create it as a regular file.  *     
 *     The owner (user ID) of the new file is set to the
 *     effective user ID of the process.  *     
 *     The group ownership (group ID) of the new file is set
 *     either to the effective group ID of the process (System V
 *     semantics) or to the group ID of the parent directory (BSD
 *     semantics).  On Linux, the behavior depends on whether the
 *     set-group-ID mode bit is set on the parent directory: if
 *     that bit is set, then BSD semantics apply; otherwise,
 *     System V semantics apply.
 * 
 *     The mode argument specifies the file mode bits to be
 *     applied when a new file is created.  If neither O_CREAT
 *     nor O_TMPFILE is specified in flags, then mode is ignored
 *     (and can thus be specified as 0, or simply omitted).  The
 *     mode argument must be supplied if O_CREAT or O_TMPFILE is
 *     specified in flags; if it is not supplied, some arbitrary
 *     bytes from the stack will be applied as the file mode.
 *     
 *     The effective mode is modified by the process's umask in
 *     the usual way: in the absence of a default ACL, the mode
 *     of the created file is (mode & ~umask).
 *     
 *     Note that mode applies only to future accesses of the
 *     newly created file; the open() call that creates a read-
 *     only file may well return a read/write file descriptor.
 */
#define NGX_FILE_CREATE_OR_OPEN  O_CREAT
/**
 * O_APPEND
 *     The file is opened in append mode.  Before each write(2),
 *     the file offset is positioned at the end of the file, as
 *     if with lseek(2).  The modification of the file offset and
 *     the write operation are performed as a single atomic step.
 * 
 * File access mode
 *     Unlike the other values that can be specified in flags, the
 *     access mode values O_RDONLY, O_WRONLY, and O_RDWR do not specify
 *     individual bits.  Rather, they define the low order two bits of
 *     flags, and are defined respectively as 0, 1, and 2.  In other
 *     words, the combination O_RDONLY | O_WRONLY is a logical error,
 *     and certainly does not have the same meaning as O_RDWR.
 */
#define NGX_FILE_APPEND          (O_WRONLY|O_APPEND)

/**
 * Note that in C, an initial 0 indicates octal notation, just like 0x indicates hexadecimal notation. 
 * So every time you write plain zero in C, it's actually an octal zero.
 * 
 * 
 * 
 * There are 3x3 bit flags for a mode:
 *      1st triplet - (owning) User:
 *          - read
 *          - write
 *          - execute
 * 
 *      2nd triplet - Group:
 *          - read
 *          - write
 *          - execute
 * 
 *      3rd triplet - Other:
 *          - read
 *          - write
 *          - execute
 * 
 * So each triple encodes nicely as an octal digit.
 *      rwx oct    meaning
 *      --- ---    -------
 *      001 01   = execute
 *      010 02   = write
 *      011 03   = write & execute
 *      100 04   = read
 *      101 05   = read & execute
 *      110 06   = read & write
 *      111 07   = read & write & execute
 * 
 * So 0644 is:
 *      - (owning) User: read & write (0b110)
 *      - Group: read (0b100)
 *      - Other: read (0b100)
 * 
 * This might also be written:
 *  -rw-r--r--
 */
#define NGX_FILE_DEFAULT_ACCESS  0644


/*
 * we use inlined function instead of simple #define
 * because glibc 2.3 sets warn_unused_result attribute for write()
 * and in this case gcc 4.3 ignores (void) cast
 */
static ngx_inline ssize_t ngx_write_fd(ngx_fd_t fd, void *buf, size_t n) {
    return write(fd, buf, n);
}

#define ngx_write_fd_n           "write()"


#define ngx_write_console        ngx_write_fd


#define ngx_linefeed(p)          *p++ = LF;
#define NGX_LINEFEED_SIZE        1


#define ngx_path_separator(c)    ((c) == '/')

#define ngx_stderr               STDERR_FILENO

#endif /* _NGX_FILES_H_INCLUDED_ */