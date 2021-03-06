#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

#include "ut/utlist.h"
#include "ut/utarray.h"
#include "ut/utstring.h"

#include "http.h"
#include "util.h"
#include "version.h"

void http_header_icd_init_f(void* elem) {
    memset(elem, 0, sizeof(http_header));
}
void http_header_icd_dtor_f(void* elem) {
    http_header *header = (http_header*)elem;
    if (header->name!=NULL) free(header->name);
    if (header->content!=NULL) free(header->content);
}

UT_icd http_header_icd = {sizeof(http_header), http_header_icd_init_f, NULL, http_header_icd_dtor_f};

char* http_method_getstring(http_request_method method, char* method_other) {
    
    switch(method) {
        case METHOD_GET:    return "GET";
        case METHOD_POST:   return "POST";
        case METHOD_HEAD:   return "HEAD";
        case METHOD_PUT:    return "PUT";
        case METHOD_DELETE: return "DELETE";
        case METHOD_OPTIONS:return "OPTIONS";
        case METHOD_TRACE:  return "TRACE";
        case METHOD_CONNECT:return "CONNECT";
        case METHOD_INVALID:return "<INVALID>";
        case METHOD_OTHER:  
            assert(method_other!=NULL); 
            return method_other;
        default: return "<INVALID#>";
    }
}

http_request_method http_method_fromstring(const char* method) {
    http_request_method methods[] = {METHOD_GET, METHOD_POST, METHOD_HEAD, METHOD_PUT, 
        METHOD_DELETE, METHOD_OPTIONS, METHOD_TRACE, 
        METHOD_CONNECT};
    
    size_t count = sizeof(methods) / sizeof(http_request_method);
    for(int i=0; i < count; i++) {
        if (strcmp(http_method_getstring(methods[i],NULL), method) == 0) {
            return methods[i];
        }
    }
    return METHOD_INVALID;
}

http_request_line *http_request_line_new(http_request_method method, const char* other) {
    http_request_line *req = calloc(1, sizeof(http_request_line));
    ALLOC_CHECK(req);
    req->method = method;
    if (req->method == METHOD_OTHER) {
        assert(other!=NULL);
        req->method_other = calloc(strlen(other)+1, sizeof(char));
        strcpy(req->method_other, other);
    } else {
        req->method_other = NULL;
    }
    req->version = HTTPXX;
    return req;
}
void http_request_line_delete(http_request_line *req) {
    assert(req!=NULL);
    if (req->method == METHOD_OTHER) {
        free(req->method_other);
    }
    free(req->uri);
    free(req);
}

http_response_line *http_response_line_new(uint16_t code) {
    http_response_line *resp = calloc(1, sizeof(http_response_line));
    ALLOC_CHECK(resp);
    resp->code = code;
    resp->version = HTTP11;
    return resp;
}
char* http_response_line_get_message(http_response_line *resp) {
    assert(resp!=NULL);
    if (resp->custom_message != NULL) {
        return resp->custom_message;
    }
    switch(resp->code) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default: return "";
    }
}
void http_response_line_delete(http_response_line *resp) {
    assert(resp!=NULL);
    free(resp->custom_message);
    free(resp);
}

http_header *http_header_new(const char* name, const char* content) {
    assert(name!=NULL);
    
    http_header *header = calloc(1, sizeof(http_header));
    ALLOC_CHECK(header);
    header->name = calloc(strlen(name)+1, sizeof(char));
    ALLOC_CHECK(header->name);
    strcpy(header->name, name);
    
    if (content != NULL) {
        http_header_append_content(header, content);
    }
    
    return header;
}
void http_header_append_content(http_header *header, const char* content) {
    assert(header!=NULL);
    assert(content!=NULL);
    
    if (header->content == NULL) {
        header->content = calloc(strlen(content)+1, sizeof(char));
        ALLOC_CHECK(header->content);
        strcpy(header->content, content);
    } else {
        size_t newlen = strlen(header->content) + strlen(content) + 1;
        header->content = realloc(header->content, newlen);
        ALLOC_CHECK(header->content);
        strcat(header->content, content);
    }
}
void http_header_delete(http_header *header) {
    assert(header!=NULL);
    
    free(header->name);
    free(header->content);
    free(header);
}

