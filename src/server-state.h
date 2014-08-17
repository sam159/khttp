/* 
 * File:   server-state.h
 * Author: sam
 *
 * Created on 17 August 2014, 22:18
 */

#ifndef SERVER_STATE_H
#define	SERVER_STATE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "http.h"
#include "config.h"
#include "thread-pool.h"
#include "server-connection.h"
    
    typedef enum server_pool {
        POOL_READ, POOL_WRITE, POOL_WORKER, /*{*/THREADPOOL_NUM/*}must be last*/
    } server_pool;
    
    typedef struct server_status {
        config_server *config;
        bool started, stopped;
        bool shutdown_requested;
        int sfd;
        int epollfd;
        thread_pool *pools[THREADPOOL_NUM];
        server_connection *clients;
    } server_status;
    
    server_status* server_status_new(config_server *config);
    void server_status_delete(server_status *status);
    
    void server_start_pools(server_status *status);
    void server_stop_pools(server_status *status);

#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_STATE_H */

