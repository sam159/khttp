#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <assert.h>
#include <errno.h>

#include "util.h"
#include "log.h"
#include "config.h"

#include "server-connection.h"
#include "server-state.h"
#include "server-loop.h"
#include "ut/utlist.h"
#include "server-socket.h"

void server_loop(server_state *state) {
    assert(state!=NULL);
    assert(state->started==true);
    
    struct epoll_event *current_event;
    struct epoll_event *events = alloca(sizeof(struct epoll_event)*EP_MAXEVENTS);
    server_connection *conn;
    
    while(state->shutdown_requested == false) {
        int event_count = epoll_wait(state->epollfd, events, EP_MAXEVENTS, EP_WAIT_TIME);
        for(int i=0; i<event_count; i++) {
            current_event = &events[i];
            
            conn = NULL;
            server_connection *conn_elem;
            LL_FOREACH(state->clients, conn_elem) {
                if (conn_elem == current_event->data.ptr) {
                    conn = EP_CONN(current_event);
                }
            }
            
            if (EP_EVENT_IS(current_event, EPOLLERR) ||
                EP_EVENT_IS(current_event, EPOLLERR) ||
                    (
                        EP_EVENT_ISNOT(current_event, EPOLLIN) && 
                        EP_EVENT_ISNOT(current_event, EPOLLOUT) && 
                        EP_EVENT_ISNOT(current_event, EPOLLRDHUP)
                    )
                ) {
                if (current_event->data.fd == state->sfd) {
                    LOG(LERROR, "Error/Unexpected event on server socket");
                    state->shutdown_requested = true;
                    break;
                } else {
                    assert(conn!=NULL);
                    char* conn_addr = calloc(INET_ADDRSTRLEN, sizeof(char));
                    skt_clientaddr(conn->skt, conn_addr, INET_ADDRSTRLEN);
                    LOG(LWARNING, "Error on socket %lu [%s]", conn->id, conn_addr);
                    free(conn_addr);
                    skt_close(conn->skt);
                }
            } else if (EP_EVENT_IS(current_event, EPOLLRDHUP)) {
                assert(conn != NULL);
                skt_close(conn->skt);
            } else if (EP_EVENT_IS(current_event, EPOLLIN)) {
                if (current_event->data.fd == state->sfd) {
                    //New connections
                    while (true) {
                        socket_info *skt = server_socket_accept(state->sfd, SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if (skt == NULL) {
                            break;
                        }
                        server_connection *client = server_connection_new(skt, state);
                        
                        struct epoll_event new_event;
                        new_event.data.ptr = client;
                        new_event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
                        if (epoll_ctl(state->epollfd, EPOLL_CTL_ADD, skt->fd, &new_event) < 0) {
                            warning(true, "Failed to add connection to epoll");
                            skt_close(client->skt);
                            server_connection_delete(client);
                        } else {
                            LL_APPEND(state->clients, client);
                            queue_add(state->pools[POOL_READ]->queue, queue_item_new2("READ", (void*)client));
                        }
                    }
                } else {
                    assert(conn != NULL);
                    //Data available on connection
                    if (queue_unblock_byptr(state->pools[POOL_READ]->queue, (void*)conn) == 0) {
                        queue_add(state->pools[POOL_READ]->queue, queue_item_new2("READ", (void*)conn));
                    }
                }
            } else if (EP_EVENT_IS(current_event, EPOLLOUT)) {
                //Connection is now available to write to
                if (queue_unblock_byptr(state->pools[POOL_READ]->queue, (void*)conn) == 0) {
                    queue_add(state->pools[POOL_WRITE]->queue, queue_item_new2("WRITE", (void*)conn));
                }
            }
        }
        
        //Clean up closed connections
        server_connection *elem, *tmp;
        LL_FOREACH_SAFE(state->clients, elem, tmp) {
            if (elem->skt->error == true) {
                skt_close(elem->skt);
            }
            if (elem->skt->closed != true) {
                continue;
            }
            for(int i=0; i < THREADPOOL_NUM; i++) {
                queue_remove_byptr(state->pools[i]->queue, elem);
            }
            for(int i=0; i < THREADPOOL_NUM; i++) {
                queue_pending_waitptr(state->pools[i]->queue, elem);
            }
            LL_DELETE(state->clients, elem);
            char address[INET_ADDRSTRLEN] = {0};
            skt_clientaddr(elem->skt, address, INET_ADDRSTRLEN);
            LOG(LINFO, "[#%lu %s] Connection Closed", elem->id, address);
            server_connection_delete(elem);
        }
    }
    state->shutdown_requested = false;
    state->started = false;
    state->stopped = true;
}