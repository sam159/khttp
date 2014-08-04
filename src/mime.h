/* 
 * File:   mime.h
 * Author: sam
 *
 * Created on 29 July 2014, 19:38
 */

#ifndef MIME_H
#define	MIME_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "ut/utlist.h"
    
#define MIME_DEFAULT_FILE "/etc/mime.types"

    typedef struct mime_type {
        char* mime;
        char* extension;
        struct mime_type *next;
    } mime_type;
    
    extern mime_type *mime_list;
    
    int mime_init(const char* file);
    void mime_destroy();
    
    int mime_load(mime_type **list, const char* file);
    void mime_free(mime_type *list);
    void mime_print_all(mime_type *list);
    
    const char* mime_get_type(const char* filename, const char* fallback);
    const char* mime_list_get_type(mime_type *list, const char* filename, const char* fallback);
    const char* mime_get_type_magic(const char* filename, const char* fallback);

#ifdef	__cplusplus
}
#endif

#endif	/* MIME_H */

