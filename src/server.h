/* 
 * File:   server.h
 * Author: sam
 *
 * Created on 17 August 2014, 22:01
 */

#ifndef SERVER_H
#define	SERVER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define DEFAULT_CONFIG_FILE "khttpd.ini"

#include "server-state.h"
    
    void server_start(server_status *status, const char* config_file);
    void server_teardown(server_status *status);

#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_H */

