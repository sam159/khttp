#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include "ut/utlist.h"

#include "util.h"
#include "log.h"
#include "config.h"
#include "data-buffer.h"

#include "server-connection.h"
#include "server-state.h"
#include "server-loop.h"
#include "http.h"
#include "http-body.h"

void* server_loop_write(void* arg) {
    thread *th = (thread*)arg;
    
    while(th->stop == false) {
        queue_item *item = queue_fetchone(th->pool->queue, true);
        if (item == NULL) {
            continue;
        }
        server_connection *conn = (server_connection*)item->data;
        CONN_LOCK(conn);
        
        if (conn->pending_writes->first != NULL) {
            errno = 0;
            if (data_buffer_list_writeto_fd(conn->pending_writes, conn->skt->fd) < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    item->blocked = true;
                } else {
                    char address[INET_ADDRSTRLEN];
                    skt_clientaddr(conn->skt, address, INET_ADDRSTRLEN);
                    warning(true, "[#%lu %s] write error", conn->id, address);
                    conn->skt->error = true;
                }
            }
        }
        if (conn->pending_writes->first == NULL) {
            http_response *response = http_response_list_next(conn->pending_responses);
            while (response != NULL) {
                char* resp_str = http_response_write(response);
                if (resp_str != NULL) {
                    data_buffer_list_append(conn->pending_writes, resp_str, strlen(resp_str));
                    free(resp_str);
                }
                if (conn->pending_writes->first != NULL) {
                    errno = 0;
                    if (data_buffer_list_writeto_fd(conn->pending_writes, conn->skt->fd) < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            item->blocked = true;
                        } else {
                            char address[INET_ADDRSTRLEN];
                            skt_clientaddr(conn->skt, address, INET_ADDRSTRLEN);
                            warning(true, "[#%lu %s] write error", conn->id, address);
                            conn->skt->error = true;
                        }
                    }
                }
                if (conn->skt->error == false && 
                    conn->pending_writes->first == NULL) {

                    if (response->send_status == SEND_BODY) {
                        if (response->body->type == BODY_NONE) {
                            response->send_status = SEND_DONE;
                        } else {
                            http_body_write_result result = http_body_writeto_fd(response->body, conn->skt->fd);
                            if (result == HBWRITE_DONE) {
                                response->send_status = SEND_DONE;  
                            } else if (result == HBWRITE_ERROR) {
                                conn->skt->error = true;
                                response->send_status = SEND_DONE;
                            } else if (result == HBWRITE_BLOCKED || result == HBWRITE_MORE) {
                                response = NULL;
                            }
                        }
                    }
                    if (response != NULL && response->send_status == SEND_DONE) {
                        
                        bool last = false;
                        http_header *connection_header = http_header_list_get(response->headers, HEADER_CONNECTION);
                        if (connection_header != NULL && strcasecmp(connection_header->content, "close") == 0) {
                            skt_close(conn->skt);
                            last = true;
                        }
                        
                        http_response_list_remove(conn->pending_responses, response);
                        http_response_delete(response);
                        if (last == true) {
                            response = NULL;
                            goto FINSIHED;
                        } else {
                            response = http_response_list_next(conn->pending_responses);
                        }
                    }
                }
            } //while response != null
        }//if no pending writes
        
        FINSIHED:
        
        CONN_UNLOCK(conn);
        queue_return_item(th->pool->queue, item, item->blocked == false);
    }
}