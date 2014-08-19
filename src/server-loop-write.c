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

void* server_loop_write(void* arg) {
    thread *th = (thread*)arg;
    
    while(th->stop == false) {
        queue_item *item = queue_fetchone(th->pool->queue, true);
        if (item == NULL) {
            continue;
        }
        server_connection *conn = (server_connection*)item->data;
        CONN_LOCK(conn);
        
        size_t count = 0;
        while(conn->pending_writes->first != NULL) {
            BUFFER_LIST_RD_LOCK(conn->pending_writes);
            data_buffer *next = conn->pending_writes->first;
            
            count = write(conn->skt->fd, next->buffer+next->rOffset, next->wOffset - next->rOffset);
            if (count < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    item->blocked = true;
                    break;
                } else {
                    if (errno != EBADF) {
                        char address[INET_ADDRSTRLEN];
                        skt_clientaddr(conn->skt, address, INET_ADDRSTRLEN);
                        warning(true, "[#%lu %s] write error", conn->id, address);
                    }
                    conn->skt->error = true;
                    break;
                }
            }
            next->rOffset += count;
            
            if (next->rOffset >= next->wOffset) {
                LL_DELETE(conn->pending_writes->first, next);
                data_buffer_free(next);
            }
            
            BUFFER_LIST_RD_UNLOCK(conn->pending_writes);
        }
        
        CONN_UNLOCK(conn);
        queue_return_item(th->pool->queue, item, item->blocked == false);
    }
}