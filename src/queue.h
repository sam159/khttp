/* 
 * File:   queue.h
 * Author: sam
 *
 * Created on 04 August 2014, 14:27
 */

#ifndef QUEUE_H
#define	QUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
    
#define QUEUE_ITEM_TAG_LEN  16
    
#define QUEUE_LOCK(q)                           \
    do {                                        \
        if (pthread_mutex_lock(q->mutex)!=0) {  \
            fatal("Could not lock queue");      \
        }                                       \
    } while(0)
#define QUEUE_UNLOCK(q)                         \
    do {                                        \
        if (pthread_mutex_unlock(q->mutex)!=0) {\
            fatal("Could not unlock queue");    \
        }                                       \
    } while(0)
    
#define QUEUE_PENDING_REMOVE(q, itemid)             \
    do {                                            \
        queue_pending_item *elem, *tmp;             \
        LL_FOREACH_SAFE(q->processing, elem, tmp) { \
            if (elem->qid == itemid) {              \
                LL_DELETE(q->processing, elem);     \
                free(elem);                         \
            }                                       \
        }                                           \
    } while(0)
#define QUEUE_HAS_PENDING(q, itemid, found)         \
    do {                                            \
        found = false;                              \
        queue_pending_item *elem;                   \
        LL_FOREACH(q->processing, elem) {           \
            if (elem->qid == itemid) {              \
                found = true;                       \
                break;                              \
            }                                       \
        }                                           \
    } while(0)
#define QUEUE_PENDING_COUNT(q, counter)         \
    do {                                        \
        queue_pending_item *elem;               \
        LL_COUNT(q->processing, elem, counter); \
    } while(0)
    
    typedef struct queue_item {
        uint64_t id;
        struct queue_item *prev;
        struct queue_item *next;
        char tag[QUEUE_ITEM_TAG_LEN];
        bool blocked;
        void *data;
    } queue_item;
    
    queue_item* queue_item_new();
    queue_item* queue_item_new2(char* tag, void* data);
    void queue_item_delete(queue_item *item);
    
    typedef struct queue_pending_item {
        uint64_t qid;
        struct queue_pending_item *next;
    } queue_pending_item;
    
    typedef struct queue {
        queue_item *list;
        uint64_t count;
        queue_pending_item *processing;
        pthread_mutex_t *mutex;
        pthread_cond_t *cond;
        pthread_cond_t *processing_cond;
   } queue;
   
   queue* queue_new();
   void queue_delete(queue *q);
   int queue_add(queue *q, queue_item *item);
   int queue_remove(queue *q, queue_item *item);
   int queue_remove_byptr(queue *q, void* ptr);
   queue_item* queue_fetchone(queue *q, bool blocking);
   void queue_unblock(queue *q, uint64_t itemid);
   void queue_clear(queue *q);
   void queue_ping(queue *q);
   size_t queue_count(queue *q);
   void queue_return_item(queue *q, queue_item *item, bool finished);
   void queue_pending_wait(queue *q, uint64_t itemid);

#ifdef	__cplusplus
}
#endif

#endif	/* QUEUE_H */

