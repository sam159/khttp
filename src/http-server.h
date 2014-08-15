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

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <dirent.h>

#include "http.h"
    
    typedef struct server_file_result {
        bool error;
        uint16_t error_code;
        char* error_text;
        bool dir;
        DIR *dirent;
        char* path;
    } server_file_result;
    
    http_response* server_process_request(config_server* config, http_request *request);
    server_file_result* server_determine_file(config_host* hconfig, const char* requested_path);
    char* server_find_directory_index(config_host *hconfig, char* path);
    FILE * server_generate_directory_index(config_host *hconfig, const char* dirpath);
    char* server_get_filesize(const char* filename);
    
    server_file_result* server_file_result_new();
    void server_file_result_seterror(server_file_result *result, uint16_t code, const char* message);
    void server_file_result_delete(server_file_result *result);
    
#ifdef	__cplusplus
}
#endif

#endif	/* HTTP_SERVER_H */

