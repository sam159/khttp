/* 
 * File:   server-socket.h
 * Author: sam
 *
 * Created on 15 August 2014, 13:22
 */

#ifndef SERVER_SOCKET_H
#define	SERVER_SOCKET_H

#include "socket.h"

#ifdef	__cplusplus
extern "C" {
#endif

    int server_socket_create();
    void server_socket_listen(int fd, uint16_t port);
    void server_socket_release(int fd);
    bool server_socket_canaccept(int fd);
    skt_info* server_socket_accept(int fd, int flags);


#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_SOCKET_H */

