#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <http_parser.h>
#include <assert.h>

#include "../main.h"
#include "http.h"
#include "parse.h"

#define GETSTR(str, at, length) do { \
    str = calloc(length+1, sizeof(char));\
    strncpy(str, at, length);\
}while(0);

http_parser_settings *parser_settings = NULL;

skt_elem *current_socket=NULL;

void parser_set_currentskt(skt_elem *elem) {
    current_socket = elem;
}

http_parser_settings* parser_get_settings(skt_elem *elem) {
    if (parser_settings == NULL) {
        parser_settings = calloc(1, sizeof(http_parser_settings));
        parser_settings->on_body = parser_cb_on_body;
        parser_settings->on_header_field = parser_cb_on_header_field;
        parser_settings->on_header_value = parser_cb_on_header_value;
        parser_settings->on_headers_complete = parser_cb_on_headers_complete;
        parser_settings->on_message_begin = parser_cb_on_message_begin;
        parser_settings->on_message_complete = parser_cb_on_message_complete;
        parser_settings->on_status_complete = parser_cb_on_status;
        parser_settings->on_url = parser_cb_on_url;
    }
    parser_set_currentskt(elem);
    return parser_settings;
}

int parser_cb_on_message_begin(http_parser* parser) {
    info("parser_cb_on_message_begin");
    if (current_socket->current_request != NULL) {
        http_request_delete(current_socket->current_request);
    }
    current_socket->current_request = http_request_new();

    return 0;
}
int parser_cb_on_url(http_parser* parser, const char *at, size_t length) {
    char* str;GETSTR(str,at,length);
    info("parser_cb_on_url: %s",str);
    current_socket->current_request->req = http_request_line_new(http_method_fromstring(http_method_str(parser->method)), NULL);
    current_socket->current_request->req->uri = str;
    return 0;
}
int parser_cb_on_status(http_parser* parser) {
    //Responses only, so ignored
    info("parser_cb_on_status");
    return 0;
}
int parser_cb_on_header_field(http_parser* parser, const char *at, size_t length) {
    char* str;GETSTR(str,at,length);
    info("parser_cb_on_header_field: %s",str);
    
    if (current_socket->parser_header_state == HSTATE_NONE) {
        //First call, new header
        if (current_socket->parser_current_header != NULL) {
            http_header_delete(current_socket->parser_current_header);
        }
        current_socket->parser_current_header = http_header_new(str, NULL);
    } else if (current_socket->parser_header_state == HSTATE_VALUE) {
        //New header
        if (current_socket->parser_current_header != NULL) {
            http_header_list_add(current_socket->current_request->headers, current_socket->parser_current_header, false);
        }
        current_socket->parser_current_header = http_header_new(str, NULL);
    } else if (current_socket->parser_header_state == HSTATE_FIELD) {
        //continuation of current headers name
        http_header* header = current_socket->parser_current_header;
        size_t newlen = strlen(header->name) + length +1;
        header->name = realloc(header->name, newlen * sizeof(char));
    } else {
        return 1;
    }
    
    current_socket->parser_header_state = HSTATE_FIELD;
    free(str);
    return 0;
    
}
int parser_cb_on_header_value(http_parser* parser, const char *at, size_t length) {
    char* str;GETSTR(str,at,length);
    info("parser_cb_on_header_value: %s",str);
    
    http_header_append_content(current_socket->parser_current_header, str);
    
    current_socket->parser_header_state = HSTATE_VALUE;
    free(str);
    return 0;
}
int parser_cb_on_headers_complete(http_parser* parser) {
    info("parser_cb_on_headers_complete");
    
    //save current header
    if (current_socket->parser_current_header != NULL) {
        http_header_list_add(current_socket->current_request->headers, current_socket->parser_current_header, false);
        current_socket->parser_current_header = NULL;
    }
    
    //the http version should also be set now
    if (parser->http_major == 1) {
        if (parser->http_minor == 0) {
            current_socket->current_request->req->version = HTTP10;
        } else if (parser->http_minor == 1) {
            current_socket->current_request->req->version = HTTP11;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
    
    return 0;
}
int parser_cb_on_body(http_parser* parser, const char *at, size_t length) {
    char* str;GETSTR(str,at,length);
    info("parser_cb_on_body: %s",str);
    
    http_request_append_body(current_socket->current_request, str);
    
    free(str);
    return 0;
}
int parser_cb_on_message_complete(http_parser* parser) {
    info("parser_cb_on_message_complete");
    
    current_socket->request_complete = true;
    
    return 0;
}