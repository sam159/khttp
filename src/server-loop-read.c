#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include "util.h"
#include "log.h"
#include "config.h"
#include "data-buffer.h"
#include "queue.h"

#include "server-connection.h"
#include "server-state.h"
#include "server-loop.h"

void* server_loop_read(void* arg) {
    thread *th = (thread*)arg;
    
    while(th->stop == false) {
        queue_item *item = queue_fetchone(th->pool->queue, true);
        if (item == NULL) {
            continue;
        }
        server_connection *conn = (server_connection*)item->data;
        CONN_LOCK(conn);
        ssize_t count=0, totalread=0;
        char readbuf[16*1024];
        while(true) {
            count = read(conn->skt->fd, readbuf, sizeof(readbuf)/sizeof(readbuf[0]));
            if (count < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    item->blocked = true;
                    break;
                } else {
                    if (errno != EBADF) {
                        char address[INET_ADDRSTRLEN];
                        skt_clientaddr(conn->skt, address, INET_ADDRSTRLEN);
                        warning(true, "[#%lu %s] read error", conn->id, address);
                    }
                    conn->skt->error = true;
                    break;
                }
            }
            if (count == 0) {
                break;
            }
            totalread += count;
            data_buffer_list_append(conn->pending_writes, readbuf, count);
        }
        if (totalread > 0) {
            queue_add(conn->server->pools[POOL_WRITE]->queue, queue_item_new2("WRITE", conn));
        }
        CONN_UNLOCK(conn);
        queue_return_item(th->pool->queue, item, item->blocked == false);
    }
}