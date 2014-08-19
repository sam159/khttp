/* 
 * File:   main-loop.h
 * Author: sam
 *
 * Created on 07 August 2014, 18:44
 */

#ifndef MAIN_LOOP_H
#define	MAIN_LOOP_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
    
    #define EPOLL_MAXEVENTS 128
    
#define CONN_LOCK(c) pthread_mutex_lock(c->mutex)
#define CONN_UNLOCK(c) pthread_mutex_unlock(c->mutex)
    
    typedef enum hmain_pool {
        POOL_READA, POOL_WRITEA, POOL_WORKERSA, POOL_DISK_READA
    } hmain_pool;
    
    //typedef enum skt_elem_hstate {HSTATE_NONE, HSTATE_VALUE, HSTATE_FIELD} skt_elem_hstate;
    
    typedef struct hmain_connection {
        uint64_t cid;
        struct hmain_status *status;
        int fd;
        struct sockaddr_in* clientaddr;
        bool isReading, isWriting;
        time_t opened;
        time_t last_activity;
        struct hmain_parse_data *parse_data;
        http_response_list *pending_responses;
        data_buffer_list *pending_write;
        struct hmain_connection *next;
        pthread_mutex_t *mutex;
        
    } hmain_connection;
    
    typedef struct hmain_status {
        config_server *config;
        int sfd;
        int epollfd;
        thread_pool *pools[4];
        bool shutdown;
        hmain_connection *connections;
    } hmain_status;
    
    typedef struct hmain_parse_data {
        http_request *current_request;
        bool request_complete;
        http_parser *parser;
        http_header *parser_current_header;
        int parser_header_state;
    } hmain_parse_data;
    
    hmain_connection* hmain_connection_new(int fd, hmain_status *status);
    void hmain_connection_close(hmain_connection *conn);
    void hmain_connection_delete(hmain_connection *conn);
    
    void hmain_setup(hmain_status *status);
    void hmain_teardown(hmain_status *status);
    void hmain_loop(hmain_status *status);

    void* thloop_read(void * arg);
    void* thloop_write(void * arg);
    void* thloop_disk_read(void * arg);
    void* thloop_worker(void * arg);

#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_LOOP_H */

