/* 
 * File:   server-loop.h
 * Author: sam
 *
 * Created on 18 August 2014, 17:16
 */

#ifndef SERVER_LOOP_H
#define	SERVER_LOOP_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include "server-state.h"
    
#define EP_MAXEVENTS 128
#define EP_WAIT_TIME 2000
    
#define EP_CONN(event) (server_connection*)event->data.ptr
#define EP_EVENT_IS(event, type) ((event->events & type) == type)
#define EP_EVENT_ISNOT(event, type) (!EP_EVENT_IS(event, type))

    void server_loop(server_state *state);
    void* server_loop_read(void* arg);
    void* server_loop_write(void* arg);
    void* server_loop_worker(void* arg);


#ifdef	__cplusplus
}
#endif

#endif	/* SERVER_LOOP_H */

