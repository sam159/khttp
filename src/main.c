/* 
 * File:   main.c
 * Author: sam
 *
 * Created on 16 July 2014, 20:05
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

#include "ut/utlist.h"
#include "main.h"
#include "socket.h"

int serverfd = 0;

/*
 * 
 */
int main(int argc, char** argv) {
    
    skt_elem *connections = NULL;
    
    serverfd = svr_create();
    svr_listen(serverfd, 1234);
    
    while(1) {
        //Accept new connections
        while(svr_canaccept(serverfd)) {
            skt_info *info = svr_accept(serverfd);
            if (info != NULL) {
                skt_elem* newconn = calloc(1, sizeof(skt_elem));
                newconn->info = info;
                LL_APPEND(connections, newconn);
            }
        }
        
        skt_elem *elem, *tmp;
        //Read from connections
        LL_FOREACH(connections, elem) {
            if (skt_canread(elem->info)) {
                skt_read(elem->info);
            }
        }
        
        //Process sockets
        LL_FOREACH(connections, elem) {
            if (utstring_len(elem->info->read) > 0) {
                utstring_printf(elem->info->write, "->");
                utstring_concat(elem->info->write, elem->info->read);
                utstring_clear(elem->info->read);
            }
        }
        
        //Write to connections
        LL_FOREACH(connections, elem) {
            if (utstring_len(elem->info->write) > 0) {
                skt_write(elem->info);
            } 
        }
        
        time_t current = time(NULL);
        time_t timeout = 5;
        time_t maxlife = 500;
        //Close where needed
        LL_FOREACH(connections, elem) {
            if (current - elem->info->last_act > timeout) {
                info("[#%lu %s] Timeout", elem->info->id, skt_addr(elem->info));
                elem->info->close = true;
            }
            if (current - elem->info->time_opened> maxlife) {
                info("[#%lu %s] Reached max life", elem->info->id, skt_addr(elem->info));
                elem->info->close = true;
            }
            if (elem->info->close == true) {
                skt_close(elem->info);
            }
        }
        //Delete closed connections
        LL_FOREACH_SAFE(connections, elem, tmp) {
            if (elem->info->closed) {
                LL_DELETE(connections, elem);
                skt_delete(elem->info);
                free(elem);
            }
        }
    }
    
    svr_release(serverfd);
    serverfd = 0;
    
    return (EXIT_SUCCESS);
}

void fatal(char* msg) {
    fprintf(stderr, "\n");
    perror(msg);
    exit(EXIT_FAILURE);
}
void warning(char* msg, bool showPError) {
    
    char warning[512];
    bzero(&warning, sizeof warning);
    snprintf(warning, 511, "Warning: %s", msg);
    
    if (showPError == true) {
        perror(warning);
    } else {
        fprintf(stderr, "%s\n", warning);
    }
}
void info(char* msg, ...) {
    va_list va;
    va_start(va, msg);
    vfprintf(stdout, msg, va);
    fputc('\n', stdout);
    va_end(va);
}