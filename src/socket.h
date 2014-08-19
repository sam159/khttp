/* 
 * File:   socket.h
 * Author: sam
 *
 * Created on 16 July 2014, 20:19
 */

#ifndef SOCKET_H
#define	SOCKET_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
    
#include "data-buffer.h"
    
    typedef struct socket_info {
        u_int64_t id;
        int fd;
        bool closed;
        struct sockaddr_in* clientaddr;
        time_t time_opened;
        bool error;
    } socket_info;
    
    u_int64_t skt_nextid();
    socket_info* skt_new(int fd);
    void skt_delete(socket_info *skt);
    
    bool skt_canread(socket_info *skt);
    size_t skt_read(socket_info *skt, char* buffer, size_t bufferlen);
    size_t skt_write(socket_info* skt, char* data, size_t len);
    int skt_write_data_buffer(socket_info *skt, data_buffer_list *list);
    void skt_close(socket_info *skt);
    char* skt_clientaddr(socket_info *skt, char* address, size_t address_len);

#ifdef	__cplusplus
}
#endif

#endif	/* SOCKET_H */

