#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "util.h"
#include "ut/utlist.h"
#include "queue.h"

queue_item* queue_item_new() {
    static uint64_t nextid = 0;
    
    queue_item *item = calloc(1, sizeof(queue_item));
    item->id = __atomic_fetch_add(&nextid, 1, __ATOMIC_SEQ_CST);
    item->blocked = false;
    return item;
}
queue_item* queue_item_new2(char* tag, void* data) {
    queue_item *item = queue_item_new();
    item->tag[0] = '\0';
    strncat(item->tag, tag, (sizeof(item->tag)/sizeof(char))-1);
    item->data = data;
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
    if (item->blocked == false) {
        pthread_cond_signal(q->cond);
    }
    QUEUE_UNLOCK(q);
    return 0;
}
int queue_remove(queue *q, queue_item *item) {
    QUEUE_LOCK(q);
    int result = 0;
    queue_item *elem, *tmp;
    DL_FOREACH_SAFE(q->list, elem, tmp) {
        if (elem == item) {
            DL_DELETE(q->list, elem);
            queue_item_delete(elem);
            result = 1;
            break;
        }
    }
    q->count -= result;
    QUEUE_UNLOCK(q);
    return result;
}
int queue_remove_byptr(queue *q, void* ptr) {
    QUEUE_LOCK(q);
    
    int result = 0;
    queue_item *elem, *tmp;
    DL_FOREACH_SAFE(q->list, elem, tmp) {
        if (elem->data == ptr) {
            DL_DELETE(q->list, elem);
            queue_item_delete(elem);
            result ++;
        }
    }
    q->count -= result;
    QUEUE_UNLOCK(q);
    return result;
}
queue_item* queue_fetchone(queue *q, bool blocking) {
    queue_item *item = NULL;
    QUEUE_LOCK(q);
    if (q->count == 0 && blocking == true) {
        pthread_cond_wait(q->cond, q->mutex);
    }
    if (q->count > 0) {
        queue_item *elem;
        LL_FOREACH(q->list, elem) {
            if (elem->blocked == false) {
                item = elem;
                break;
            }
        }
        if (item != NULL) {
            DL_DELETE(q->list, item);
            q->count--;
        }
    }
    QUEUE_UNLOCK(q);
    return item;
}
void queue_unblock(queue *q, uint64_t itemid) {
    queue_item *item=NULL, *elem=NULL;
    QUEUE_LOCK(q);
    LL_FOREACH(q->list, elem) {
        if (elem->id == itemid) {
            if (elem->blocked == true) {
                elem->blocked = false;
                item = elem;
            }
            break;
        }
    }
    if (item != NULL) {
        pthread_cond_signal(q->cond);
    }
    QUEUE_UNLOCK(q);
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