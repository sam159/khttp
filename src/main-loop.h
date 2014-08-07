/* 
 * File:   main-loop.h
 * Author: sam
 *
 * Created on 07 August 2014, 18:44
 */

#ifndef MAIN_LOOP_H
#define	MAIN_LOOP_H

#include "config.h"
#include "thread-pool.h"

#ifdef	__cplusplus
extern "C" {
#endif
    
    typedef struct hmain_status {
        config_server *config;
        int sfd;
        int epollfd;
        struct {
            thread_pool *read, *write, *workers, *disk_read;
        } pool;
    } hmain_status;
    
    void hmain_setup(hmain_status **statusptr);
    void hmain_teardown(hmain_status *status);
    void hmain_loop(hmain_status *status);

    void* thloop_read(void * arg);
    void* thloop_write(void * arg);
    void* thloop_disk_read(void * arg);
    void* thloop_worker(void * arg);

#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_LOOP_H */

