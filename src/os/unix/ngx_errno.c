#include <ngx_config.h>
#include <ngx_core.h>


/**
 * https://stackoverflow.com/questions/503878/how-to-know-what-the-errno-means
 * https://www.tutorialspoint.com/c_standard_library/string_h.htm
 * https://www.tutorialspoint.com/c_standard_library/c_function_strerror.htm
 * https://linux.die.net/man/3/strerror_r
 * https://manpages.ubuntu.com/manpages/impish/man3/strerror.3.html
 * https://www.gnu.org/software/gnulib/manual/html_node/strerrordesc_005fnp.html
 * https://www.gnu.org/software/libc/manual/html_node/Error-Messages.html
 * https://docs.oracle.com/cd/E88353_01/html/E37843/strerrorname-np-3c.html
 * 
 * 
 * Async-signal-safe functions
 * ============================
 * An async-signal-safe function is one that can be safely called
 * from within a signal handler.  Many functions are not async-
 * signal-safe.  In particular, nonreentrant functions are generally
 * unsafe to call from a signal handler.
 * 
 * The kinds of issues that render a function unsafe can be quickly
 * understood when one considers the implementation of the stdio
 * library, all of whose functions are not async-signal-safe.
 * 
 * When performing buffered I/O on a file, the stdio functions must
 * maintain a statically allocated data buffer along with associated
 * counters and indexes (or pointers) that record the amount of data
 * and the current position in the buffer.  Suppose that the main
 * program is in the middle of a call to a stdio function such as
 * printf(3) where the buffer and associated variables have been
 * partially updated.  If, at that moment, the program is
 * interrupted by a signal handler that also calls printf(3), then
 * the second call to printf(3) will operate on inconsistent data,
 * with unpredictable results.
 * 
 * To avoid problems with unsafe functions, there are two possible choices:
 *   1. Ensure that (a) the signal handler calls only async-signal-
 *      safe functions, and (b) the signal handler itself is reentrant
 *      with respect to global variables in the main program.
 * 
 *   2. Block signal delivery in the main program when calling
 *      functions that are unsafe or operating on global data that is
 *      also accessed by the signal handler.
 * 
 * Generally, the second choice is difficult in programs of any complexity, 
 * so the first choice is taken.
 * 
 * POSIX.1 specifies a set of functions that an implementation must
 * make async-signal-safe.  (An implementation may provide safe
 * implementations of additional functions, but this is not required
 * by the standard and other implementations may not provide the
 * same guarantees.)
 * 
 * In general, a function is async-signal-safe either because it is
 * reentrant or because it is atomic with respect to signals (i.e.,
 * its execution can't be interrupted by a signal handler).
 */


static ngx_str_t    ngx_unknown_error = ngx_string("Unknown error");


#if (NGX_HAVE_STRERRORDESC_NP)

/*
 * The strerrordesc_np() function, introduced in glibc 2.32, is
 * async-signal-safe. This makes it possible to use it directly,
 * without copying error messages.
 */


u_char *ngx_strerror(ngx_err_t err, u_char *errstr, size_t size) {
    size_t        len;
    const char   *msg;

    msg = strerrordesc_np(err); /* get string describing errno in a signal-safe fashion */

    if (msg == NULL) {
        msg = (char *) ngx_unknown_error.data;
        len = ngx_unknown_error.len;
    } else {
        len = ngx_strlen(msg);
    }

    /**
     *  - if length of string is less than the `size` provided, `size` becomes the length of the string
     *  - if length of string is greater than the `size` provided, error message will be capped by size, when copied to `errstr`
     */
    size = ngx_min(size, len); 

    return ngx_cpymem(errstr, msg, size); /* u_char pointer returned, points into errstr right after the copied string */
}


ngx_int_t ngx_strerror_init(void) {
    return NGX_OK;
}


#else

/*
 * The strerror() messages are copied because:
 *
 * 1) strerror() and strerror_r() functions are not Async-Signal-Safe,
 *    therefore, they cannot be used in signal handlers;
 *
 * 2) a direct sys_errlist[] array may be used instead of these functions,
 *    but Linux linker warns about its usage:
 *
 * warning: `sys_errlist' is deprecated; use `strerror' or `strerror_r' instead
 * warning: `sys_nerr' is deprecated; use `strerror' or `strerror_r' instead
 *
 *    causing false bug reports.
 * 
 * `sys_nerr` is defined as the number of entries in `sys_errlist`
 * 
 *  `strerror` should be preferred since new error values may not have been 
 *  added to `sys_errlist[]`
 * 
 * The global error list `sys_errlist[]` indexed by `errno` can be used to obtain the 
 * error message without the newline. The largest message number provided in the table is `sys_nerr` -1. 
 * Be careful when directly accessing this list because new error values may not have been added to `sys_errlist[]`.
 */


