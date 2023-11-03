#include <ngx_config.h>
#include <ngx_core.h>

/**
 * https://man7.org/linux/man-pages/man7/epoll.7.html
 * https://man7.org/linux/man-pages/man2/epoll_create1.2.html
 * https://idndx.com/tag/epoll/
 * https://habr.com/ru/company/ruvds/blog/523946/
 * https://habr.com/ru/post/416669/
 * https://habr.com/ru/company/infopulse/blog/415259/
 */

/* #if (NGX_TEST_BUILD_EPOLL) TODO!!!!! */

typedef struct {
    ngx_uint_t  events;
    ngx_uint_t  aio_requests;
} ngx_epoll_conf_t; /* TODO!!!!! */