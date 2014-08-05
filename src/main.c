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
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>
#include <signal.h>
#include <bits/stdio2.h>

#include "http_parser.h"

#include "ut/utlist.h"
#include "ut/utarray.h"
#include "main.h"
#include "socket.h"
#include "http.h"
#include "http-reader.h"
#include "config.h"
#include "http-server.h"
#include "mime.h"
#include "queue.h"
#include "thread-pool.h"

int serverfd = 0;
volatile static bool stop = false;

static void signal_int(int signum) {
    fprintf(stderr, "Terminating...\n");
    stop = true;
}

int main(int argc, char** argv) {
    
    mime_init(NULL);
    config_server *config = config_server_new();
    if (config_read_ini("khttpd.ini", config) < 0) {
        fatal("Could not read config");
    }
    
    signal(SIGINT, signal_int);
    
    skt_elem *connections = NULL;
    
    serverfd = svr_create();
    svr_listen(serverfd, config->listen_port);
    
    while(1) {
        uint32_t connections_open;
        skt_elem *elem, *tmp;
        
        //Accept new connections
        LL_COUNT(connections, elem, connections_open);
        while(connections_open < 100 && svr_canaccept(serverfd)) {
            skt_info *info = svr_accept(serverfd);
            if (info != NULL) {
                skt_elem *elem = skt_elem_new(info);
                LL_APPEND(connections, elem);
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
                //Parse the incoming data
                int parsedcount = http_parser_execute(
                        elem->parser, 
                        parser_get_settings(elem), 
                        utstring_body(elem->info->read), 
                        utstring_len(elem->info->read));
                //Check that all data was read
                if (parsedcount != utstring_len(elem->info->read)) {
                    //emit warning
                    char warningmsg[2048] = {0};
                    snprintf(warningmsg, 2048, 
                            "error parsing request (%s: %s). closing connection", 
                            http_errno_name(elem->parser->http_errno),
                            http_errno_description(elem->parser->http_errno));
                    warning(false, warningmsg);
                    //send 400 back and close connection
                    http_response *resp400 = http_response_create_builtin(400, "Request was invalid or could not be read");
                    http_header_list_add(resp400->headers, http_header_new(HEADER_CONNECTION, "close"), false);
                    skt_elem_write_response(elem, resp400, false);
                    http_response_delete(resp400);
                    skt_elem_reset(elem);
                }
                //Clear read data now that we have processed it
                utstring_clear(elem->info->read);
                //Process request if received
                if (elem->request_complete == true) {
                    http_response *response = server_process_request(config, elem->current_request);
                    if (response == NULL) {
                        response = http_response_create_builtin(500, "Request could not be processed");
                        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
                    }
                    skt_elem_write_response(elem, response, true);
                    
                    skt_elem_reset(elem);
                }
            }
        }
        
        //Write to connections
        LL_FOREACH(connections, elem) {
            if (utstring_len(elem->info->write) > 0 && elem->info->close == false) {
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
            if (current - elem->info->time_opened > maxlife) {
                info("[#%lu %s] Reached max life", elem->info->id, skt_clientaddr(elem->info));
                elem->info->close = true;
            }
            if (elem->info->close_afterwrite && utstring_len(elem->info->write) == 0) {
                elem->info->close = true;
            }
            if (elem->info->close == true || stop == true) {
                skt_close(elem->info);
            }
        }
        //Delete closed connections
        LL_FOREACH_SAFE(connections, elem, tmp) {
            if (elem->info->closed) {
                LL_DELETE(connections, elem);
                skt_elem_delete(elem);
            }
        }
        if (stop == true) {
            break;
        }
    }
    
    mime_destroy();
    config_server_delete(config);
    svr_release(serverfd);
    serverfd = 0;
    
    return (EXIT_SUCCESS);
}

skt_elem* skt_elem_new(skt_info *info) {
    skt_elem* elem = calloc(1, sizeof(skt_elem));
    elem->info = info;
    elem->parser = calloc(1, sizeof(http_parser));
    http_parser_init(elem->parser, HTTP_REQUEST);
    elem->parser->data = (void*)elem;
    elem->parser_header_state = HSTATE_NONE;
    elem->request_complete = false;
    return elem;
}
void skt_elem_reset(skt_elem *elem) {
    if (elem->current_request != NULL) {
        http_request_delete(elem->current_request);
        elem->current_request = NULL;
    }
    if (elem->parser_current_header != NULL) {
        http_header_delete(elem->parser_current_header);
    }
    elem->parser_current_header = NULL;
    elem->parser_header_state = HSTATE_NONE;
    elem->request_complete = false;
}
void skt_elem_write_response(skt_elem *elem, http_response *response, bool dispose) {
    http_header* connection_header = http_header_list_get(response->headers, HEADER_CONNECTION);
    if (connection_header != NULL && strcasecmp(connection_header->content, "close") == 0) {
        elem->info->close_afterwrite = true;
    }
    if (connection_header == NULL) {
        if (response->resp->version == HTTP11) {
            http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "Keep-Alive"), true);
        } else if (response->resp->version == HTTP10) {
            elem->info->close_afterwrite = true;
        }
    }
    char *response_str = http_response_write(response);
    utstring_printf(elem->info->write, "%s", response_str);
    free(response_str);
    if (dispose == true) {
        http_response_delete(response);
    }
}
void skt_elem_delete(skt_elem* elem) {
    if (elem->info!=NULL) skt_delete(elem->info);
    if (elem->current_request!=NULL) http_request_delete(elem->current_request);
    if (elem->parser!= NULL) {
        elem->parser->data = NULL;
        free(elem->parser);
    }
    
    free(elem);
}
