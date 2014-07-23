#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "../main.h"
#include "../ut/utarray.h"
#include "../ut/utstring.h"
#include "http.h"

/*
 * METHOD_GET, METHOD_POST, METHOD_HEAD, METHOD_PUT, 
        METHOD_DELETE, METHOD_OPTIONS, METHOD_TRACE, 
        METHOD_CONNECT, METHOD_OTHER
 */

void http_header_icd_init_f(void* elem) {
    memset(elem, 1, sizeof(http_header));
}

UT_icd http_header_icd = {sizeof(http_header), http_header_icd_init_f, NULL, NULL};

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
        case METHOD_OTHER:  return method_other;
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
    if (req == NULL) {
        fatal("calloc failed");
    }
    req->method = method;
    if (req->method == METHOD_OTHER) {
        req->method_other = calloc(strlen(other)+1, sizeof(char));
        strcpy(req->method_other, other);
    } else {
        req->method_other = NULL;
    }
    return req;
}
void http_request_line_delete(http_request_line *req) {
    free(req->method_other);
    free(req->uri);
    free(req);
}

http_response_line *http_response_line_new(uint16_t code) {
    http_response_line *resp = calloc(1, sizeof(http_response_line));
    if (resp == NULL) {
        fatal("calloc failed");
    }
    resp->code = code;
    resp->version = HTTP11;
    return resp;
}
char* http_response_line_get_message(http_response_line *resp) {
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
    free(resp->custom_message);
    free(resp);
}

http_header *http_header_new(const char* name, const char* content) {
    http_header *header = calloc(1, sizeof(http_header));
    if (header == NULL) {
        fatal("calloc failed");
    }
    header->name = calloc(strlen(name)+1, sizeof(char));
    strcpy(header->name, name);
    
    if (content != NULL) {
        http_header_append_content(header, content);
    }
    
    return header;
}
void http_header_append_content(http_header *header, const char* content) {
    if (header->content == NULL) {
        header->content = calloc(strlen(content)+1, sizeof(char));
        if (header->content == NULL) {
            fatal("calloc failed");
        }
        strcpy(header->content, content);
    } else {
        size_t newlen = strlen(header->content) + strlen(content) + 1;
        header->content = realloc(header->content, newlen);
        if (header->content == NULL) {
            fatal("calloc failed");
        }
        strcat(header->content, content);
    }
}
void http_header_delete(http_header *header) {
    free(header->name);
    free(header->content);
    free(header);
}

http_header_list* http_header_list_new() {
    http_header_list* list = NULL;
    utarray_new(list, &http_header_icd);
    return list;
}
void http_header_list_add(http_header_list* list, http_header *header, bool replace) {
    if (replace == true) {
        http_header_list_remove(list, header->name);
    }
    utarray_push_back(list, header);
    free(header);
}
http_header* http_header_list_get(http_header_list* list, const char* name) {
    HTTP_HEADER_FOREACH(list, elem) {
        if (strcmp(elem->name, name) == 0) {
            return elem;
        }
    }
    return NULL;
}
http_header** http_header_list_getall(http_header_list* list, const char* name, size_t *out_header_count) {
    http_header **headers = NULL;
    size_t count = 0;
    HTTP_HEADER_FOREACH(list, elem) {
        if (strcmp(elem->name, name) == 0) {
            count++;
            headers = realloc(headers, count * sizeof(http_header*));
            headers[count-1] = elem;
        }
    }
    *out_header_count = count;
    return headers;
}
void http_header_list_remove(http_header_list *list, const char* name) {
    http_header **headers;
    size_t count;
    headers = http_header_list_getall(list, name, &count);
    for(int i=0; i<count; i++) {
        int pos = utarray_eltidx(list,headers[i]);
        free(headers[i]->name);
        free(headers[i]->content);
        utarray_erase(list, pos, 1);
    }
    free(headers);
}
void http_header_list_delete(http_header_list *list) {
    HTTP_HEADER_FOREACH(list, elem) {
        free(elem->name);
        free(elem->content);
    }
    utarray_free(list);
}

