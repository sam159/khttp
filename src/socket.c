#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>

#include "ut/utstring.h"
#include "ut/utlist.h"

#include "util.h"
#include "data-buffer.h"
#include "socket.h"

u_int64_t skt_nextid() {
    static u_int64_t id = 1;
    return __atomic_fetch_add(&id, 1, __ATOMIC_SEQ_CST);
}
socket_info* skt_new(int fd) {
    assert(fd>0);
    socket_info* skt = calloc(1, sizeof(socket_info));
    skt->id = skt_nextid();
    skt->fd = fd;
    skt->closed = false;
    skt->time_opened = time(NULL);
    skt->error = false;
    skt->clientaddr = NULL;
    return skt;
}

void skt_delete(socket_info* skt) {
    assert(skt != NULL);
    free(skt->clientaddr);
    free(skt);
}

bool skt_canread(socket_info* skt) {
    assert(skt != NULL);
    int len = 0;
    if (ioctl(skt->fd, FIONREAD, &len) < 0) {
        warning(true, "ioctl failed");
        return false;
    }
    return len > 0;
}
size_t skt_read(socket_info* skt, char* buffer, size_t bufferlen) {
    assert(skt != NULL);
    int result = read(skt->fd, buffer, bufferlen);
    if (result < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            warning(true, "read error");
            skt->error = true;
        }
        return 0;
    }
    return result; //Number of bytes read
}
size_t skt_write(socket_info* skt, char* data, size_t len) {
    assert(skt != NULL);
    assert(data != NULL);
    
    int result = write(skt->fd, data, len);
    if (result < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            warning(true, "write error");
            skt->error = true;
        }
        return 0;
    }
    return result; //bytes written
}
int skt_data_buffer(socket_info *skt, data_buffer_list *list) {
    assert(skt != NULL);
    assert(list != NULL);
    BUFFER_LIST_RD_LOCK(list);
    
    do {
        data_buffer *elem = list->first;
        size_t written = skt_write(skt, elem->buffer + elem->rOffset, elem->wOffset - elem->rOffset);
        if (written == 0) {
            break;
        }
        elem->rOffset += written;
        if (elem->rOffset == elem->wOffset) {
            BUFFER_LIST_WR_LOCK(list);
            LL_DELETE(list->first, elem);
            BUFFER_LIST_WR_UNLOCK(list);
            data_buffer_free(elem);
        }
    } while(list->first != NULL);
    
    int result;
    if (skt->error == true) {
        result = -1;
    }
    if (list->first == NULL) {
        result = 0;
    } else {
        result = 1;
    }
    
    BUFFER_LIST_RD_UNLOCK(list);
    
    return result;
}
void skt_close(socket_info* skt) {
    assert(skt != NULL);
    if (close(skt->fd) < 0) {
        warning(true, "error closing socket");
    }
    skt->closed = true;
}
char* skt_clientaddr(socket_info *skt, char* address, size_t address_len) {
    assert(skt != NULL);
    assert(address != NULL);
    assert(address_len >= INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &skt->clientaddr->sin_addr, address, address_len);
    if (address == NULL) {
        warning(true, "error fetching client address");
        free(address);
    }
    return address;
}