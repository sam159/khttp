/* 
 * File:   main-loop.h
 * Author: sam
 *
 * Created on 07 August 2014, 18:44
 */

#ifndef MAIN_LOOP_H
#define	MAIN_LOOP_H

#include <stdbool.h>
#include <time.h>
#include "config.h"
#include "thread-pool.h"
#include "http.h"

#ifdef	__cplusplus
extern "C" {
#endif
    
#define EPOLL_MAXEVENTS 128
    
    typedef enum hmain_pool {
        POOL_READ, POOL_WRITE, POOL_WORKERS, POOL_DISK_READ
    };
    
    typedef struct hmain_status {
        config_server *config;
        int sfd;
        int epollfd;
        thread_pool *pools[4];
        bool shutdown;
        hmain_connection *connections;
    } hmain_status;
    
    typedef struct hmain_connection {
        uint64_t cid;
        hmain_status *status;
        int fd;
        struct sockaddr_in* clientaddr;
        bool isReading, isWriting;
        time_t opened;
        time_t last_activity;
        http_response_list *pending_responses;
        hmain_connection *next;
    } hmain_connection;
    
    hmain_connection* hmain_connection_new(int fd, hmain_status *status);
    void hmain_connection_close(hmain_connection *conn);
    void hmain_connection_delete(hmain_connection *conn);
    
    void hmain_setup(hmain_status **statusptr);
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

