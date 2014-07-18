/* 
 * File:   http.h
 * Author: sam
 *
 * Created on 18 July 2014, 14:15
 */

#ifndef HTTP_H
#define	HTTP_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include "request.h"

    typedef enum http_method {
        METHOD_GET, METHOD_POST, METHOD_HEAD, METHOD_PUT, 
        METHOD_DELETE, METHOD_OPTIONS, METHOD_TRACE, 
        METHOD_CONNECT, METHOD_OTHER, METHOD_INVALID
    } http_method;
    
    typedef enum http_version {
        HTTP10, HTTP11
    } http_version;
    
    typedef struct http_request_line {
        http_method method;
        char* method_other;
        char* uri;
        http_version version;
    } http_request_line;
    
    typedef struct http_response_line {
        http_version version;
        uint16_t code;
        char* custom_message;
    } http_response_line;

    typedef struct http_header {
        char* name;
        char* content;
    } http_header;
    
    typedef struct http_request {
        http_request_line *req;
        http_header **headers;
        uint32_t header_count;
        char *body;
    } http_request;
    
    char* http_method_getstring(http_method method, char* method_other);
    http_method http_method_fromstring(const char* method);
    
    http_request_line* http_request_line_new(http_method method, const char* other);
    void http_request_line_delete(http_request_line *req);
    
    http_response_line* http_response_line_new(uint16_t code);
    char* http_response_line_get_message(http_response_line *resp);
    void http_reponse_line_delete(http_response_line *resp);
    
    http_header* http_header_new(const char* name);
    void http_header_append_content(http_header *header, const char* content);
    void http_header_delete(http_header *header);
    
    http_request* http_request_new();
    void http_request_add_header(http_request *req, http_header *header);
    void http_request_apppend_body(http_request *req, const char* body);
    void http_request_delete(http_request *req);
    

#ifdef	__cplusplus
}
#endif

#endif	/* HTTP_H */

