/* 
 * File:   server-connection.h
 * Author: sam
 *
 * Created on 15 August 2014, 18:07
 */

#ifndef SERVER_CONNECTION_H
#define	SERVER_CONNECTION_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>
#include <pthread.h>
    
#include "data-buffer.h"
#include "http.h"
#include "http-reader.h"
#include "server-state.h"
#include "socket.h"
    
#define CONN_LOCK(c) pthread_mutex_lock(&c->mutex)
#define CONN_UNLOCK(c) pthread_mutex_unlock(&c->mutex)

    typedef struct server_parse_status {
        http_request *current_request;
        bool request_complete;
        http_parser *parser;
        http_header *parser_current_header;
        int parser_header_state;
    } server_parse_status;
    
    typedef struct server_connection {
        uint64_t id;
        struct socket_info *skt;
        time_t last_activity;
        server_state *server;
        http_response_list *pending_responses;
        data_buffer_list *pending_writes;
        server_parse_status *parse_state; 
        struct server_connection *next;
        pthread_mutex_t mutex;
    } server_connection;
    
    server_connection* server_connection_new(socket_info *skt, server_state *state);
    void server_connection_delete(server_connection *conn);

#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_CONNECTION_H */

