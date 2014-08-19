#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "ut/utlist.h"

#include "util.h"
#include "config.h"
#include "server-state.h"
#include "queue.h"
#include "thread-pool.h"
#include "server-connection.h"

server_state* server_status_new(config_server *config) {
    assert(config!=NULL);
    assert(config->host_count>0);
    assert(config->listen_port>0);
    
    server_state *status = calloc(1, sizeof(server_state));
    status->started = false;
    status->stopped = true;
    status->shutdown_requested = false;
    status->config = config;
    status->epollfd = status->sfd = 0;
    status->clients = NULL;
    
    return status;
}
void server_status_delete(server_state *status) {
    assert(status!=NULL);
    assert(status->stopped==true);
    assert(status->pools[0]==NULL);
    
    server_connection *elem, *tmp;
    LL_FOREACH_SAFE(status->clients, elem, tmp) {
        LL_DELETE(status->clients, elem);
        server_connection_delete(elem);
    }
    
    free(status);
}

void server_start_pools(server_state *status, thread_func pool_functions[]) {
    assert(status!=NULL);
    assert(status->pools[0]==NULL);
    
    //Create thread pools/queues
    thread_pool *pool = thread_pool_new("read", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 2;
    pool->func = pool_functions[POOL_READ];
    status->pools[POOL_READ] = pool;
    thread_pool_start(pool);
    
    pool = thread_pool_new("write", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 2;
    pool->func = pool_functions[POOL_WRITE];
    status->pools[POOL_WRITE] = pool;
    thread_pool_start(pool);
    
    pool = thread_pool_new("worker", queue_new());
    pool->min_threads = 1;
    pool->max_threads = 5;
    pool->func = pool_functions[POOL_WORKER];
    status->pools[POOL_WORKER] = pool;
    thread_pool_start(pool);
}
void server_stop_pools(server_state *status) {
    assert(status!=NULL);
    assert(status->pools[0]!=NULL);
    
    for(int i=0; i<THREADPOOL_NUM; i++) {
        thread_pool_stop(status->pools[i]);
        queue_delete(status->pools[i]->queue);
        thread_pool_stop(status->pools[i]);
    }
    memset(status->pools, 0, sizeof(status->pools));
}