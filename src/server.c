#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"
#include "util.h"
#include "mime.h"
#include "queue.h"
#include "thread-pool.h"
#include "http_parser.h"
#include "http.h"
#include "http-reader.h"
#include "config.h"

#include "server-socket.h"
#include "server-state.h"
#include "server-connection.h"
#include "server-loop.h"
#include "server.h"

void server_start(server_state *status) {
    assert(status!=NULL);
    assert(status->stopped==true);
    
    status->shutdown_requested = false;
    
    //Start Logging
    log_register_add(log_new("stderr", stderr), true, LALL & ~(LINFO|LDEBUG));
    log_register_add(log_new("stdout", stdout), false, LDEBUG | LINFO);
    
    //Load mime types
    mime_init(NULL);
    
    //Open the server socket
    status->sfd = server_socket_create();
    server_socket_listen_epoll(status->sfd, status->config->listen_port, &status->epollfd);
    
    //Start thread pools
    thread_func pool_functions[] = {
        [POOL_READ] = server_loop_read,
        [POOL_WRITE] = server_loop_write,
        [POOL_WORKER] = server_loop_worker
    };
    server_start_pools(status, pool_functions);
    
    status->started = true;
    status->stopped = false;
    
    //Start the main loop
    server_loop(status);
    
    //Cleanup after the loop exits
    server_teardown(status);
}
void server_teardown(server_state *status) {
    assert(status!=NULL);
    assert(status->stopped==true);
    
    //Stop thread pools
    server_stop_pools(status);
    
    //Close connections
    server_connection *elem, *tmp;
    LL_FOREACH_SAFE(status->clients, elem, tmp) {
        skt_close(elem->skt);
        LL_DELETE(status->clients, elem);
        server_connection_delete(elem);
    }
    
    //Close server socket
    close(status->epollfd);
    server_socket_release(status->sfd);
    
    //Free mime data
    mime_destroy();
    
    //Stop loggers
    log_register_clear();
    
    //Delete config
    config_server_delete(status->config);
    
    server_status_delete(status);
}