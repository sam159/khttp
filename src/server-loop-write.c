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
        
        
        
        CONN_UNLOCK(conn);
        queue_return_item(th->pool->queue, item, item->blocked == false);
    }
}