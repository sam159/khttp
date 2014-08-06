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
    
    typedef struct queue_item {
        struct queue_item *prev;
        struct queue_item *next;
        char tag[16];
        void *data;
    } queue_item;
    
    queue_item* queue_item_new();
    void queue_item_delete(queue_item *item);
    
    typedef struct queue {
        queue_item *list;
        uint64_t count;
        pthread_mutex_t *mutex;
        pthread_cond_t *cond;
   } queue;
   
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
   
   queue* queue_new();
   void queue_delete(queue *q);
   int queue_add(queue *q, queue_item *item);
   int queue_remove(queue *q, queue_item *item);
   queue_item* queue_fetchone(queue *q, bool blocking);
   void queue_clear(queue *q);
   void queue_ping(queue *q);
   size_t queue_count(queue *q);

#ifdef	__cplusplus
}
#endif

#endif	/* QUEUE_H */

