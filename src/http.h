/* 
 * File:   http.h
 * Author: sam
 *
 * Created on 18 July 2014, 14:15
 */

#ifndef HTTP_H
#define	HTTP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ut/utarray.h"
#include "util.h"
#include "http-body.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define HEADER_CONTENT_TYPE         "Content-Type" 
#define HEADER_CONTENT_LENGTH       "Content-Length"
#define HEADER_USER_AGENT           "User-Agent" 
#define HEADER_SERVER               "Server" 
#define HEADER_LAST_MODIFIED        "Last-Modified"
#define HEADER_LOCATION             "Location"
#define HEADER_HOST                 "Host"
#define HEADER_TRANSFER_ENCODING    "Transfer-Encoding"
#define HEADER_DATE                 "Date"
#define HEADER_CONNECTION           "Connection"
#define HEADER_IF_MODIFIED_SINCE    "If-Modified-Since"
#define HEADER_IF_UNMODIFIED_SINCE  "If-Unmodified-Since"
    
#define FORMAT_HEADER_DATE          "%a, %d %h %Y %T %Z"
#define DEFAULT_CONTENT_TYPE        "text/plain"

#define HTTP_CHUNK_MAXSIZE          1024*16
    
    typedef enum http_request_method {
        METHOD_GET, METHOD_POST, METHOD_HEAD, METHOD_PUT, 
        METHOD_DELETE, METHOD_OPTIONS, METHOD_TRACE, 
        METHOD_CONNECT, METHOD_OTHER, METHOD_INVALID
    } http_request_method;
    
    typedef enum http_version {
        HTTP10, HTTP11, HTTPXX
    } http_version;
    
    typedef struct http_request_line {
        http_request_method method;
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
    
    typedef UT_array http_header_list;
    
    extern UT_icd http_header_icd;
    
#define HTTP_HEADER_FOREACH(list, elem)                             \
        for (elem= (http_header*)utarray_next(list,NULL);  \
            elem!= NULL;                                             \
            elem=(http_header*)utarray_next(list,elem))
    
    typedef enum http_request_parsestatus {
        PARSE_REQUESTLINE, PARSE_HEADERS, PARSE_BODY, PARSE_DONE, PARSE_FAIL
    } http_request_parsestatus;
    
    typedef struct http_request {
        http_request_line *req;
        http_request_parsestatus parsestatus;
        http_header_list *headers;
        http_body *body;
        struct http_request *next;
    } http_request;
    
    typedef enum http_response_send_status {
        SEND_RESPONSE_LINE, SEND_HEADERS, SEND_BODY, SEND_DONE
    } http_response_send_status;
    
    typedef struct http_response {
        http_response_line *resp;
        http_header_list *headers;
        bool body_chunked;
        http_body *body;
        http_response_send_status send_status;
        struct http_response *next;
    } http_response;
    
#define HTTP_RESPONSE_LIST_FOREACH(list, elem) LL_FOREACH(list->first, elem)
#define HTTP_RESPONSE_LIST_FOREACH_SAFE(list, elem, tmp) LL_FOREACH_SAFE(list->first, elem, tmp)
    
    typedef struct http_response_list {
        http_response *first;
    } http_response_list;
    
    char* http_method_getstring(http_request_method method, char* method_other);
    http_request_method http_method_fromstring(const char* method);
    
    http_request_line* http_request_line_new(http_request_method method, const char* other);
    void http_request_line_delete(http_request_line *req);
    
    http_response_line* http_response_line_new(uint16_t code);
    char* http_response_line_get_message(http_response_line *resp);
    void http_response_line_delete(http_response_line *resp);
    
    http_header* http_header_new(const char* name, const char* content);
    void http_header_append_content(http_header *header, const char* content);
    void http_header_delete(http_header *header);
    
    http_header_list* http_header_list_new();
    void http_header_list_add(http_header_list* list, http_header *header, bool replace);
    http_header* http_header_list_get(http_header_list* list, const char* name);
    http_header** http_header_list_getall(http_header_list* list, const char* name, size_t *out_header_count);
    void http_header_list_remove(http_header_list *list, const char* name);
    void http_header_list_delete(http_header_list *list);
    
    http_request* http_request_new();
    void http_request_append_body(http_request *req, const char* body);
    char* http_request_write(http_request *req);
    void http_request_delete(http_request *req);
    
    http_response* http_response_new(http_response_line *resp);
    void http_response_append_body(http_response *resp, const char* body);
    void http_response_delete(http_response *resp);
    char* http_response_write(http_response *resp);
    http_response* http_response_create_builtin(uint16_t code, const char* errmsg);
    
    void http_chunks_write(char* source, UT_string* output);
    void http_chunks_terminate(http_header_list *footers, UT_string* output);
    
    http_response_list* http_response_list_new();
    void http_response_list_append(http_response_list *list, http_response* response);
    void http_response_list_remove(http_response_list *list, http_response* response);
    http_response* http_response_list_next(http_response_list *list);
    http_response* http_response_list_next2(http_response_list *list, bool remove);
    void http_response_list_delete(http_response_list *list);

#ifdef	__cplusplus
}
#endif

#endif	/* HTTP_H */

