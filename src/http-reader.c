#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "util.h"
#include "http.h"
#include "server-connection.h"
#include "http-reader.h"

#define GET_CB_STR(str, at, length) do { \
    str = calloc(length+1, sizeof(char));\
    strncpy(str, at, length);\
}while(0);
#define SKT(parser) ((request_parse_state*)parser->data)

http_parser_settings *parser_settings = NULL;

http_parser_settings* parser_get_settings() {
    if (parser_settings == NULL) {
        parser_settings = calloc(1, sizeof(http_parser_settings));
        parser_settings->on_body = parser_cb_on_body;
        parser_settings->on_header_field = parser_cb_on_header_field;
        parser_settings->on_header_value = parser_cb_on_header_value;
        parser_settings->on_headers_complete = parser_cb_on_headers_complete;
        parser_settings->on_message_begin = parser_cb_on_message_begin;
        parser_settings->on_message_complete = parser_cb_on_message_complete;
        parser_settings->on_status = parser_cb_on_status;
        parser_settings->on_url = parser_cb_on_url;
        
    }
    return parser_settings;
}
void parser_free_settings() {
    if (parser_settings != NULL) {
        free(parser_settings);
        parser_settings = NULL;
    }
}

int parser_cb_on_message_begin(http_parser* parser) {
    if (SKT(parser)->current_request != NULL) {
        http_request_delete(SKT(parser)->current_request);
    }
    SKT(parser)->current_request = http_request_new();

    return 0;
}
int parser_cb_on_url(http_parser* parser, const char *at, size_t length) {
    char* str;GET_CB_STR(str,at,length);
    SKT(parser)->current_request->req = http_request_line_new(http_method_fromstring(http_method_str(parser->method)), NULL);
    SKT(parser)->current_request->req->uri = str;
    return 0;
}
int parser_cb_on_status(http_parser* parser, const char *at, size_t length) {
    //Responses only, so ignored
    info("parser_cb_on_status");
    return 0;
}
int parser_cb_on_header_field(http_parser* parser, const char *at, size_t length) {
    char* str;GET_CB_STR(str,at,length);
    
    if (SKT(parser)->parser_header_state == HSTATE_NONE) {
        //First call, new header
        if (SKT(parser)->parser_current_header != NULL) {
            http_header_delete(SKT(parser)->parser_current_header);
        }
        SKT(parser)->parser_current_header = http_header_new(str, NULL);
        
        //the http version should also be set now
        if (parser->http_major == 1) {
            if (parser->http_minor == 0) {
                SKT(parser)->current_request->req->version = HTTP10;
            } else if (parser->http_minor == 1) {
                SKT(parser)->current_request->req->version = HTTP11;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    } else if (SKT(parser)->parser_header_state == HSTATE_VALUE) {
        //New header
        if (SKT(parser)->parser_current_header != NULL) {
            http_header_list_add(SKT(parser)->current_request->headers, SKT(parser)->parser_current_header, false);
        }
        SKT(parser)->parser_current_header = http_header_new(str, NULL);
    } else if (SKT(parser)->parser_header_state == HSTATE_FIELD) {
        //continuation of current headers name
        http_header* header = SKT(parser)->parser_current_header;
        size_t newlen = strlen(header->name) + length +1;
        header->name = realloc(header->name, newlen * sizeof(char));
        strcat(header->name, str);
    } else {
        return 1;
    }
    
    SKT(parser)->parser_header_state = HSTATE_FIELD;
    free(str);
    return 0;
    
}
int parser_cb_on_header_value(http_parser* parser, const char *at, size_t length) {
    char* str;GET_CB_STR(str,at,length);
    
    http_header_append_content(SKT(parser)->parser_current_header, str);
    
    SKT(parser)->parser_header_state = HSTATE_VALUE;
    free(str);
    return 0;
}
int parser_cb_on_headers_complete(http_parser* parser) {
    
    //save current header
    if (SKT(parser)->parser_current_header != NULL) {
        http_header_list_add(SKT(parser)->current_request->headers, SKT(parser)->parser_current_header, false);
        SKT(parser)->parser_current_header = NULL;
    }
    
    return 0;
}
int parser_cb_on_body(http_parser* parser, const char *at, size_t length) {
    char* str;GET_CB_STR(str,at,length);
    
    http_request_append_body(SKT(parser)->current_request, str);
    
    free(str);
    return 0;
}
int parser_cb_on_message_complete(http_parser* parser) {
    
    SKT(parser)->request_complete = true;
    
    return 0;
}