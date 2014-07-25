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

#include <stdbool.h>
#include "http_parser.h"
#include "socket.h"
#include "http/http.h"
    
    typedef struct file_map {
        char* map;
        size_t size;
    } file_map;
    
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
    void skt_elem_delete(skt_elem* elem);

    int main(int argc, char** argv);

    void fatal(char* msg);
    void warning(char* msg, bool showPError);
    void info(char* msg, ...);

    char** str_splitlines(char *str, size_t *line_count);
    char* str_replace(char *str, const char *search, const char *replacement);

    file_map* file_map_new(const char* filename);
    void  file_map_delete(file_map* map);


#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