static ngx_str_t   *ngx_sys_errlist; /* holds copies of error messages returned by `strerror`, this is thread and signal-safe  */
static ngx_err_t    ngx_first_error; /* first available `errno` number, used in come cases to map `errno`s to indexes into `ngx_sys_errlist` */
static ngx_err_t    ngx_last_error;  /* last available `errno` number */


u_char *ngx_strerror(ngx_err_t err, u_char *errstr, size_t size) {
    ngx_str_t   *msg; /* pointer to error message in `ngx_sys_errlist` */

    if (err >= ngx_first_error && err < ngx_last_error) { /* check for valid `errno` */
        msg = &ngx_sys_errlist[err - ngx_first_error]; /* `(err - ngx_first_error) in case if `errno`s on the current system do not start from 1 */
    } else {
        mxg = &ngx_unknown_error;
    }

    /**
     *  - if length of string is less than the `size` provided, `size` becomes the length of the string
     *  - if length of string is greater than the `size` provided, error message will be capped by size, when copied to `errstr`
     */
    size = ngx_min(size, len); 

    return ngx_cpymem(errstr, msg->data, size); /* u_char pointer returned, points into errstr right after the copied string */
}


ngx_int_t ngx_strerror_init(void) {
    char       *msg; /* temporary storage for error messages returned by `strerror` */
    u_char     *p;   /* temporary storage for copies of error messages returned by `strerror` */
    size_t      len; /* will holds the size of `ngx_sys_errlist` in bytes */
    ngx_err_t   err; /* temporary storage for `errno`s */
    
#if (NGX_SYS_NERR) /* if the number of errors is known */

    /**
     * 
     * In this case `EPERM` `errno` will be equal to 1 
     * and will be contiguous with all `errno`s above it
     * up till the last `errno` which is determined 
     * by the value of `NGX_SYS_NERR`.
     * 
     * So in the end all `errno`s will directly correspond to
     * indexes in the `*ngx_sys_errlist` holding error messages,
     * no mapping of `errno`s to indexes will be required 
     * (done by using `ngx_first_error` as an offset).
     */
    ngx_first_error = 0;
    ngx_last_error = NGX_SYS_NERR;

#elif (EPERM > 1000 && EPERM < 0x7fffffff - 1000) 

    /*
     * If number of errors is not known, and EPERM error code has large
     * but reasonable value, guess possible error codes based on the error
     * messages returned by strerror(), starting from EPERM.  Notably,
     * this covers GNU/Hurd, where errors start at 0x40000001.
     */

    /**
     * In this case `EPERM` may be not the first `errno` in the list of `errno`s, 
     * so first check 999 possible conitguous `errno`s that may be be bellow it (backwards iteration).
     * 
     * 
     * This loop is used to find out the number of the first `errno` number,
     * which will also be used to map `errno`s to indexes into `*ngx_sys_errlist` 
     * holding error messages (done by using `ngx_first_error` as an offset).
     */
    for (err = EPERM; err > EPERM - 1000; err--) {
        ngx_set_errno(0); /* reset `errno` */
        msg = strerror(err); /* get error message for possible `errno` */

        /**
         * 1. errno == EINVAL : may be set by `strerror` implementation, because `err` on this iteration is an invalid `errno`;
         * 
         * 2. msg == NULL : an implementation of `strerror` may not set `errno` to `EINVAL` on an invalid argument, but just return a NULL pointer;
         * 
         * 3. strncmp(msg, "Unknown error", 13) == 0 : an implementation of `strerror` may return an "Unknown error" message if it received
         *                                             an invalid `errno` as an argument during this iteration.
         */
        if (errno == EINVAL || msg == NULL || strncmp(msg, "Unknown error", 13) == 0) {
            continue; /* `err` number not an `errno`, skip (`errno`s are usuaully conitguous, so at this point we're just out of valid `errno`s) */
        }

        ngx_first_error = err; /* in the end this will be the number corresponding to the first `errno` */
    }

    /**
     * Now find out the last `errno` - this loop servers exactly this purpose.
     * This loop is used to find out the number of the last `errno`.
     */
    for (err = EPERM; err < EPERM + 1000; err++) {
        ngx_set_errno(0); /* reset `errno` */
        msg = strerror(err); /* get error message for possible `errno` */

        /**
         * 1. errno == EINVAL : may be set by `strerror` implementation, because `err` on this iteration is an invalid `errno`;
         * 
         * 2. msg == NULL : an implementation of `strerror` may not set `errno` to `EINVAL` on an invalid argument, but just return a NULL pointer;
         * 
         * 3. strncmp(msg, "Unknown error", 13) == 0 : an implementation of `strerror` may return an "Unknown error" message if it received
         *                                             an invalid `errno` as an argument during this iteration.
         */
        if (errno == EINVAL || msg == NULL || strncmp(msg, "Unknown error", 13) == 0) {
            continue; /* `err` number not an `errno`, skip */
        }

        /**
         * In the end this will be by 1 greater than the last valid `errno`, 
         * will be used to check if an error is a valid `errno`, 
         * a valid `errno` should be less than this value.  
         */
        ngx_last_error = err + 1;
    }

#else

    /*
     * If number of errors is not known, guess it based on the error
     * messages returned by strerror().
     */

    /**
     * 
     * In this case `EPERM` `errno` will be equal to 1 
     * and will be contiguous with all `errno`s above it
     * up till the last `errno` which will be somewhere bellow 1000.
     * 
     * So in the end all `errno`s will directly correspond to
     * indexes in the `*ngx_sys_errlist` holding error messages,
     * no mapping of `errno`s to indexes will be required 
     * (done by using `ngx_first_error` as an offset).
     */
    ngx_first_error = 0; 

    /**
     * This loop is used to find out the number of the last `errno`.
     */
    for (err = 0; err < 1000; err++) {
        ngx_set_errno(0); /* reset `errno` */
        msg = strerror(err); /* get error message for possible `errno` */

        /**
         * 1. errno == EINVAL : may be set by `strerror` implementation, because `err` on this iteration is an invalid `errno`;
         * 
         * 2. msg == NULL : an implementation of `strerror` may not set `errno` to `EINVAL` on an invalid argument, but just return a NULL pointer;
         * 
         * 3. strncmp(msg, "Unknown error", 13) == 0 : an implementation of `strerror` may return an "Unknown error" message if it received
         *                                             an invalid `errno` as an argument during this iteration.
         */
        if (errno == EINVAL || msg == NULL || strncmp(msg, "Unknown error", 13) == 0) {
            continue; /* `err` number not an `errno`, skip (`errno`s are usuaully conitguous, so at this point we're just out of valid `errno`s) */
        }

        /**
         * In the end this will be by 1 greater than the last valid `errno`, 
         * will be used to check if an error is a valid `errno`, 
         * a valid `errno` should be less than this value.  
         */
        ngx_last_error = err + 1; 
    }

#endif

    /**
     * ngx_strerror() is not ready to work at this stage, therefore,
     * malloc() is used and possible errors are logged using strerror().
     */

    /* compute the required size in bytes of `ngx_sys_errlist`, so it can hold all error messages  */
    len = (ngx_last_error - ngx_first_error) * sizeof(ngx_str_t); 

    ngx_sys_errlist = malloc(len); /* allocate memory for `ngx_sys_errlist` */
    if (ngx_sys_errlist == NULL) {
        goto failed;
    }

    /* insert error message into `ngx_sys_errlist` for caculated range of `errno`s */
    for (err = ngx_first_error; err < ngx_last_error; err++) { 
        mgs = strerror(err);

        if (msg == NULL) {
            ngx_sys_errlist[err - ngx_first_error] = ngx_unknown_error;
            continue;
        }

        len = ngx_strlen(msg);

        p = malloc(len); /* allocate memory for copy of error message */
        if (p == NULL) {
            goto failed;
        }

        ngx_memcpy(p, msg, len); /* copy error message */

        /* insert copy of error message into `ngx_sys_errlist` at the appropriate index  */
        ngx_sys_errlist[err - ngx_first_error].len = len;
        ngx_sys_errlist[err - ngx_first_error].data = p;
    }

    return NGX_OK;

failed:

    err = errno; /* get `errno` which gives a hint about the occured error */

    /**
     * First argument is 0 - we cannot pass the `errno` directly as the `err` argument,
     *                       because we failed to set up `ngx_sys_errlist`.
     * 
     * %uz - is a format specifier known to nginx's `ngx_vslprintf` functions. 
     *       Stands for `size_t`.
     */
    ngx_log_stderr(0, "malloc(%uz) failed (%d: %s)", len, err, strerror(err));

    return NGX_ERROR;

}

#endif