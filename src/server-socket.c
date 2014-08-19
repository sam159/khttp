#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "socket.h"
#include "server-socket.h"

int server_socket_create() {
    int fd = 0;
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        fatal("could not create socket");
    }
    return fd;
}
void server_socket_listen(int fd, uint16_t port) {
    assert(fd>=0);
    assert(port>0);
    
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr*)&server_address, sizeof server_address) < 0) {
        close(fd);
        fatal("Failed to bind to socket");
    }
    if (listen(fd, SOMAXCONN) < 0) {
        close(fd);
        fatal("Could not set socket to listen mode");
    }
    info("Listening on port %u", port);
}
void server_socket_listen_epoll(int fd, uint16_t port, int *out_epfd) {
    assert(out_epfd != NULL);
    server_socket_listen(fd, port);
    
    int epfd = 0;
    
    //Open epoll socket
    epfd = epoll_create1(0);
    if (epfd < 0) {
        fatal("Failed to create epollfd");
    }
    
    //Register socket with epoll
    struct epoll_event svr_event = {0};
    svr_event.data.fd = fd;
    svr_event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &svr_event) < 0) {
        fatal("Could not register server socket with epoll");
    }
    *out_epfd = epfd;
}
void server_socket_release(int fd) {
    assert(fd>=0);
    if (close(fd) < 0) {
        warning(true, "could not close socket");
    }
}
socket_info* server_socket_accept(int fd, int flags) {
    assert(fd>=0);
    struct sockaddr_in* clientaddr = calloc(1, sizeof(struct sockaddr_in));
    
    int clientfd=0;
    socklen_t clientaddr_len = (socklen_t)sizeof(struct sockaddr_in);
    clientfd = accept4(fd, (struct sockaddr*)clientaddr, &clientaddr_len, flags);
    if (clientfd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return NULL;
        }
        warning(true, "error accepting connection");
        return NULL;
    }
    
    socket_info* skt = skt_new(clientfd);
    skt->clientaddr = clientaddr;
    skt->fd = clientfd;
    
    char * address = calloc(INET_ADDRSTRLEN, sizeof(char));
    skt_clientaddr(skt, address, INET_ADDRSTRLEN);
    info("[#%lu %s] New Connection", skt->id, address);
    free(address);
    
    return skt;
}