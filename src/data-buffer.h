/* 
 * File:   data-buffer.h
 * Author: sam
 *
 * Created on 09 August 2014, 16:54
 */

#ifndef DATA_BUFFER_H
#define	DATA_BUFFER_H

#include <stdbool.h>
#include <pthread.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define BUFFER_LIST_WR_LOCK(l) data_buffer_list_lock(l, false, true)
#define BUFFER_LIST_WR_UNLOCK(l) data_buffer_list_unlock(l, false, true)
#define BUFFER_LIST_RD_LOCK(l) data_buffer_list_lock(l, true, false)
#define BUFFER_LIST_RD_UNLOCK(l) data_buffer_list_unlock(l, true, false)
    
#define DATA_BUFFER_SIZE 16*1024
    
    typedef struct data_buffer_list {
        struct data_buffer *first;
        pthread_mutex_t *wrlock, *rdlock;
    } data_buffer_list;
    
    typedef struct data_buffer {
        char* buffer;
        size_t size;
        size_t wOffset, rOffset;
        struct data_buffer *next;
    } data_buffer;
    
    data_buffer_list* data_buffer_list_new();
    void data_buffer_list_delete(data_buffer_list *list);
    int data_buffer_list_append(data_buffer_list *list, const char* src, size_t n);
    void data_buffer_list_lock(data_buffer_list *list, bool rd, bool wr);
    void data_buffer_list_unlock(data_buffer_list *list, bool rd, bool wr);
    
    data_buffer* data_buffer_new(size_t size);
    void data_buffer_free(data_buffer *buffer);
    

#ifdef	__cplusplus
}
#endif

#endif	/* DATA_BUFFER_H */

