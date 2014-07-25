/* 
 * File:   parse.h
 * Author: sam
 *
 * Created on 21 July 2014, 15:17
 */

#ifndef PARSE_H
#define	PARSE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "http_parser.h"
#include "http.h"
#include "../main.h"
    

    void parser_set_currentskt(skt_elem *elem);
    
    http_parser_settings* parser_get_settings(skt_elem *elem);
    
    int parser_cb_on_message_begin(http_parser* parser);
    int parser_cb_on_url(http_parser* parser, const char *at, size_t length);
    int parser_cb_on_status(http_parser* parser, const char *at, size_t length);
    int parser_cb_on_header_field(http_parser* parser, const char *at, size_t length);
    int parser_cb_on_header_value(http_parser* parser, const char *at, size_t length);
    int parser_cb_on_headers_complete(http_parser* parser);
    int parser_cb_on_body(http_parser* parser, const char *at, size_t length);
    int parser_cb_on_message_complete(http_parser* parser);

#ifdef	__cplusplus
}
#endif

#endif	/* PARSE_H */

