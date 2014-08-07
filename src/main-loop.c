#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>

#include "main-loop.h"
#include "mime.h"
#include "log.h"
#include "socket.h"
#include "thread-pool.h"
#include "queue.h"

void hmain_setup(hmain_status **statusptr) {
    hmain_status *status = *statusptr;
    
    if (status != NULL) {
        fatal("hmain already setup");
    }
    status = calloc(1, sizeof(hmain_status));
    
    //Start Logging
    log_register_add(log_new("stderr", stderr), true, LALL & ~(LINFO|LDEBUG));
    log_register_add(log_new("stdout", stdout), false, LDEBUG | LINFO);
    
    //Load mime types
    mime_init(NULL);
    
    //Load the config
    config_server *config = config_server_new();
    if (config_read_ini("khttpd.ini", config) < 0) {
        fatal("Could not read config");
    }
    status->config = config;
    
    //Open our listening socket
    status->sfd = svr_create();
    svr_setnonblock(status->sfd);
    svr_listen(status->sfd, status->config->listen_port);
    
    //Open epoll for socket
    status->epollfd = epoll_create1(0);
    if (status->epollfd < 0) {
        fatal("Failed to create epollfd");
    }
    
    //Register socket with epoll
    struct epoll_event svr_event;
    svr_event.data.fd = status->sfd;
    svr_event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(s->epollfd, EPOLL_CTL_ADD, status->sfd, &svr_event) < 0) {
        fatal("Could not register server socket with epoll");
    }
    
    //Create thread pools/queues
    thread_pool *pool = thread_pool_new("read", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 2;
    pool->func = thloop_read;
    status->pool.read = pool;
    
    pool = thread_pool_new("write", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 2;
    pool->func = thloop_write;
    status->pool.write = pool;
    
    pool = thread_pool_new("disk_read", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 2;
    pool->func = thloop_disk_read;
    status->pool.disk_read = pool;
    
    pool = thread_pool_new("worker", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 5;
    pool->func = thloop_worker;
    status->pool.workers = pool;
    
}
void hmain_teardown(hmain_status *status) {
    if (status == NULL) {
        fatal("hmain is not setup");
    }
    //Close epoll
    close(status->epollfd);
    
    //Close the listening connection
    svr_release(status->sfd);
    
    //Cleanup the mime detector
    mime_destroy();
    
    //Stop and remove all loggers
    log_register_clear();
    
    //Delete the config and the status
    config_server_delete(status->config);
    free(status);
}
void hmain_loop(hmain_status *status) {
    
}

void* thloop_read(void * arg) {
    
}
void* thloop_write(void * arg) {
    
}
void* thloop_disk_read(void * arg){
    
}
void* thloop_worker(void * arg) {
    
}