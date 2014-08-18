/* 
 * File:   server-socket.h
 * Author: sam
 *
 * Created on 15 August 2014, 13:22
 */

#ifndef SERVER_SOCKET_H
#define	SERVER_SOCKET_H


#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "socket.h"
    
    int server_socket_create();
    void server_socket_listen(int fd, uint16_t port);
    void server_socket_listen_epoll(int fd, uint16_t port, int *out_epfd);
    void server_socket_release(int fd);
    socket_info* server_socket_accept(int fd, int flags);


#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_SOCKET_H */

