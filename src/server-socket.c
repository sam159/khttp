#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

#include "socket.h"
#include "server-socket.h"
#include "util.h"

int server_socket_create() {
    int fd = 0;
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        fatal("could not create socket");
    }
    return fd;
}
void server_socket_listen(int fd, uint16_t port) {
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
void server_socket_release(int fd) {
    if (close(fd) < 0) {
        warning(true, "could not close socket");
    }
}
bool server_socket_canaccept(int fd) {
    struct pollfd* pfd = calloc(1, sizeof(struct pollfd));
    
    pfd[0].fd = fd;
    pfd[0].events = POLLIN;
    
    if (poll(pfd, 1, 50/*ms*/) < 0) {
        warning(true, "poll failed");
        free(pfd);
        return false;
    }
    if ((pfd[0].revents & POLLIN) == POLLIN) {
        free(pfd);
        return true;
    }
    free(pfd);
    return false;
}
socket_info* server_socket_accept(int fd, int flags) {
    struct sockaddr_in* clientaddr = calloc(1, sizeof(struct sockaddr_in));
    
    int clientfd=0;
    socklen_t clientaddr_len = (socklen_t)sizeof(struct sockaddr_in);
    clientfd = accept4(fd, (struct sockaddr*)clientaddr, &clientaddr_len, flags);
    if (clientfd < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        warning(true, "error accepting connection");
        return NULL;
    }
    
    socket_info* skt = skt_new(clientfd);
    skt->clientaddr = clientaddr;
    skt->fd = clientfd;
    
    info("[#%lu %s] New Connection", skt->id, skt_clientaddr(skt));
    
    return skt;
    
}