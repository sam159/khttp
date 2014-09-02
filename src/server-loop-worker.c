#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "ut/utlist.h"

#include "util.h"
#include "log.h"
#include "config.h"

#include "server-connection.h"
#include "server-state.h"
#include "server-loop.h"
#include "http.h"
#include "http-server.h"

void* server_loop_worker(void* arg) {
    thread *th = (thread*)arg;
    
    while(th->stop == false) {
        queue_item *item = queue_fetchone(th->pool->queue, true);
        if (item == NULL) {
            continue;
        }
        server_connection *conn = (server_connection*)item->data;
        CONN_LOCK(conn);
        
        while (conn->pending_requests != NULL) {
            http_request *request = conn->pending_requests;
            http_response *resp = server_process_request(conn->server->config, request);
            LL_DELETE(conn->pending_requests, request);
            http_request_delete(request);
            if (resp == NULL) {
                LOG(LERROR, "Request did not generate a response");
                resp = http_response_create_builtin(500, "Request did not complete successfully");
                http_header_list_add(resp->headers, http_header_new(HEADER_CONNECTION, "Close"), false);
            }
            http_response_list_append(conn->pending_responses, resp);
            CONN_ENQUEUE(conn, POOL_WRITE, "RESP");
        }
        
        CONN_UNLOCK(conn);
        queue_return_item(th->pool->queue, item, true);
    }
}