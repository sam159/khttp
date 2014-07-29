#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <magic.h>
#include <unistd.h>
#include <sys/stat.h>

#include "http_parser.h"
#include "http.h"
#include "main.h"
#include "config.h"
#include "http-server.h"
#include "mime.h"

http_response* server_process_request(config_server* config, http_request *request) {
    http_response* response = NULL;
    //Determin host
    char* hostname=NULL;
    if (request->req->version == HTTP11) {
        http_header *hostheader = http_header_list_get(request->headers, HEADER_HOST);
        if (hostheader != NULL) {
            hostname = strdup(hostheader->content);
        } else {
            response = http_response_create_builtin(400, "Host header required");
            http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
            return response;
        }
    }
    //hostname may be null, this indicates that the default host should be used
    config_host* host_config = config_server_gethost(config, hostname);
    if (hostname != NULL) {
        free(hostname);
        hostname = NULL;
    }
    if (host_config == NULL) {
        //host not found and default not found
        response = http_response_create_builtin(500, "Server configuration error (host/default not found)");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    
    //Validate request method
    http_request_method acceptable_methods[] = {METHOD_GET, METHOD_HEAD, METHOD_POST};
    bool valid_method = false;
    for(int i=0; i<sizeof(acceptable_methods)/sizeof(http_request_method); i++) {
        if (request->req->method == acceptable_methods[i]) {
            valid_method = true;
            break;
        }
    }
    if (valid_method == false) {
        response = http_response_create_builtin(411, "Method is not valid");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    
    //Parse the request uri
    struct http_parser_url *url = calloc(1, sizeof(struct http_parser_url));
    if (http_parser_parse_url(request->req->uri, strlen(request->req->uri), false, url) < 0) {
        response = http_response_create_builtin(400, "Invalid request");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    
    //Get the uri/path
    char* uri = NULL;
    if ((url->field_set & (1 << UF_PATH)) != 0) {
        uri = calloc(url->field_data[UF_PATH].len+1, sizeof(char));
        strncpy(uri, request->req->uri+url->field_data[UF_PATH].off, url->field_data[UF_PATH].len);
    } else {
        //Not found (for some reason)
        response = http_response_create_builtin(400, "URI is required or was invalid");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    
    //Build actual path to file requested
    char* filepath_requested = NULL;
    size_t totallen = strlen(host_config->serve_dir) + strlen(uri) + 2;
    filepath_requested = calloc(totallen, sizeof(char));
    strcat(filepath_requested, host_config->serve_dir);
    strcat(filepath_requested, "/");
    strcat(filepath_requested, uri);
    
    //Check that the file exists
    char* filepath_actual = realpath(filepath_requested, NULL);
    if (filepath_actual == NULL) {
        warning("realpath: not found/error", true);
        response = http_response_create_builtin(404, "File not found");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    //Check that the file is within the server directory of the host
    if (strncmp(host_config->serve_dir, filepath_actual, strlen(host_config->serve_dir)) != 0) {
        //file is outside the server directory :S
        response = http_response_create_builtin(400, "URI is required or was invalid");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    
    struct stat *pathstat = calloc(1, sizeof(struct stat));
    if (stat(filepath_actual, pathstat) < 0) {
        warning("stat failed", true);
        response = http_response_create_builtin(400, "URI is required or was invalid");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    //Is is a directory?
    if (S_ISDIR(pathstat->st_mode) != 0) {
        response = http_response_create_builtin(200, "Directory listing not implemented");
        return response;
    }
    
    //Open file
    FILE *file = fopen(filepath_actual, "r");
    if (file == NULL) {
        warning("failed to open file for reading", true);
        response = http_response_create_builtin(404, "File not found");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        return response;
    }
    
    //File is ok and can be served to the client
    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);
    
    //Read file into response
    //TODO: send file directly from the write loop
    char* buffer = calloc(filesize, sizeof(char));
    if (fread(buffer, sizeof(char), filesize, file) != filesize) {
        warning("failed to read file into memory", true);
        response = http_response_create_builtin(500, "Could not read file");
    } else {
        response = http_response_new(http_response_line_new(200));
        response->resp->version = request->req->version;
        http_header_list_add(response->headers, http_header_new(HEADER_CONTENT_TYPE, mime_get_type(filepath_actual, DEFAULT_CONTENT_TYPE)), false);
        http_response_append_body(response, buffer);
    }
    fclose(file);
    free(buffer);
    free(filepath_requested);
    free(filepath_actual);
    
    //Check to see if client requested the connection be closed
    http_header* request_connection = http_header_list_get(request->headers, HEADER_CONNECTION);
    if (request_connection != NULL && strcasecmp(request_connection->content, "close") == 0) {
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
    }
    
    return response;
}