http_request *http_request_new() {
    http_request *req = calloc(1, sizeof(http_request));
    if (req == NULL) {
        fatal("calloc failed");
    }
    req->headers = http_header_list_new();
    req->parsestatus = PARSE_REQUESTLINE;
    return req;
}
void http_request_append_body(http_request *req, const char* body) {
    uint32_t bodylen = 0;
    if (req->body != NULL) {
        bodylen = strlen(req->body);
    }
    bodylen += strlen(body) + 1;
    req->body = realloc(req->body, bodylen * sizeof(char));
    if (req->body == NULL) {
        fatal("calloc failed");
    }
    strcat(req->body, body);
}
void http_request_delete(http_request *req) {
    if (req->req != NULL) {
        http_request_line_delete(req->req);
    }
    http_header_list_delete(req->headers);
    free(req->body);
    free(req);
}
    
http_response* http_response_new(http_response_line *resp) {
    http_response *response = calloc(1, sizeof(http_response));
    response->resp = resp;
    response->headers = http_header_list_new();
    response->body = NULL;
    return response;
}
void http_response_append_body(http_response *resp, const char* body) {
    uint32_t bodylen = 0;
    if (resp->body != NULL) {
        bodylen = strlen(resp->body);
    }
    bodylen += strlen(body) + 1;
    if (resp->body == NULL) {
        resp->body = calloc(bodylen, sizeof(char));
    } else {
        resp->body = realloc(resp->body, bodylen * sizeof(char));
    }
    strcat(resp->body, body);
}
void http_response_delete(http_response *resp) {
    http_response_line_delete(resp->resp);
    http_header_list_delete(resp->headers);
    free(resp->body);
    free(resp);
}
char* http_response_write(http_response *resp) {
    UT_string *output = calloc(1, sizeof(UT_string));
    utstring_init(output);
    
    if (resp->resp->version == HTTP10) {
        utstring_printf(output, "HTTP/1.0 ");
    } else if (resp->resp->version == HTTP11) {
        utstring_printf(output, "HTTP/1.1 ");
    }
    //Write the response line
    utstring_printf(output, "%hu %s\r\n", resp->resp->code, http_response_line_get_message(resp->resp));
    
    if (resp->resp->code != 100) { //No additional headers for Continue messages
        //Add content length header
        uint32_t messageLength = 0;
        if (resp->body != NULL) {
            messageLength = strlen(resp->body);
        }
        char messageLengthStr[100];
        snprintf(messageLengthStr, 99, "%u", messageLength);
        http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_LENGTH, messageLengthStr), true);
        
        //Add date header
        time_t timenow = time(NULL);
        struct tm * timeinfo = gmtime(&timenow);
        char dateStr[100] = {0};
        strftime(dateStr, 99, FORMAT_HEADER_DATE, timeinfo);
        http_header_list_add(resp->headers, http_header_new(HEADER_DATE, dateStr), true);
    }
    
    //write headers
    HTTP_HEADER_FOREACH(resp->headers, elem) {
        utstring_printf(output, "%s: %s\r\n", elem->name, elem->content);
    }
    utstring_printf(output, "\r\n");
    
    //Write the request
    //TODO: chunked support for output
    if (resp->body != NULL) {
        utstring_printf(output, "%s", resp->body);
    }
    char* outputStr = utstring_body(output);
    free(output);
    return outputStr;
}

http_response* http_response_create_builtin(uint16_t code, char* errmsg) {
    http_response *resp = http_response_new(http_response_line_new(code));
    
    http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_TYPE, "text/html"), false);
    
    file_map* errorpage = map_file("content/error.html");
    if (errorpage != NULL) {
        http_response_append_body(resp, errorpage->map);
        free_mapped_file(errorpage);
    } else {
        http_response_append_body(resp, "{{title}}\n\n{{message}}");
        http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_TYPE, "text/plain"), true);
    }
    
    char buffer[1024] = {0};
    
    char* title_message = http_response_line_get_message(resp->resp);
    snprintf(buffer, 1023, "%s %hu - %s", (code >= 400) ? "Error" : "Response Code", code, title_message);
    resp->body = str_replace(resp->body, "{{title}}", buffer);
    resp->body = str_replace(resp->body, "{{body_title}}", buffer);
    
    resp->body = str_replace(resp->body, "{{message}}", errmsg);
    
    return resp;
}