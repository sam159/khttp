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
#include "queue.h"
#include "http_parser.h"
#include "http.h"
#include "http-reader.h"

#include "server-connection.h"
#include "server-state.h"
#include "server-loop.h"

void* server_loop_read(void* arg) {
    thread *th = (thread*)arg;
    
    while(th->stop == false) {
        queue_item *item = queue_fetchone(th->pool->queue, true);
        if (item == NULL) {
            continue;
        }
        server_connection *conn = (server_connection*)item->data;
        CONN_LOCK(conn);
        ssize_t count=0, totalread=0;
        char readbuf[16*1024];
        while(true) {
            count = read(conn->skt->fd, readbuf, sizeof(readbuf)/sizeof(readbuf[0]));
            if (count < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    item->blocked = true;
                    break;
                } else {
                    if (errno != EBADF) {
                        char address[INET_ADDRSTRLEN];
                        skt_clientaddr(conn->skt, address, INET_ADDRSTRLEN);
                        warning(true, "[#%lu %s] read error", conn->id, address);
                    }
                    conn->skt->error = true;
                    break;
                }
            }
            if (count == 0) {
                break;
            }
            totalread += count;
            bool error = false;
            size_t parsed_count = http_parser_execute(conn->parse_state->parser, parser_get_settings(), readbuf, count);
            if (conn->parse_state->parser->upgrade) {
                //No upgrades of the protocol are supported, send 501 not impl. back
                http_response *resp = http_response_create_builtin(501, "Protocol upgrade is not supported");
                http_header_list_add(resp->headers, http_header_new(HEADER_CONNECTION, "Close"), false);
                http_response_list_append(conn->pending_responses, resp);
                error = true;
            } else if (parsed_count != count || HTTP_PARSER_ERRNO(conn->parse_state->parser) != HPE_OK) {
                //Error parsing request
                http_response *resp = http_response_create_builtin(400, http_errno_description(HTTP_PARSER_ERRNO(conn->parse_state->parser)));
                http_header_list_add(resp->headers, http_header_new(HEADER_CONNECTION, "Close"), false);
                http_response_list_append(conn->pending_responses, resp);
                error = true;
            } else if (conn->parse_state->request_complete == true) {
                //Request has been read successfully, notify worker queue
                LL_APPEND(conn->pending_requests, conn->parse_state->current_request);
                server_parser_status_reset(conn->parse_state);
                queue_add(conn->server->pools[POOL_WORKER]->queue, queue_item_new2("REQ", (void*)conn));
            } else if (conn->parse_state->current_request != NULL && conn->parse_state->current_request->req!=NULL) {
                //Send 100 Continue message
                
            }
            if (error = true) {
                //Write any error directly, this will also close the connection
                queue_add(conn->server->pools[POOL_WRITE]->queue, queue_item_new2("RESP", (void*)conn));
                break;
            }
        }
        CONN_UNLOCK(conn);
        queue_return_item(th->pool->queue, item, item->blocked == false);
    }
}