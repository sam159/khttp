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
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <http_parser.h>

#include "ut/utlist.h"
#include "ut/utarray.h"
#include "main.h"
#include "socket.h"
#include "http/http.h"
#include "http/parse.h"

int serverfd = 0;

int main(int argc, char** argv) {
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
                LL_APPEND(connections, skt_elem_new(info));
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
                int parsedcount = http_parser_execute(
                        elem->parser, 
                        parser_get_settings(elem), 
                        utstring_body(elem->info->read), 
                        utstring_len(elem->info->read));
                if (parsedcount != utstring_len(elem->info->read)) {
                    warning("error parsing request. closing connection", false);
                    elem->info->close = true;
                }
                utstring_clear(elem->info->read);
                if (elem->request_complete == true) {
                    http_response* resp = http_response_create_builtin(200, elem->current_request->req->uri);
                    utstring_printf(elem->info->write, "%s", http_response_write(resp));
                    http_response_delete(resp);
                    elem->request_complete = false;
                    http_request_delete(elem->current_request);
                    elem->current_request = NULL;
                    elem->info->close_afterwrite = true;
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
            if (elem->info->close == true) {
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
    }
    
    svr_release(serverfd);
    serverfd = 0;
    
    return (EXIT_SUCCESS);
}

skt_elem* skt_elem_new(skt_info *info) {
    skt_elem* elem = calloc(1, sizeof(skt_elem));
    elem->info = info;
    elem->parser = calloc(1, sizeof(http_parser));
    http_parser_init(elem->parser, HTTP_REQUEST);
    elem->parser_header_state = HSTATE_NONE;
    elem->request_complete = false;
    return elem;
}
void skt_elem_delete(skt_elem* elem) {
    if (elem->info!=NULL) skt_delete(elem->info);
    if (elem->current_request!=NULL) http_request_delete(elem->current_request);
    free(elem);
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

file_map* map_file(const char* filename) {
    
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        warning("Failed to open file for memory mapping", true);
        return NULL;
    }
    size_t size = lseek(fd, 0L, SEEK_END);
    void* map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        warning("Failed to mmap file", true);
        close(fd);
        return NULL;
    }
    close(fd);
    
    file_map* filemap = calloc(1, sizeof(file_map));
    filemap->map = (char*)map;
    filemap->size = size;
    return filemap;
}
void free_mapped_file(file_map* file) {
    if (munmap((void*)file->map, file->size) < 0) {
        warning("failed to unmap file", true);
    }
    free(file);
}

char* str_replace(char *haystack, const char *search, const char *replacement) {
    
    size_t haystacklen = strlen(haystack);
    size_t searchlen = strlen(search);
    size_t replacementlen = strlen(replacement);
    
    char* result = haystack;
    
    if (searchlen > haystacklen || searchlen == 0) {
        return result;
    }
    if (strstr(replacement, search) != NULL) {
        warning("str_replace: replacement should not contain the search criteria", false);
    }
    int count = 0;
    while(count++ < 1000) {
        char* pos = strstr(result, search);
        if (pos == NULL) {
            break;
        }
        uint32_t start = (pos - result) / sizeof(char);
        uint32_t end = start + searchlen;
        
        size_t resultlen = strlen(result);
        size_t newlen = resultlen + replacementlen - searchlen;
        
        char* newstr = calloc(newlen+1, sizeof(char));
        strncpy(newstr, result, start);
        strcat(newstr, replacement);
        strcat(newstr, pos+(searchlen*sizeof(char)));
        
        free(result);
        result = newstr;
    }
    return result;
}