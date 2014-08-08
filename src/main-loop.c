#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <time.h>

#include <ut/utlist.h>
#include <errno.h>

#include "main-loop.h"
#include "mime.h"
#include "log.h"
#include "socket.h"
#include "thread-pool.h"
#include "queue.h"
#include "log.h"

hmain_connection* hmain_connection_new(int fd, hmain_status *status) {
    static u_int64_t nextid = 1;
    
    hmain_connection *conn = calloc(1, sizeof(hmain_connection));
    conn->cid = __atomic_fetch_add(&u_int64_t, 1, __ATOMIC_SEQ_CST);
    conn->fd = fd;
    conn->status = status;
    conn->opened = time(NULL);
    conn->last_activity = conn->opened;
    conn->pending_responses = http_response_list_new();
    
    return conn;
}
void hmain_connection_close(hmain_connection *conn) {
    close(fd);
    //TODO: remove from all queues
}
void hmain_connection_delete(hmain_connection *conn) {
    http_response_list_delete(conn->pending_responses);
    free(conn->clientaddr);
    free(conn);
}

#define EVENT_IS(event, type) ((event.events & type) == type)
#define EVENT_ISNOT(event, type) (!EVENT_IS(event, type))

void hmain_setup(hmain_status **statusptr) {
    hmain_status *status = *statusptr;
    
    if (status != NULL) {
        fatal("hmain already setup");
    }
    status = calloc(1, sizeof(hmain_status));
    status->shutdown = false;
    
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
    status->pools[POOL_READ] = pool;
    thread_pool_start(pool);
    
    pool = thread_pool_new("write", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 2;
    pool->func = thloop_write;
    status->pools[POOL_WRITE] = pool;
    thread_pool_start(pool);
    
    pool = thread_pool_new("disk_read", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 2;
    pool->func = thloop_disk_read;
    status->pools[POOL_DISK_READ] = pool;
    thread_pool_start(pool);
    
    pool = thread_pool_new("worker", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 5;
    pool->func = thloop_worker;
    status->pools[POOL_WORKERS] = pool;
    thread_pool_start(pool);
    
    
}
void hmain_teardown(hmain_status *status) {
    if (status == NULL) {
        fatal("hmain is not setup");
    }
    //Stop pools
    size_t pool_count = sizeof(status->pools) / sizeof(status->pools[0]);
    for(int i=0; i<pool_count; i++) {
        thread_pool_stop(status->pools[i]);
        queue_delete(status->pools[i]->queue);
        thread_pool_delete(status->pools[i]);
    }
    
    //Close epoll
    close(status->epollfd);
    
    //Close the listening socket
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
    struct epoll_event *current_event;
    struct epoll_event *events = calloc(EPOLL_MAXEVENTS, sizeof(struct epoll_event));
    
    struct sockaddr_in* clientaddr = calloc(1, sizeof(struct sockaddr_in));
    socklen_t clientaddr_len = (socklen_t)sizeof(struct sockaddr_in);
    
    while(status->shutdown == false) {
        int event_count = epoll_wait(status->epollfd, events, EPOLL_MAXEVENTS, 1000);
        for(int i=0; i<event_count; i++) {
            current_event = &events[i];
            
            if (EVENT_IS(current_event, EPOLLERR) ||
                EVENT_IS(current_event, EPOLLHUP) ||
                EVENT_ISNOT(current_event, EPOLLIN)) {
                LOG(LERROR, "epoll; unexpected error or event");
                //TODO: close socket & cleanup
                
            } else if (EVENT_IS(current_event, EPOLLRDHUP)) {
                LOG(LINFO, "connection closed");
                //TODO: close socket & cleanup
            } else if (EVENT_IS(current_event, EPOLLIN)) {
                if (current_event->data.fd == status->sfd) {
                    //New connection(s)
                    while(true) {
                        struct epoll_event new_event;
                        int clientfd = accept4(status->sfd, (struct sockaddr*)clientaddr, clientaddr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if (clientfd < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                //Done with new connections
                                break;
                            } else {
                                warning(true, "Error accepting new connection");
                            }
                        }
                        hmain_connection *conn = hmain_connection_new(clientfd, status);
                        conn->clientaddr = calloc(1, sizeof(struct sockaddr_in));
                        memcpy(conn->clientaddr, clientaddr, clientaddr_len);
                        new_event.data.fd = clientfd;
                        new_event.data.ptr = conn;
                        new_event.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLET;
                        if (epoll_ctl(status->epollfd, EPOLL_CTL_ADD, status->sfd, &new_event) < 0) {
                            fatal("Could not register new connection with epoll");
                        }
                    }
                } else {
                    //Data is ready to read on existing connection
                    hmain_connection *conn = (hmain_connection*)current_event->data.ptr;
                    if (conn->isReading == true) {
                        continue;
                    }
                    queue_add(status->pools[POOL_READ]->queue, queue_item_new2("READ", (void*)conn));
                }
            } else if (EVENT_IS(current_event, EPOLLOUT)) {
                //Data can be written to connection
                hmain_connection *conn = (hmain_connection*)current_event->data.ptr;
                if (conn->isWriting == true || conn->pending_responses == NULL) {
                    continue;
                }
                queue_add(status->pools[POOL_WRITE]->queue, queue_item_new2("WRITE", (void*)conn));
            }
        }//for events
    }//while shutdown == false
    free(events);
    free(clientaddr);
}

void* thloop_read(void * arg) {
    
}
void* thloop_write(void * arg) {
    
}
void* thloop_disk_read(void * arg){
    
}
void* thloop_worker(void * arg) {
    
}