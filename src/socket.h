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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include "http/http.h"
#include "ut/utstring.h"
    
    typedef struct skt_info skt_info;
    
    struct skt_info {
        u_int64_t id;
        int fd;
        time_t time_opened;
        time_t last_act;
        UT_string *read;
        UT_string *write;
        bool close;
        bool close_afterwrite;
        bool closed;
        struct sockaddr_in* clientaddr;
    };
    
    typedef struct skt_elem {
        skt_info* info;
        http_request *current_request;
        struct skt_elem *next;
    } skt_elem;
    
    u_int64_t skt_nextid();
    skt_info* skt_new(int fd);
    void skt_delete(skt_info *skt);
    
    bool skt_canread(skt_info *skt);
    uint32_t skt_read(skt_info *skt);
    uint32_t skt_write(skt_info *skt);
    void skt_close(skt_info *skt);
    char* skt_clientaddr(skt_info *skt);
    
    int svr_create();
    void svr_listen(int fd, uint16_t port);
    void svr_release(int fd);
    bool svr_canaccept(int fd);
    skt_info* svr_accept(int fd);
    


#ifdef	__cplusplus
}
#endif

#endif	/* SOCKET_H */