http_header_list* http_header_list_new() {
    http_header_list* list = calloc(1, sizeof(http_header_list));
    ALLOC_CHECK(list);
    utarray_init(list, &http_header_icd);
    return list;
}
void http_header_list_add(http_header_list* list, http_header *header, bool replace) {
    assert(list!=NULL);
    assert(header!=NULL);
    
    if (replace == true) {
        http_header_list_remove(list, header->name);
    }
    utarray_push_back(list, header);
    free(header);
}
http_header* http_header_list_get(http_header_list* list, const char* name) {
    assert(list!=NULL);
    
    if (name == NULL || strlen(name) == 0) {
        return NULL;
    }
    
    http_header *elem;
    HTTP_HEADER_FOREACH(list, elem) {
        if (strcmp(elem->name, name) == 0) {
            return elem;
        }
    }
    return NULL;
}
http_header** http_header_list_getall(http_header_list* list, const char* name, size_t *out_header_count) {
    assert(list!=NULL);
    assert(name!=NULL);
    assert(out_header_count!=NULL);
    
    http_header **headers = NULL;
    size_t count = 0;
    http_header *elem;
    HTTP_HEADER_FOREACH(list, elem) {
        if (strcmp(elem->name, name) == 0) {
            count++;
            headers = realloc(headers, count * sizeof(http_header*));
            ALLOC_CHECK(headers);
            headers[count-1] = elem;
        }
    }
    *out_header_count = count;
    return headers;
}
void http_header_list_remove(http_header_list *list, const char* name) {
    assert(list!=NULL);
    assert(name!=NULL);
    
    http_header **headers;
    size_t count;
    headers = http_header_list_getall(list, name, &count);
    for(int i=0; i<count; i++) {
        int pos = utarray_eltidx(list,headers[i]);
        utarray_erase(list, pos, 1);
    }
    free(headers);
}
void http_header_list_delete(http_header_list *list) {
    assert(list!=NULL);
    
    utarray_free(list);
}

http_request *http_request_new() {
    http_request *req = calloc(1, sizeof(http_request));
    ALLOC_CHECK(req);
    req->headers = http_header_list_new();
    req->parsestatus = PARSE_REQUESTLINE;
    req->body = http_body_new(BODY_NONE, NULL);
    return req;
}
void http_request_append_body(http_request *req, const char* body) {
    assert(req!=NULL);
    assert(body!=NULL);
    assert(req->body->type == BODY_STRING);
    
    http_body_append_str(req->body, body, strlen(body));
}
char* http_request_write(http_request *req) {
    assert(req!=NULL);
    assert(req->req!=NULL);
    
    UT_string *output = calloc(1, sizeof(UT_string));
    ALLOC_CHECK(output);
    utstring_init(output);
    
    utstring_printf(output, "%s %s %s\r\n", 
            http_method_getstring(req->req->method, req->req->method_other),
            req->req->uri,
            req->req->version == HTTP10 ? "HTTP/1.0" : "HTTP/1.1"
            );
    
    http_header *elem;
    HTTP_HEADER_FOREACH(req->headers, elem) {
        utstring_printf(output, "%s: %s\r\n",
                elem->name, elem->content);
    }
    
    utstring_printf(output, "\r\n");
    
    http_body_writeto_utstring(req->body, output);
    
    char* result = utstring_body(output);
    free(output);
    return result;
}
void http_request_delete(http_request *req) {
    assert(req!=NULL);
    
    if (req->req != NULL) {
        http_request_line_delete(req->req);
    }
    http_header_list_delete(req->headers);
    http_body_delete(req->body);
    free(req);
}

http_response* http_response_new(http_response_line *resp) {
    assert(resp!=NULL);
    
    http_response *response = calloc(1, sizeof(http_response));
    ALLOC_CHECK(resp);
    response->resp = resp;
    response->headers = http_header_list_new();
    response->body_chunked = false;
    response->body = http_body_new(BODY_NONE, NULL);
    response->send_status = SEND_RESPONSE_LINE;
    return response;
}
void http_response_append_body(http_response *resp, const char* body) {
    assert(resp!=NULL);
    assert(body!=NULL);
    assert(resp->body->type == BODY_STRING);
    
    http_body_append_str(resp->body, body, strlen(body));
}
void http_response_delete(http_response *resp) {
    assert(resp!=NULL);
    
    http_response_line_delete(resp->resp);
    http_header_list_delete(resp->headers);
    http_body_delete(resp->body);
    free(resp);
}
char* http_response_write(http_response *resp) {
    assert(resp!=NULL);
    assert(resp->resp !=NULL);
    
    if (resp->send_status == SEND_BODY || resp->send_status == SEND_DONE) {
        return NULL;
    }
    
    UT_string *output = calloc(1, sizeof(UT_string));
    ALLOC_CHECK(output);
    utstring_init(output);
    
    if (resp->send_status == SEND_RESPONSE_LINE) {
        if (resp->resp->version == HTTP10) {
            utstring_printf(output, "HTTP/1.0 ");
        } else if (resp->resp->version == HTTP11) {
            utstring_printf(output, "HTTP/1.1 ");
        }
        //Write the response line
        utstring_printf(output, "%hu %s\r\n", resp->resp->code, http_response_line_get_message(resp->resp));
        
        resp->send_status = SEND_HEADERS;
    }
    
    if (resp->send_status == SEND_HEADERS) {
        if (resp->resp->code != 100) { //No additional headers for Continue messages
            if (resp->body_chunked == false) {
                //Add content length header
                uint32_t messageLength = http_body_len(resp->body);
                char messageLengthStr[100];
                snprintf(messageLengthStr, 99, "%u", messageLength);
                http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_LENGTH, messageLengthStr), true);
            } else { //Chunked encoding
                http_header_list_add(resp->headers, http_header_new(HEADER_TRANSFER_ENCODING, "chunked"), true);
            }

            //Add content type if not defined
            http_header* contenttype = http_header_list_get(resp->headers, HEADER_CONTENT_TYPE);
            if (contenttype == NULL) {
                http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_TYPE, DEFAULT_CONTENT_TYPE), false);
            }

            //Add date header
            time_t timenow = time(NULL);
            struct tm * timeinfo = gmtime(&timenow);
            char dateStr[100] = {0};
            strftime(dateStr, 99, FORMAT_HEADER_DATE, timeinfo);
            http_header_list_add(resp->headers, http_header_new(HEADER_DATE, dateStr), true);

            //Add server identifier header
            http_header_list_add(resp->headers, http_header_new(HEADER_SERVER, SERVER_NAME), true);
        }
        //write headers
        http_header *elem;
        HTTP_HEADER_FOREACH(resp->headers, elem) {
            utstring_printf(output, "%s: %s\r\n", elem->name, elem->content);
        }
        utstring_printf(output, "\r\n");
        
        resp->send_status = SEND_BODY;
    }
    
    char* outputStr = utstring_body(output);
    free(output);
    return outputStr;
}

