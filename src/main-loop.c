#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <time.h>

#include <ut/utlist.h>
#include <errno.h>
#include <unistd.h>

#include "main-loop.h"
#include "mime.h"
#include "log.h"
#include "socket.h"
#include "thread-pool.h"
#include "queue.h"
#include "log.h"
#include "http.h"
#include "http-reader.h"
#include "server-socket.h"

hmain_connection* hmain_connection_new(int fd, hmain_status *status) {
    static u_int64_t nextid = 1;
    
    hmain_connection *conn = calloc(1, sizeof(hmain_connection));
    conn->cid = __atomic_fetch_add(&nextid, 1, __ATOMIC_SEQ_CST);
    conn->fd = fd;
    conn->status = status;
    conn->opened = time(NULL);
    conn->last_activity = conn->opened;
    conn->pending_responses = http_response_list_new();
    conn->pending_write = data_buffer_list_new();
    conn->mutex = calloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(conn->mutex, NULL);
    
    return conn;
}
void hmain_connection_close(hmain_connection *conn) {
    close(conn->fd);
    if (conn->pending_write != NULL) {
        data_buffer_list_delete(conn->pending_write);
    }
    //TODO: remove from all queues
}
void hmain_connection_delete(hmain_connection *conn) {
    LL_DELETE(conn->status->connections, conn);
    http_response_list_delete(conn->pending_responses);
    if (conn->pending_write != NULL) {
        data_buffer_list_delete(conn->pending_write);
    }
    pthread_mutex_destroy(conn->mutex);
    free(conn->mutex);
    free(conn->clientaddr);
    free(conn);
}

#define EVENT_IS(event, type) ((event->events & type) == type)
#define EVENT_ISNOT(event, type) (!EVENT_IS(event, type))

void hmain_setup(hmain_status *status) {
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
    status->sfd = server_socket_create();
    server_socket_listen(status->sfd, status->config->listen_port);
    
    //Open epoll socket
    status->epollfd = epoll_create1(0);
    if (status->epollfd < 0) {
        fatal("Failed to create epollfd");
    }
    
    //Register socket with epoll
    struct epoll_event svr_event;
    svr_event.data.fd = status->sfd;
    svr_event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(status->epollfd, EPOLL_CTL_ADD, status->sfd, &svr_event) < 0) {
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
    
    //Close all connections
    hmain_connection *elem;
    LL_FOREACH(status->connections, elem) {
        hmain_connection_close(elem);
    }
    
    //Close epoll
    close(status->epollfd);
    
    //Close the listening socket
    server_socket_release(status->sfd);
    
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
        int event_count = epoll_wait(status->epollfd, events, EPOLL_MAXEVENTS, 2000);
        for(int i=0; i<event_count; i++) {
            current_event = &events[i];
            
            if (EVENT_IS(current_event, EPOLLERR) ||
                EVENT_IS(current_event, EPOLLHUP) ||
                EVENT_ISNOT(current_event, EPOLLIN | EPOLLOUT)) {
                LOG(LERROR, "epoll; unexpected error or event");
                //TODO: close socket & cleanup
                
            } else if (EVENT_IS(current_event, EPOLLRDHUP)) {
                LOG(LINFO, "connection closed");
                hmain_connection *conn = (hmain_connection*)current_event->data.ptr;
                hmain_connection_close(conn);
                hmain_connection_delete(conn);
                //TODO: close socket & cleanup
            } else if (EVENT_IS(current_event, EPOLLIN)) {
                if (current_event->data.fd == status->sfd) {
                    //New connection(s)
                    while(true) {
                        struct epoll_event new_event;
                        int clientfd = accept4(status->sfd, (struct sockaddr*)clientaddr, &clientaddr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
                        new_event.events = EPOLLIN | EPOLLOUT | EPOLLHUP;
                        if (epoll_ctl(status->epollfd, EPOLL_CTL_ADD, clientfd, &new_event) < 0) {
                            fatal("Could not register new connection with epoll");
                        }
                        LL_APPEND(status->connections, conn);
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
                if (conn->isWriting == true || (conn->pending_responses == NULL && conn->pending_write == NULL)) {
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
    thread* th = (thread*)arg;
    
    while(th->stop==false) {
        queue_item *item = queue_fetchone(th->pool->queue, true);
        if (item == NULL) {
            continue;
        }
        hmain_connection *conn = (hmain_connection*)item->data;
        CONN_LOCK(conn);
        conn->isReading = true;
        
        char buffer[16*1024];
        int count;
        do {
            count = read(conn->fd, buffer, sizeof(buffer)/sizeof(buffer[0]));
            if (count < 0) {
                if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                    //Socket error
                    warning(true, "failed to read from connection #%lu", conn->cid);
                }
                break;
            }
            if (count == 0) {
                break;
            }
            
            LOG(LDEBUG, "Read %zu", count);
            
            //conn->pending_write = data_pool_appendbuffer(conn->pending_write, conn->status->buffer_pool, buffer, count);
            
            queue_add(conn->status->pools[POOL_WRITE]->queue, queue_item_new2("WRITE", (void*)conn));
            /*if (conn->parse_data == NULL) {
                conn->parse_data = calloc(1, sizeof(hmain_parse_data));
                conn->parse_data->parser = calloc(1, sizeof(http_parser));
                http_parser_init(conn->parse_data->parser, HTTP_REQUEST);
                conn->parse_data->parser->data = conn->parse_data;
                conn->parse_data->parser_header_state = HSTATE_NONE;
            }
            */
        } while(count > 0);
        conn->isReading = false;
        CONN_UNLOCK(conn);
    }
}
void* thloop_write(void * arg) {
    thread* th = (thread*)arg;
    
    while(th->stop==false) {
        queue_item *item = queue_fetchone(th->pool->queue, true);
        if (item == NULL) {
            continue;
        }
        hmain_connection *conn = (hmain_connection*)item->data;
        do {
            CONN_LOCK(conn);
            conn->isWriting = true;
        
            
        }while(0);
        conn->isWriting = false;
        CONN_UNLOCK(conn);
    }
}
void* thloop_disk_read(void * arg){
    
}
void* thloop_worker(void * arg) {
    
}