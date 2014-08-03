/* 
 * File:   main.h
 * Author: sam
 *
 * Created on 16 July 2014, 20:18
 */

#ifndef MAIN_H
#define	MAIN_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define SERVER_NAME "KHTTP/0.1"

#include <stdbool.h>
#include "http_parser.h"
#include "socket.h"
#include "http.h"
#include "util.h"
    
    typedef enum skt_elem_hstate {HSTATE_NONE, HSTATE_VALUE, HSTATE_FIELD} skt_elem_hstate;
    typedef struct skt_elem {
        skt_info* info;
        http_request *current_request;
        bool request_complete;
        http_parser *parser;
        http_header *parser_current_header;
        skt_elem_hstate parser_header_state;
        struct skt_elem *next;
    } skt_elem;
    
    skt_elem* skt_elem_new(skt_info *info);
    void skt_elem_reset(skt_elem *elem);
    void skt_elem_write_response(skt_elem *skt, http_response *response, bool dispose);
    void skt_elem_delete(skt_elem* elem);

    int main(int argc, char** argv);

    


#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

