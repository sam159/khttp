#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <magic.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

#include "ut/utstring.h"

#include "http_parser.h"
#include "http.h"
#include "config.h"
#include "http-server.h"
#include "mime.h"
#include "util.h"

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
        free(url);
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
        free(url);
        return response;
    }
    free(url);
    
    //Build actual path to file requested
    char* filepath = NULL;
    size_t totallen = strlen(host_config->serve_dir) + strlen(uri) + 1;
    filepath = calloc(totallen, sizeof(char));
    strcat(filepath, host_config->serve_dir);
    strcat(filepath, uri);
    free(uri);
    
    server_file_result *file_result = server_determine_file(host_config, filepath);
    free(filepath);
    filepath=NULL;
    
    if (file_result->error != false) {
        response = http_response_create_builtin(file_result->error_code, file_result->error_text);
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        server_file_result_delete(file_result);
        return response;
    }
    FILE *file = NULL;
    if (file_result->dir == true) {
        file = server_generate_directory_index(host_config, file_result->path);
        if (file == NULL) {
            response = http_response_create_builtin(500, "Failed to generate directory index");
            http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
            server_file_result_delete(file_result);
            return response;
        }
    } else {
        filepath = strdup(file_result->path);
    }
    server_file_result_delete(file_result);
    
    //Open file
    if (file == NULL) {
        file = fopen(filepath, "rb");
    }
    if (file == NULL) {
        warning(true, "failed to open file for reading");
        response = http_response_create_builtin(404, "File not found");
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
        free(filepath);
        return response;
    }
    
    //File is ok and can be served to the client
    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);
    
    //Read file into response
    //TODO: send file directly from the write loop
    char* buffer = calloc(filesize+1, sizeof(char));
    if (fread(buffer, sizeof(char), filesize, file) != filesize) {
        warning(true, "failed to read file into memory");
        response = http_response_create_builtin(500, "Could not read file");
    } else {
        response = http_response_new(http_response_line_new(200));
        response->resp->version = request->req->version;
        const char* mime_type = "text/html";
        if (filepath != NULL) {
            mime_type = mime_get_type(filepath, DEFAULT_CONTENT_TYPE);
        }
        http_header_list_add(response->headers, http_header_new(HEADER_CONTENT_TYPE, mime_type), false);
        http_response_append_body(response, buffer);
    }
    fclose(file);
    free(buffer);
    free(filepath);
    
    //Check to see if client requested the connection be closed
    http_header* request_connection = http_header_list_get(request->headers, HEADER_CONNECTION);
    if (request_connection != NULL && strcasecmp(request_connection->content, "close") == 0) {
        http_header_list_add(response->headers, http_header_new(HEADER_CONNECTION, "close"), false);
    }
    
    return response;
}

