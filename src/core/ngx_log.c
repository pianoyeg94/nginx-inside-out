#include <ngx_config.h>
#include <ngx_core.h>

static ngx_log_t        ngx_log;      /* TODO!!!!! isn't thread safe */
static ngx_open_file_t  ngx_log_file; /* TODO!!!!! isn't thread safe */


static ngx_str_t err_levels[] = {
    ngx_null_string,
    ngx_string("emerg"),
    ngx_string("alert"),
    ngx_string("crit"),
    ngx_string("error"),
    ngx_string("warn"),
    ngx_string("notice"),
    ngx_string("info"),
    ngx_string("debug")
};


#if (NGX_HAVE_VARIADIC_MACROS)

void ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, ngx_err_t err, const char *fmt, ...)

#else 

void ngx_log_error_core(ngx_uint_t level, ngx_log_t *log, ngx_err_t err, const char *fmt, va_list args)

#endif
{
#if (NGX_HAVE_VARIADIC_MACROS)
    va_list      args;
#endif
    u_char      *p, last, *msg;
    u_char       errstr[NGX_MAX_ERROR_STR];

    last = errstr + NGX_MAX_ERROR_STR;

    p = ngx_cpymem(errstr, ngx_cached_err_log_time.data, ngx_cached_err_log_time.len); /* TODO!!!!! */

    p = ngx_slprintf(p, last, " [%V] ", &err_levels[level]);

    /* pid#tid */
    p = ngx_slprintf(p, last, "%P#" NGX_TID_T_FMT ": ", ngx_log_pid, ngx_log_tid); /* TODO!!!!! */

    if (log->connection) {
        p = ngx_slprintf(p, last, "*%uA", log->connection); /* TODO!!!!! */
    }

    msg = p;

#if (NGX_HAVE_VARIADIC_MACROS)

    va_start(args, fmt);
    p = ngx_vslprintf(p, last, fmt, args);
    va_end(args);

#else

    p = ngx_vslprintf(p, last, fmt, args);

#endif

    if (err) {
        p = ngx_log_errno(p, last, err);
    }

    /* TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
}


void ngx_cdecl ngx_log_stderr(ngx_err_t err, const char *fmt, ...) {
    /**
     *  `p` - pointer into `errstr`, which points at different positions in the buffer durting this function's lifecycle;
     *  `last` - pointer to last byte in `errstr`
     */
    u_char   *p, *last; 
    /**
     * va_list macro
     * ===============
     *   - used in situations in which we need to access optional parameters and it's an argument list
     *   - represents a data object used to hold the parameters corresponding to the ellipsis part of the parameter list
     */
    va_list   args; /* pointer for variable arguments */
    u_char    errstr[NGX_MAX_ERROR_STR]; /* string buffer, which will eventually be written to stderr */

    last = errstr + NGX_MAX_ERROR_STR; 

    p = ngx_cpymem(errstr, "nginx: ", 7); /* `p` now points just after "nginx: " */

    /**
     * va_start macro
     * =============== 
     *   - will connect our argument list with some argument pointer
     *   - the list specified in va_list is the first argument and the second argument is the last fixed parameter
     */
    va_start(args, fmt); /* initialize argument position */
    p = ngx_vslprintf(p, last, fmt, args); /* after the call returns `p` will point into the next byte of errstr */
    /**
     * va_end macro
     * ===============
     *   - is used in situations when we would like to stop using the variable argument list (cleanup)
     */
    va_end(args);

    if (err) { /* underlying type is errno */
        p = ngx_log_errno(p, last, err); /* after the call returns `p` will point into the next byte of errstr */
    }

    if (p > last - NGX_LINEFEED_SIZE) { /* if there's no space left for a line feed */
        p = last - NGX_LINEFEED_SIZE; /* sacrifice last character of error message of `fmt` for line feed */
    }
    
    ngx_linefeed(p); /* `*p++ = LF;` */

    (void) ngx_write_console(ngx_stderr, errstr, p - errstr); /* inlined `write(fd, buf, n)` */
}

u_char *ngx_log_errno(u_char *buf, u_char *last, ngx_err_t err) {
    if (buf > last - 50) { /* if there's less than 50 characters available in the buffer for this function */

        /* leave a space for an error code  */
        
        /**
         * give at least 50 character for error code and error message, 
         * replace ending of the previous content with `...` 
         */
        buf = last - 50; 
        *buf++ = '.';
        *buf++ = '.';
        *buf++ = '.';
    }

    buf = ngx_slprintf(buf, last, " (%d: ", err); /* buf now points to next position after written content */
    buf = ngx_strerror(err, buf, last - buf); /* buf now points to next position after written error message */

    if (buf < last) { /* if there's space left in buffer, close the opening bracket written by `ngx_slprintf` */
        *buf++ = ')';
    }
    
    return buf; /* return pointer into buffer after all of the written content */
}

