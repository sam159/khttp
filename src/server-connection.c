#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include "http_parser.h"
#include "http.h"
#include "http-reader.h"
#include "socket.h"
#include "server-state.h"
#include "server-connection.h"

server_connection* server_connection_new(socket_info *skt, server_state *state) {
    static uint64_t nextid = 1;
    assert(skt!=NULL);
    assert(state!=NULL);
    
    server_connection *conn = calloc(1, sizeof(server_connection));
    conn->id = __atomic_fetch_add(&nextid, 1, __ATOMIC_SEQ_CST);
    conn->last_activity = time(NULL);
    conn->server = state;
    conn->parse_state = server_parse_status_new();
    conn->pending_requests = NULL;
    conn->pending_responses = http_response_list_new();
    conn->pending_writes = data_buffer_list_new();
    conn->skt = skt;
    pthread_mutex_init(&conn->mutex, NULL);
    return conn;
}
void server_connection_delete(server_connection *conn) {
    assert(conn!=NULL);
    
    server_parse_status_delete(conn->parse_state);
    http_response_list_delete(conn->pending_responses);
    data_buffer_list_delete(conn->pending_writes);
    skt_delete(conn->skt);
    pthread_mutex_destroy(&conn->mutex);
    free(conn);
}

server_parse_status* server_parse_status_new() {
    server_parse_status *state = calloc(1, sizeof(server_parse_status));
    state->current_request = NULL;
    state->parser_current_header = NULL;
    state->request_complete = false;
    state->parser_header_state = HSTATE_NONE;
    state->parser = calloc(1, sizeof(http_parser));
    http_parser_init(state->parser, HTTP_REQUEST);
    
    return state;
}
void server_parse_status_delete(server_parse_status* state) {
    assert(state!=NULL);
    
    if (state->current_request != NULL) {
        http_request_delete(state->current_request);
    }
    if (state->parser != NULL) {
        free(state->parser);
    }
    if (state->parser_current_header != NULL) {
        http_header_delete(state->parser_current_header);
    }
    free(state);
}
void server_parser_status_reset(server_parse_status* state) {
    assert(state!=NULL);
    
    state->request_complete = false;
    state->parser_header_state = HSTATE_NONE;
    if (state->parser_current_header != NULL) {
        http_header_delete(state->parser_current_header);
        state->parser_current_header = NULL;
    }
    http_parser_init(state->parser, HTTP_REQUEST);
}