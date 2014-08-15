#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include "http.h"
#include "server-connection.h"


server_connection* server_connection_new(skt_info *skt) {
    static uint64_t nextid = 1;
    assert(skt!=null);
    
    server_connection *conn = calloc(1, sizeof(server_connection));
    conn->id = __atomic_fetch_add(&nextid, 1, __ATOMIC_SEQ_CST);
    conn->last_activity = time(NULL);
    conn->parse_state = NULL;
    conn->pending_responses = http_response_list_new();
    conn->pending_writes = data_buffer_list_new();
    conn->skt = skt;
    conn->write_qid = 0;
    return conn;
}
void server_connection_delete(server_connection *conn) {
    assert(conn!=NULL);
    
    http_response_list_delete(conn->pending_responses);
    data_buffer_list_delete(conn->pending_writes);
    skt_delete(conn->skt);
    free(conn);
}