ngx_log_t *ngx_log_init(u_char *prefix, u_char *error_log) {
    /**
     * - *p : temporary variable for absolute or relative prefix of the log file name;
     * - *name : temporary variable for the log file name (absolute path or partial path (not relative))).
     */
    u_char *p, *name; 
    size_t nlen, plen; /* name and prefix strlen */

    ngx_log.file = &ngx_log_file; /* the file will be initialized bellow */
    ngx_log.log_level = NGX_LOG_NOTICE; /* TODO!!!!! default level? what does it log? */

    /**
     * If `-e` command line argument is not specified file — use the default file "logs/error.log".
     * An alternative error log file can be specified via `-e` command line argument to store the 
     * log instead of a default file. 
     * The special value stderr selects the standard error file - the caller will pass an empty string for stderr.
    */
    if (error_log == NULL) {
        error_log = (u_char *) NGX_ERROR_LOG_PATH; /* autogenerated default is "logs/error.log" or specified via --error-log-path at configure time before building nginx */
    }
    
    name = error_log; /* borrow reference for future manipulations (bellow) */
    nlen = ngx_strlen(name); 

    /**
     * If the `error_log` (`-e`) parameter wasn't specified by the caller of this function 
     * and --error-log-path=.stderr was specified at configure time before building nginx,
     * NGX_ERROR_LOG_PATH will be an empty string.
     * 
     * Or the `-e` command line argument had a valued of stderr (the caller will pass an empty string for stderr).
     */
    if (nlen == 0) { 
        ngx_log_file.fd = ngx_stderr;
        return &ngx_log; /* no need to set up ngx_log->ngx_log_file->name, since using stderr */
    }

    p = NULL; /* initialize `p` */

    
    /**
     * Passed in `error_log` path isn't an absolute path 
     * or user didn't supply and absolute path via -error-log-path at configure time before building nginx, 
     * this includes the autogenerated default "logs/error.log".
     */
    if (name[0] != '/') {
        /**
         * If the `prefix` parameter was supplied by the caller of this function, 
         * which in turn got it from the `-p` command line argument — set nginx path prefix, 
         * i.e. a directory that will keep server files (default value is /usr/local/nginx).
         */
        if (prefix) { 
            plen = ngx_strlen(prefix);
        } else { /* if the `prefix` parameter was NOT supplied by the caller of this function (no `-p` command line argument specified)  */
    /**
     * Use the `--path` configure option, 
     * the default, if not supplied at configure time before building nginx, is "/usr/local/nginx/", 
     * this is nginx's installation prefix.  
     */
    #ifdef NGX_PREFIX 
            prefix = (u_char *) NGX_PREFIX;
            plen = ngx_strlen(prefix);
    #else /* if "!" supplied to the `--path` configure option */
            plen = 0;
    #endif
        }

        /** 
         * Extend path to error log file with prefix, if there is one 
         * (passed in by the caller, specified at configure time or 
         * the "/usr/local/nginx/" installation prefix) 
         */
        if (plen) {
            /** 
             * Point `name` to memory that is capable of fitting the prefix and the log file name,
             * +2 bytes - to fit a possible path separator between the prefix and the log file name + terminating 0 character.
             */ 
            name = malloc(plen + nlen + 2); 
            if (name == NULL) {
                return NULL;
            }

            /**
             * Copies prefix to freshly malloced block of memory, 
             * which is able to accomodate the prefix and error log name, 
             * moves p to point just after the prefix string, 
             * casts to u_char pointer 
             */
            p = ngx_cpymem(name, prefix, plen);  

            if (!ngx_path_separator(*(p-1))) { /* if prefix defined by user doesn't end with a line separator ('/'), add it */
                *p++ = '/';
            }
            
            /**
             * Copy the passed in or specified at configure time error log name into `name` 
             * (just after the prefix) 
             */
            ngx_cpystrn(p, error_log, nlen + 1); 

            p = name; /* reset p to point at the start of the full error log name path */
        }
    }

    /**
     * Default access mode 0644 is:
     *     - (owning) User (creating process): read & write (0b110)
     *     - Group: read (0b100)
     *     - Other: read (0b100)
     */
    ngx_log_file.fd = ngx_open_file(name, NGX_FILE_APPEND, NGX_FILE_CREATE_OR_OPEN, NGX_FILE_DEFAULT_ACCESS);
    if (ngx_log_file.fd == NGX_INVALID_FILE) {
        /* log message, `errno` code and `errno` error message to stderr */
        ngx_log_stderr(ngx_errno, "[alert] could not open error log file: " ngx_open_file_n " \"%s\" failed", name); 
        ngx_log_file.fd = ngx_stderr;
    }

    if (p) {
        ngx_free(p);
    }

    return &ngx_log;
}