server_file_result* server_determine_file(config_host* hconfig, const char* requested_path) {
    server_file_result *result = server_file_result_new();
    //Check that the file exists
    char* filepath_actual = realpath(requested_path, NULL);
    if (filepath_actual == NULL) {
        server_file_result_seterror(result, 404, "File not found");
        return result;
    }
    //Check that the file is within the server directory of the host
    if (strncmp(hconfig->serve_dir, filepath_actual, strlen(hconfig->serve_dir)) != 0) {
        //file is outside the server directory :S
        server_file_result_seterror(result, 404, "File not found");
        free(filepath_actual);
        return result;
    }
    
    struct stat *pathstat = calloc(1, sizeof(struct stat));
    if (stat(filepath_actual, pathstat) < 0) {
        server_file_result_seterror(result, 404, "File not found");
        free(filepath_actual);
        free(pathstat);
        return result;
    }
    //If directory
    if (S_ISDIR(pathstat->st_mode) != 0) {
        char* dir_index = server_find_directory_index(hconfig, filepath_actual);
        if (hconfig->index_files_count > 0 && dir_index != NULL) {
            //Use the directory index
            result->path = dir_index;
        } else if (hconfig->dir_listings == true) {
            //Show directory listing
            result->dir = true;
            result->path = strdup(filepath_actual);
        } else {
            server_file_result_seterror(result, 403, "No index file was found and directory indexes are disabled");
        }
    } else if (S_ISREG(pathstat->st_mode) != 0) {
        result->path = strdup(filepath_actual);
    }
    free(pathstat);
    free(filepath_actual);
    return result;
}
char* server_find_directory_index(config_host *hconfig, char* path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return NULL;
    }
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        for(int i=0; i<hconfig->index_files_count; i++) {
            if (strcasecmp(entry->d_name, hconfig->index_files[i]) == 0) {
                char* dirindex = calloc(strlen(path)+strlen(entry->d_name)+2, sizeof(char));
                strcat(dirindex, path);
                strcat(dirindex, "/");
                strcat(dirindex, entry->d_name);
                closedir(dir);
                return dirindex;
            }
        }
    }
    closedir(dir);
    return NULL;
}
FILE * server_generate_directory_index(config_host *hconfig, const char* dirpath) {
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        return NULL;
    }
    
    UT_string *index = NULL;
    utstring_new(index);
    
    struct dirent *entry;
    struct stat *filestat = calloc(1, sizeof(struct stat));
    uint32_t count=0;
    while((entry = readdir(dir)) != NULL) {
        if (count++ > 1024*8) break;
        char *filepath = calloc(strlen(dirpath)+strlen(entry->d_name)+2, sizeof(char));
        strcat(filepath, dirpath);
        strcat(filepath, "/");
        strcat(filepath, entry->d_name);
        
        if (stat(filepath, filestat) < 0) {
            continue;
        }
        
        char* filesize = NULL;
        if (S_ISDIR(filestat->st_mode)) {
            filesize = strdup("[DIR]");
        } else if (S_ISREG(filestat->st_mode)) {
            filesize = server_get_filesize(filepath);
        } else {
            continue;
        }
        char* uri = strdup(filepath);
        uri = str_replace(uri, hconfig->serve_dir, "");
        if (uri[0] = '/') {
            memmove(uri, uri+1, strlen(uri+1)+1);
            uri = realloc(uri, strlen(uri)+1);
        }
        
        time_t file_mtime = (time_t)filestat->st_mtim.tv_sec;
        char* file_mod_time = calloc(32, sizeof(char));
        ctime_r(&file_mtime, file_mod_time);
        
        utstring_printf(index, "<tr><td><a href=\"%s\">%s</a></td><td>%s</td><td>%s</td></tr>\r\n", uri, uri,
                (filesize!=NULL)?filesize:"N/A", 
                (file_mod_time!=NULL)?file_mod_time:"N/A");
        free(file_mod_time);
        free(filepath);
        free(filesize);
        free(uri);
    }
    closedir(dir);
    free(filestat);
    char *dirname = strdup(dirpath);
    dirname = str_replace(dirname, hconfig->serve_dir, "/");
    
    file_map *dirindex_map = file_map_new("dirindex.html");
    if (dirindex_map == NULL) {
        utstring_free(index);
        free(dirname);
        return NULL;
    }
    char* dirindex = strdup(dirindex_map->map);
    file_map_delete(dirindex_map);
    
    dirindex = str_replace(dirindex, "{{dirname}}", dirname);
    free(dirname);
    dirindex = str_replace(dirindex, "{{index}}", utstring_body(index));
    utstring_free(index);
    
    FILE *file = tmpfile();
    fwrite(dirindex, sizeof(char), strlen(dirindex), file);
    free(dirindex);
    
    return file;
}
char* server_get_filesize(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    long int size = ftell(file);
    fclose(file);
    
    char* sizenames[] = {"b", "kb", "mb", "gb", "pb"};
    int size_i = 0;
    
    while(size > 1024) {
        if (size_i > (sizeof(sizenames) / sizeof(char*))) {
            break;
        }
        size_i++;
        size /= 1024;
    }
    char* sizestr = calloc(100, sizeof(char));
    snprintf(sizestr, 100, "%li%s", size, sizenames[size_i]);
    sizestr = realloc(sizestr, sizeof(char)*(strlen(sizestr)+1));
    return sizestr;
}

server_file_result* server_file_result_new() {
    server_file_result *result = calloc(1, sizeof(server_file_result));
    result->error = false;
    result->error_code = 0;
    result->error_text = NULL;
    result->dir = false;
    result->path = NULL;
    
    return result;
}
void server_file_result_seterror(server_file_result *result, uint16_t code, const char* message) {
    result->error = true;
    result->error_code = code;
    result->error_text = strdup(message);
}
void server_file_result_delete(server_file_result *result) {
    free(result->error_text);
    free(result->path);
    free(result);
}