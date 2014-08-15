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
    
#include "data-buffer.h"
#include "http.h"
#include "http-reader.h"
#include "socket.h"

    typedef struct server_connection {
        uint64_t id;
        struct skt_info *skt;
        time_t last_activity;
        http_response_list *pending_responses;
        data_buffer_list *pending_writes;
        uint64_t write_qid;//item id in write queue
        request_parse_state *parse_state;
        struct server_connection *next;
    } server_connection;
    
    server_connection* server_connection_new(skt_info *skt);
    void server_connection_delete(server_connection *conn);

#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_CONNECTION_H */
