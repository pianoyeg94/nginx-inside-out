#include <ngx_config.h>
#include <ngx_core.h>


#ifndef _NGX_QUEUE_H_INCLUDED_
#define _NGX_QUEUE_H_INCLUDED_

/**
 * ngx_queue_t is based on a cycled doubly-linked list,
 * meaning that the tail element always points back to 
 * the head and head always points back to the tail, 
 * or head and tail are the same.
 * 
 * TODO!!!!! is head and tail always the same?
 */
typedef struct ngx_queue_s  ngx_queue_t;

/**
 * ngx_queue_s is based on a cycled doubly-linked list,
 * meaning that the tail element always points back to 
 * the head and head always points back to the tail, 
 * or head and tail are the same.
 * 
 * TODO!!!!! is head and tail always the same?
 */
struct ngx_queue_s {
    /**
     * Queue tail in top-level queue or pointer to previous element in queue, 
     * when `ngx_queue_t` is "embedded" as a `queue` member in other structure.
     * 
     * Basically the queue elements do not store data, BUT data embeds `ngx_queue_t`
     * as its `queue` member, which is basically a doubly-linked list element with pointers
     * to previous and next elements. Data is retrieved by calculating the offset from this
     * embdedded structure to start address of the "owner" structure instance (via `offsetof`
     * macro from stddef.h, check out `ngx_queue_data` macro at the bottom of the file, which
     * does exactly this and then casts the resulting pointer to the required data type).
     * 
     * P.S. The offset is calculated at compile time by axpanding the `ngx_queue_data` macro,
     * which itself expands `stddef.h`'s `offsetof` macro. So during runtime queue element data
     * is accessed by taking the embedded queue instance's start address, applying an offset to it 
     * determined at compile time and casting the resulting address to the required pointer type  
     * expanded before compilation took place.
     */
    ngx_queue_t  *prev; 

    /**
     * Queue tail in top-level queue or pointer to previous element in queue, 
     * when `ngx_queue_t` is "embedded" as a `queue` member in other structure.
     * 
     * Check out comment to `prev` pointer abover for more details.
     */ 
    ngx_queue_t  *next;
};

/* sets queue's head and tail to passed in `ngx_queue_t` instance */
#define ngx_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


/* returns true if queue's passed in head is the same as it's tail */
#define ngx_queue_empty(h)                                                    \
    (h == (h)->prev)


/**
 * inserts `x` just after queue's head (strictly speaking 
 * inserts just after any element passed in as `h`) 
 */
#define ngx_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


/* inserts `x` just after queue's element `h` */
#define ngx_queue_insert_after   ngx_queue_insert_head

/**
 * inserts `x` after the current tail and makes it the new tail, 
 * makes passed in head point to the new tail and the new tail 
 * point back to head 
 */
#define ngx_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x;


/**
 * returns queue's head (strictly speaking just the 
 * next pointer of the passed in element). 
 */
#define ngx_queue_head(h)                                                     \
    (h)->next


/**
 * returns queue's tail (strictly speaking just the 
 * prev pointer of the passed in element). 
 */
#define ngx_queue_last(h)                                                     \
    (h)->prev


/* just returns passed in head (strictly speaking any element) */
#define ngx_queue_sentinel(h)                                                 \
    (h)


/* returns queue element which comes after the passed in one */
#define ngx_queue_next(q)                                                     \
    (q)->next


/* returns queue element which comes before the passed in one */
#define ngx_queue_prev(q)                                                     \
    (q)->prev


#if (NGX_DEBUG)

/* removes passed in element from queue */
#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL                                         

#else

/* removes passed in element from queue */
#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \

#endif


// #define ngx_queue_split(h, q, n)                              


#endif /* _NGX_QUEUE_H_INCLUDED_ */