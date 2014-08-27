/* 
 * File:   http-body.h
 * Author: sam
 *
 * Created on 27 August 2014, 20:17
 */

#ifndef HTTP_BODY_H
#define	HTTP_BODY_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stddef.h>
#include <stdio.h>

#include "util.h"
#include "ut/utstring.h"
    
    typedef enum http_body_type {
        BODY_NONE, BODY_STRING, BODY_FILEMAP, BODY_FILE
    } http_body_type;
    
    typedef enum http_body_write_result {
        HBWRITE_DONE, HBWRITE_MORE, HBWRITE_BLOCKED, HBWRITE_ERROR
    } http_body_write_result;
    
    typedef struct http_body {
        http_body_type type;
        size_t rOffset;
        union {
            char* str;
            file_map *filemap;
            FILE *file;
            void *ptr;
        } data;
    } http_body;
    
    http_body* http_body_new(http_body_type type, void* dataptr);
    void http_body_delete(http_body *body);
    
    size_t http_body_append_str(http_body *body, const char** str_ptr);
    size_t http_body_len(http_body *body);
    
    http_body_write_result http_body_writeto_fd(http_body *body, int fd);
    http_body_write_result http_body_writeto_str(http_body *body, const char* str, size_t str_len);
    http_body_write_result http_body_writeto_utstring(http_body *body, UT_string *utstr);


#ifdef	__cplusplus
}
#endif

#endif	/* HTTP_BODY_H */

