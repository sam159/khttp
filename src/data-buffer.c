#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ut/utlist.h"
#include "data-buffer.h"
#include "util.h"

data_buffer_list* data_buffer_list_new() {
    data_buffer_list *list = calloc(1, sizeof(data_buffer_list));
    list->first = NULL;
    pthread_mutex_init(&list->rdlock, NULL);
    pthread_mutex_init(&list->wrlock, NULL);
    return list;
}
void data_buffer_list_delete(data_buffer_list *list) {
    assert(list!=NULL);
    pthread_mutex_destroy(&list->wrlock);
    pthread_mutex_destroy(&list->rdlock);
    data_buffer *elem, *tmp;
    LL_FOREACH_SAFE(list->first, elem, tmp) {
        LL_DELETE(list->first, elem);
        data_buffer_delete(elem);
    }
    free(list);
}
void data_buffer_list_append(data_buffer_list *list, const char* src, size_t n) {
    assert(list!=NULL);
    assert(src!=NULL && n>0);
    BUFFER_LIST_WR_LOCK(list);
    
    //Fetch last buffer in list, in case it has space left
    data_buffer *newbuf = NULL, *elem = NULL;
    bool first_is_new = false;
    LL_FOREACH(list->first, elem) {
        if (elem->next == NULL) {
            newbuf = elem;
        }
    }
    //Use a new buffer if list empty or last buffer is full
    if (newbuf == NULL || newbuf->wOffset == newbuf->size) {
        newbuf = data_buffer_new(DATA_BUFFER_SIZE);
        first_is_new = true;
    }
    //Add new buffers until we have enough allocated
    size_t allocated = newbuf->size - newbuf->wOffset;
    while(allocated < n) {
        data_buffer *buffer = data_buffer_new(DATA_BUFFER_SIZE);
        allocated += buffer->size;
        LL_APPEND(newbuf, buffer);
    }
    //Add data to the buffers
    size_t offset = 0;
    elem = NULL;
    LL_FOREACH(newbuf, elem) {
        size_t copy_count = n - offset;
        if (copy_count > (elem->size - elem->wOffset)) {
            copy_count = (elem->size - elem->wOffset);
        }
        memcpy(elem->buffer+elem->wOffset, src+offset, copy_count);
        offset += copy_count;
        elem->wOffset += copy_count;
    }
    //Don't re-append the last buffer
    if (first_is_new == false) {
        LL_DELETE(newbuf, newbuf);
    }
    LL_CONCAT(list->first, newbuf);
    BUFFER_LIST_WR_UNLOCK(list);
}
void data_buffer_list_lock(data_buffer_list *list, bool rd, bool wr) {
    assert(list != NULL);
    if (wr == true) pthread_mutex_lock(&list->wrlock);
    if (rd == true) pthread_mutex_lock(&list->rdlock);
}
void data_buffer_list_unlock(data_buffer_list *list, bool rd, bool wr) {
    assert(list != NULL);
    if (rd == true) pthread_mutex_unlock(&list->rdlock);
    if (wr == true) pthread_mutex_unlock(&list->wrlock);
}
ssize_t data_buffer_list_writeto_fd(data_buffer_list *list, int fd) {
    assert(list != NULL);
    
    ssize_t result = 0;
    
    BUFFER_LIST_RD_LOCK(list);
    data_buffer *next = list->first;
    while(next != NULL) {
        size_t write_count = next->wOffset - next->rOffset;
        if (write_count > 0) {
            ssize_t written = write(fd, next->buffer+next->rOffset, write_count);
            if (written <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    result = -1;
                    goto DONE;
                }
                if (written < 0) {
                    result = -1;
                    goto DONE;
                }
            }
            result += written;
            next->rOffset += written;
        } else {
            LL_DELETE(list->first, next);
            data_buffer_delete(next);
            next = list->first;
        }
    }
    
    DONE:
    BUFFER_LIST_RD_UNLOCK(list);
    
    return result;
}

data_buffer* data_buffer_new(size_t size) {
    assert(size > 0);
    
    data_buffer* buf = calloc(1, sizeof(data_buffer));
    ALLOC_CHECK(buf);
    buf->buffer = calloc(size, sizeof(char));
    ALLOC_CHECK(buf->buffer);
    buf->size = size;
    
    return buf;
}
void data_buffer_delete(data_buffer *buf) {
    assert(buf != NULL);
    
    free(buf->buffer);
    free(buf);
}
