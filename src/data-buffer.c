#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

#include "ut/utlist.h"
#include "data-buffer.h"

data_buffer_list* data_buffer_list_new() {
    data_buffer_list *list = calloc(1, sizeof(data_buffer_list));
    list->first = NULL;
    list->wrlock = calloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(list->wrlock, NULL);
    list->rdlock = calloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(list->rdlock, NULL);
    return list;
}
void data_buffer_list_delete(data_buffer_list *list) {
    assert(list!=NULL);
    pthread_mutex_destroy(list->wrlock);
    pthread_mutex_destroy(list->rdlock);
    data_buffer *elem, *tmp;
    LL_FOREACH_SAFE(list->first, elem, tmp) {
        LL_DELETE(list->first, elem);
        data_buffer_free(elem);
    }
    free(list->wrlock);
    free(list->rdlock);
    free(list);
}
void data_buffer_list_append(data_buffer_list *list, const char* src, size_t n) {
    assert(list!=NULL);
    assert(src!=NULL && n>0);
    BUFFER_LIST_WR_LOCK(list);
    
    int blocks = 1;
    data_buffer *newbuf = data_buffer_new(DATA_BUFFER_SIZE);
    while(blocks * DATA_BUFFER_SIZE < n) {
        blocks++;
        LL_PREPEND(newbuf, data_buffer_new(DATA_BUFFER_SIZE));
    }
    
    size_t offset = 0;
    data_buffer *elem;
    LL_FOREACH(newbuf, elem) {
        size_t copy_count = n - offset;
        if (copy_count > elem->size) {
            copy_count = elem->size;
        }
        memcpy(elem->buffer, src+offset, copy_count);
        offset += copy_count;
        elem->wOffset += copy_count;
    }
    
    LL_CONCAT(list->first, newbuf);
    BUFFER_LIST_WR_UNLOCK(list);
}
void data_buffer_list_lock(data_buffer_list *list, bool rd, bool wr) {
    assert(list != NULL);
    if (rd == true) pthread_mutex_lock(list->rdlock);
    if (wr == true) pthread_mutex_lock(list->wrlock);
}
void data_buffer_list_unlock(data_buffer_list *list, bool rd, bool wr) {
    assert(list != NULL);
    if (rd == true) pthread_mutex_unlock(list->rdlock);
    if (wr == true) pthread_mutex_unlock(list->wrlock);
}

data_buffer* data_buffer_new(size_t size) {
    assert(size > 0);
    
    data_buffer* buffer = calloc(1, sizeof(data_buffer));
    buffer->buffer = calloc(size, sizeof(char));
    buffer->size = size;
    
    return buffer;
}
void data_buffer_free(data_buffer *buffer) {
    assert(buffer != NULL);
    
    free(buffer->buffer);
    free(buffer);
}
