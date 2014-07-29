/* 
 * File:   http-server.h
 * Author: sam
 *
 * Created on 26 July 2014, 15:32
 */

#ifndef HTTP_SERVER_H
#define	HTTP_SERVER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "http.h"
#include "main.h"
#include "config.h"
    
    http_response* server_process_request(config_server* config, http_request *request);
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* HTTP_SERVER_H */
