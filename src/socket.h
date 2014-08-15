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
    
    typedef struct skt_info {
        u_int64_t id;
        int fd;
        struct sockaddr_in* clientaddr;
        time_t time_opened;
        bool error;
    } skt_info;
    
    u_int64_t skt_nextid();
    skt_info* skt_new(int fd);
    void skt_delete(skt_info *skt);
    
    bool skt_canread(skt_info *skt);
    size_t skt_read(skt_info *skt, char* buffer, size_t bufferlen);
    size_t skt_write(skt_info* skt, char* data, size_t len);
    int skt_write_data_buffer(skt_info *skt, data_buffer_list *list);
    void skt_close(skt_info *skt);
    const char* skt_clientaddr(skt_info *skt);

#ifdef	__cplusplus
}
#endif

#endif	/* SOCKET_H */