http_response* http_response_create_builtin(uint16_t code, const char* errmsg) {
    http_response *resp = http_response_new(http_response_line_new(code));
    http_body_set_type(resp->body, BODY_STRING);
    
    http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_TYPE, "text/html"), false);
    
    file_map* errorpage = file_map_new("error.html");
    if (errorpage != NULL) {
        http_response_append_body(resp, errorpage->map);
        file_map_delete(errorpage);
    } else {
        http_response_append_body(resp, "{{title}}\n\n{{message}}");
        http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_TYPE, "text/plain"), true);
    }
    
    char buffer[1024] = {0};
    
    char* title_message = http_response_line_get_message(resp->resp);
    snprintf(buffer, 1023, "%s %hu - %s", (code >= 400) ? "Error" : "Response Code", code, title_message);
    
    char* str = resp->body->data.str;
    
    str = str_replace(str, "{{title}}", buffer);
    str = str_replace(str, "{{body_title}}", buffer);
    if (errmsg == NULL) {
        errmsg = "";
    }
    str = str_replace(str, "{{message}}", errmsg);
    
    
    resp->body->data.str = str;
    
    return resp;
}

void http_chunks_write(char* source, UT_string* output) {
    assert(source!=NULL);
    assert(output!=NULL);
    
    size_t source_len = strlen(source);
    
    char buffer[HTTP_CHUNK_MAXSIZE+1] = {0};
    //determine how long the length line will be
    sprintf(buffer, "%zx;\r\n", (size_t)HTTP_CHUNK_MAXSIZE);
    size_t overhead = strlen(buffer);
    overhead+=3;//account for terminating CR + LF + \0
    buffer[0] = '\0';
    
    size_t i = 0;
    while (i < source_len) {
        //how much can we write in this chunk?
        size_t sourcerem = source_len - i;
        size_t chunk_len = sourcerem;
        if (chunk_len > HTTP_CHUNK_MAXSIZE-overhead) {
            chunk_len = HTTP_CHUNK_MAXSIZE-overhead;
        }
        //Write chunk length
        utstring_printf(output, "%zx;\r\n", chunk_len);
        //Write chunk data
        utstring_bincpy(output, source+i, chunk_len);
        i += chunk_len;
    }
}
void http_chunks_terminate(http_header_list *footers, UT_string* output) {
    assert(output!=NULL);
    
    utstring_printf(output, "0\r\n");
    if (footers != NULL) {
        //write footers
        http_header *elem;
        HTTP_HEADER_FOREACH(footers, elem) {
            utstring_printf(output, "%s: %s\r\n", elem->name, elem->content);
        }
    }
    utstring_printf(output, "\r\n");
}

http_response_list* http_response_list_new() {
    http_response_list *list = calloc(1, sizeof(http_response_list));
    ALLOC_CHECK(list);
    list->first = NULL; 
    return list;
}
void http_response_list_append(http_response_list *list, http_response* response) {
    assert(list != NULL);
    assert(response != NULL);
    
    LL_APPEND(list->first, response);
}
void http_response_list_remove(http_response_list *list, http_response* response) {
    assert(list != NULL);
    assert(response != NULL);
    
    LL_DELETE(list->first, response);
}
http_response* http_response_list_next(http_response_list *list) {
    assert(list != NULL);
    
    return http_response_list_next2(list, false);
}
http_response* http_response_list_next2(http_response_list *list, bool remove) {
    assert(list != NULL);
    
    if (list->first == NULL) {
        return NULL;
    }
    http_response* resp = list->first;
    if (remove == true) {
        LL_DELETE(list->first, resp);
        resp->next = NULL;
    }
    return resp;
}
void http_response_list_delete(http_response_list *list) {
    assert(list != NULL);
    
    http_response *elem, *tmp;
    HTTP_RESPONSE_LIST_FOREACH_SAFE(list, elem, tmp) {
        http_response_delete(elem);
    }
    free(list);
}