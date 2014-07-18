#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "http.h"
#include "../main.h"

/*
 * METHOD_GET, METHOD_POST, METHOD_HEAD, METHOD_PUT, 
        METHOD_DELETE, METHOD_OPTIONS, METHOD_TRACE, 
        METHOD_CONNECT, METHOD_OTHER
 */

char* http_method_getstring(http_method method, char* method_other) {
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

http_method http_method_fromstring(const char* method) {
    http_method methods[] = {METHOD_GET, METHOD_POST, METHOD_HEAD, METHOD_PUT, 
        METHOD_DELETE, METHOD_OPTIONS, METHOD_TRACE, 
        METHOD_CONNECT};
    
    size_t count = sizeof(methods) / sizeof(http_method);
    for(int i=0; i < count; i++) {
        if (strcmp(http_method_getstring(methods[i],NULL), method) == 0) {
            return methods[i];
        }
    }
    return METHOD_INVALID;
}

http_request_line *http_request_line_new(http_method method, const char* other) {
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
void http_reponse_line_delete(http_response_line *resp) {
    free(resp->custom_message);
    free(resp);
}

http_header *http_header_new(const char* name) {
    http_header *header = calloc(1, sizeof(http_header));
    if (header == NULL) {
        fatal("calloc failed");
    }
    header->name = calloc(strlen(name)+1, sizeof(char));
    strcpy(header->name, name);
    
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
        uint32_t newlen = strlen(header->content) + strlen(content) + 1;
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

http_request *http_request_new() {
    http_request *req = calloc(1, sizeof(http_request));
    if (req == NULL) {
        fatal("calloc failed");
    }
    req->header_count = 0;
    req->body = NULL;
    req->parsestatus = PARSE_REQUESTLINE;
    return req;
}
void http_request_add_header(http_request *req, http_header *header) {
    req->header_count++;
    req->headers = realloc(req->headers, req->header_count * sizeof(http_header*));
    if (req->headers == NULL) {
        fatal("calloc failed");
    }
    req->headers[req->header_count-1] = header;
}
void http_request_apppend_body(http_request *req, const char* body) {
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
    http_request_line_delete(req->req);
    for(int i =0; i < req->header_count; i++) {
        http_header_delete(req->headers[i]);
    }
    free(req->headers);
    free(req->body);
    free(req);
}