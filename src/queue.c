#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "queue.h"
#include "util.h"
#include "ut/utlist.h"

queue_item* queue_item_new() {
    queue_item *item = calloc(1, sizeof(queue_item));
    return item;
}
void queue_item_delete(queue_item *item) {
    free(item);
}

queue* queue_new() {
    queue *q = calloc(1, sizeof(queue));
    q->count = 0;
    q->list = NULL;
    q->mutex = calloc(1, sizeof(pthread_mutex_t));
    if (pthread_mutex_init(q->mutex, NULL) != 0) {
        fatal("Failed to init queue mutex");
    }
    q->cond = calloc(1, sizeof(pthread_cond_t));
    if (pthread_cond_init(q->cond, NULL) != 0) {
        fatal("Failed to init queue cond");
    }
    
    return q;
}
void queue_delete(queue *q) {
    queue_item *elem, *tmp;
    DL_FOREACH_SAFE(q->list, elem, tmp) {
        queue_item_delete(elem);
        DL_DELETE(q->list, elem);
    }
    pthread_mutex_destroy(q->mutex);
    free(q->mutex);
    pthread_cond_destroy(q->cond);
    free(q->cond);
    free(q);
}
int queue_add(queue *q, queue_item *item) {
    QUEUE_LOCK(q);
    DL_APPEND(q->list, item);
    q->count++;
    pthread_cond_signal(q->cond);
    QUEUE_UNLOCK(q);
    return 0;
}
int queue_remove(queue *q, queue_item *item) {
    QUEUE_LOCK(q);
    queue_item *elem, *tmp;
    DL_FOREACH_SAFE(q->list, elem, tmp) {
        if (elem == item) {
            DL_DELETE(q->list, elem);
            queue_item_delete(elem);
            break;
        }
    }
    q->count--;
    QUEUE_UNLOCK(q);
}
queue_item* queue_fetchone(queue *q, bool blocking) {
    queue_item *item = NULL;
    QUEUE_LOCK(q);
    if (q->count == 0 && blocking == true) {
        pthread_cond_wait(q->cond, q->mutex);
    }
    if (q->count > 0) {
        item = q->list;
        DL_DELETE(q->list, q->list);
        q->count--;
    }
    QUEUE_UNLOCK(q);
    return item;
}
void queue_clear(queue *q) {
    QUEUE_LOCK(q);
    queue_item *elem, *tmp;
    DL_FOREACH_SAFE(q->list, elem, tmp) {
        queue_item_delete(elem);
        DL_DELETE(q->list, elem);
    }
    pthread_cond_broadcast(q->cond);
    QUEUE_UNLOCK(q);
}
void queue_ping(queue *q) {
    QUEUE_LOCK(q);
    pthread_cond_broadcast(q->cond);
    QUEUE_UNLOCK(q);
}
size_t queue_count(queue *q) {
    size_t count;
    QUEUE_LOCK(q);
    count = q->count;
    QUEUE_UNLOCK(q);
    return count;
}