#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "../main.h"
#include "http.h"
#include "basicresponses.h"

http_response* parse_request(http_request *req, char *input) {
    size_t line_count;
    char** lines = str_splitlines(input, &line_count);
    http_response* response = NULL;
    
    switch(req->parsestatus) {
        case PARSE_REQUESTLINE:
            if (line_count == 0) {
                break;
            }
            
            char* requestStr = lines[0];
            
            char methodStr[20] = {0};
            char uriStr[1024] = {0};
            char versionStr[16] = {0};
            
            int count = sscanf(requestStr, "%19s%*[ \t]%1023s%*[ \t]%15s", methodStr, uriStr, versionStr);
            if (count < 3) {
                response = response_create_builtin(400, "Could not parse request line");
                req->parsestatus = PARSE_FAIL;
                break;
            }
            
            http_method method = http_method_fromstring(methodStr);
            if (method == METHOD_INVALID) {
                response = response_create_builtin(405, "");
                req->parsestatus = PARSE_FAIL;
                break;
            }
            
            http_version version;
            if (strcasecmp(versionStr, "HTTP/1.0") == 0) { version = HTTP10; }
            else if (strcasecmp(versionStr, "HTTP/1.1") == 0) { version = HTTP11; }
            else {
                response = response_create_builtin(505, "");
                req->parsestatus = PARSE_FAIL;
                break;
            }
            
            http_request_line *request_line = http_request_line_new(method, NULL);
            request_line->version = version;
            request_line->uri = calloc(strlen(uriStr)+1, sizeof(char));
            if (request_line->uri == NULL) {
                fatal("calloc failed");
            }
            strcpy(request_line->uri, uriStr);
            req->req = request_line;
            req->parsestatus = PARSE_HEADERS;
            
            if (req->req->version == HTTP11) {
                response = http_response_new(http_response_line_new(100));
            }
            
            break;
        case PARSE_HEADERS:
            break;
        case PARSE_BODY:
            break;
        case PARSE_DONE:
        case PARSE_FAIL:
            break;
    }
    
    for(size_t i=0; i < line_count; i++) {
        free(lines[i]);
        free(lines);
    }
    
    return response;
}