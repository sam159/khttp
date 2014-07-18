#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <poll.h>
#include <arpa/inet.h>
#include "socket.h"
#include "ut/utstring.h"
#include "main.h"

u_int64_t skt_nextid() {
    static u_int64_t id = 0;
    return id++;
}
skt_info* skt_new(int fd) {
    skt_info* skt = calloc(1, sizeof(skt_info));
    skt->id = skt_nextid();
    skt->fd = fd;
    skt->last_act = skt->time_opened = time(NULL);
    utstring_new(skt->read);
    utstring_new(skt->write);
    skt->close = false;
    skt->close_afterwrite = false;
    skt->closed = false;
    
    return skt;
}
void skt_delete(skt_info* skt) {
    utstring_free(skt->read);
    utstring_free(skt->write);
    free(skt->clientaddr);
    free(skt);
}

bool skt_canread(skt_info* skt) {
    int len = 0;
    if (ioctl(skt->fd, FIONREAD, &len) < 0) {
        warning("ioctl failed", true);
        return false;
    }
    return len > 0;
}
uint32_t skt_read(skt_info* skt) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    
    int result = read(skt->fd, &buffer,1023);
    if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        warning("read error", true);
        skt->close = true;
        return 0;
    }
    skt->last_act = time(NULL);
    utstring_printf(skt->read, "%s", buffer);
    return result; //Number of bytes read
}
uint32_t skt_write(skt_info* skt) {
    if (utstring_len(skt->write) == 0) {
        return 0;
    }
    
    int result = write(skt->fd, utstring_body(skt->write), utstring_len(skt->write));
    if (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        warning("write error", true);
        skt->close = true;
        return 0;
    }
    skt->last_act = time(NULL);
    
    if (result == utstring_len(skt->write)) {
        utstring_free(skt->write);
        utstring_new(skt->write);
        return 0;
    }
    //remove first x chars
    char* newstr = calloc(utstring_len(skt->write) - result + 1, sizeof(char));
    
    char* writeBody = utstring_body(skt->write);
    strcpy(newstr, writeBody + (sizeof(char) * result));
    
    utstring_free(skt->write);
    utstring_new(skt->write);
    utstring_printf(skt->write, "%s", newstr);
    free(newstr);
    return result; //bytes written
}
void skt_close(skt_info* skt) {
    if (skt->closed == true) {
        return;
    }
    info("[#%lu %s] Closed", skt->id, skt_clientaddr(skt));
    if (close(skt->fd) < 0) {
        warning("error closing socket", true);
    }
    skt->closed = true;
}
char* skt_clientaddr(skt_info *skt) {
    char* address = inet_ntoa(skt->clientaddr->sin_addr);
    return address;
}

int svr_create() {
    int fd = 0;
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        fatal("could not create socket");
    }
    return fd;
}
void svr_listen(int fd, uint16_t port) {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr*)&server_address, sizeof server_address) < 0) {
        warning("failed to bind", true);
        close(fd);
        exit(EXIT_FAILURE);
    }
    if (listen(fd, 16) < 0) {
        warning("failed to listen", true);
        close(fd);
        exit(EXIT_FAILURE);
    }
    info("Listening on port %u", port);
}
void svr_release(int fd) {
    if (close(fd) < 0) {
        warning("could not close socket", true);
    }
}
bool svr_canaccept(int fd) {
    struct pollfd* pfd = calloc(1, sizeof(struct pollfd));
    
    pfd[0].fd = fd;
    pfd[0].events = POLLIN;
    
    if (poll(pfd, 1, 50/*ms*/) < 0) {
        warning("poll failed", true);
        return false;
    }
    if ((pfd[0].revents & POLLIN) == POLLIN) {
        return true;
    }
    return false;
}
skt_info* svr_accept(int fd) {
    struct sockaddr_in* clientaddr = calloc(1, sizeof(struct sockaddr_in));
    
    int clientfd=0;
    socklen_t clientaddr_len = (socklen_t)sizeof(struct sockaddr_in);
    clientfd = accept(fd, (struct sockaddr*)clientaddr, &clientaddr_len);
    if (clientfd < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        warning("error accepting connection", true);
        return NULL;
    }
    
    skt_info* skt = skt_new(clientfd);
    skt->clientaddr = clientaddr;
    skt->fd = clientfd;
    
    info("[#%lu %s] New Connection", skt->id, skt_clientaddr(skt));
    
    return skt;
    
}