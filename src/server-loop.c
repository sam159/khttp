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

void server_loop(server_status *state) {
    assert(state!=NULL);
    assert(state->started==true);
    
    struct epoll_event *current_event;
    struct epoll_event *events = alloca(sizeof(struct epoll_event)*EP_MAXEVENTS);
    struct server_connection *conn;
    
    while(state->shutdown_requested == false) {
        int event_count = epoll_wait(state->epollfd, events, EP_MAXEVENTS, EP_WAIT_TIME);
        for(int i=0; i<event_count; i++) {
            current_event = &events[i];
            conn = current_event->data.ptr != NULL ? EP_CONN(current_event) : NULL;
            
            if (EP_EVENT_IS   (current_event, EPOLLERR | EPOLLHUP) ||
                EP_EVENT_ISNOT(current_event, EPOLLIN | EPOLLOUT | EPOLLRDHUP)) {
                if (conn == NULL) {
                    fatal("Unexpected error on unknown socket");
                } else if (conn->skt->fd == state->sfd) {
                    LOG(LWARNING, "Error/Unexpected event on server socket");
                    state->shutdown_requested = true;
                    break;
                } else {
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
                assert(conn != NULL);
                if (conn->skt->fd == state->sfd) {
                    //New connections
                    while (true) {
                        socket_info *skt = server_socket_accept(state->sfd, SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if (skt == NULL) {
                            break;
                        }
                        server_connection *client = server_connection_new(skt);
                        struct epoll_event new_event;
                        new_event.data.ptr = client;
                        new_event.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLET;
                        if (epoll_ctl(state->epollfd, EPOLL_CTL_ADD, client->skt->fd, &new_event) < 0) {
                            warning(true, "Failed to add connection to epoll");
                            skt_close(client->skt);
                            server_connection_delete(client);
                        }
                        LL_APPEND(state->clients, client);
                    }
                } else {
                    assert(conn != NULL);
                    //Data available on connection
                    queue_add(state->pools[POOL_READ]->queue, (void*)conn);
                }
            } else if (EP_EVENT_IS(current_event, EPOLLOUT)) {
                //Connection is now available to write to
                queue_add(state->pools[POOL_WRITE]->queue, (void*)conn);
            }
        }
        
        //Clean up closed connections
        server_connection *elem, *tmp;
        LL_FOREACH(state->clients, elem) {
            if (elem->skt->closed != true) {
                continue;
            }
            for(int i=0; i < THREADPOOL_NUM; i++) {
                queue_remove_byptr(state->pools[i]->queue, elem);
            }
            for(int i=0; i < THREADPOOL_NUM; i++) {
                queue_pending_waitptr(state->pools[i]->queue, elem);
            }
            server_connection_delete(elem);
        }
    }
    state->shutdown_requested = false;
    state->started = false;
    state->stopped = true;
}