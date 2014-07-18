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
#include "ut/utarray.h"
#include "main.h"
#include "socket.h"
#include "http/http.h"
#include "http/request.h"

int serverfd = 0;
char* teststr = "GET /testing/123 HTTP/1.1\r\n";

/*
 * 
 */
int main(int argc, char** argv) {
    /*char *test = calloc(strlen(teststr)+1, sizeof(char));
    strcpy(test, teststr);
    
    http_request *req = http_request_new();
    parse_request(req, test);
    
    return 0;*/
    skt_elem *connections = NULL;
    
    serverfd = svr_create();
    svr_listen(serverfd, 1234);
    
    while(1) {
        uint32_t counter;
        skt_elem *elem, *tmp;
        
        //Accept new connections
        LL_COUNT(connections, elem, counter);
        while(counter < 100 && svr_canaccept(serverfd)) {
            skt_info *info = svr_accept(serverfd);
            if (info != NULL) {
                skt_elem* newconn = calloc(1, sizeof(skt_elem));
                newconn->info = info;
                newconn->current_request = http_request_new();
                LL_APPEND(connections, newconn);
            }
        }
        
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
        time_t timeout = 30;
        time_t maxlife = 500;
        //Close where needed
        LL_FOREACH(connections, elem) {
            if (current - elem->info->last_act > timeout) {
                info("[#%lu %s] Timeout", elem->info->id, skt_clientaddr(elem->info));
                elem->info->close = true;
            }
            if (current - elem->info->time_opened> maxlife) {
                info("[#%lu %s] Reached max life", elem->info->id, skt_clientaddr(elem->info));
                elem->info->close = true;
            }
            if (elem->info->close_afterwrite && utstring_len(elem->info->write) == 0) {
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
                if (elem->current_request != NULL) {
                    http_request_delete(elem->current_request);
                }
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

char** str_splitlines(char *str, size_t *line_count) {
    char **result;
    *line_count = 0;
    char *tmp = str;
    
    while(*tmp) {
        if (*tmp == '\n') {
            (*line_count)++;
        }
        tmp++;
    }
    if (*line_count == 0) {
        result = calloc(1, sizeof(char*));
        result[0] = calloc(strlen(str), sizeof(char));
        strcpy(result[0], str);
        return result;
    }
    result = calloc(*line_count, sizeof(char*));
    if (result == NULL) {
        fatal("calloc failed");
    }
    
    size_t i=0, linelen = 0;
    char *line = strtok(str, "\n");
    while(line) {
        linelen = strlen(line);
        result[i] = calloc(linelen+1, sizeof(char));
        if (result[i] == NULL) {
            fatal("calloc failed");
        }
        strcpy(result[i], line);
        if (result[i][linelen-1] == '\r') {
            result[i][linelen-1] = '\0';
            result[i] = realloc(result[i], linelen);
        }
        line = strtok(NULL, "\n");
        i++;
    }
    
    return result;
}