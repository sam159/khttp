/* 
 * File:   util.h
 * Author: sam
 *
 * Created on 03 August 2014, 13:52
 */

#ifndef UTIL_H
#define	UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
    
#include "ut/utstring.h"
    
#define ALLOC_CHECK(ptr)                                        \
    do {                                                        \
        if (ptr==NULL) {                                        \
            fatal("Memory allocation failed. Out of memory?");  \
        }                                                       \
    } while(0)
    
    typedef struct file_map {
        char* map;
        size_t size;
    } file_map;
    
    void fatal(char* fmt, ...);
    void warning(bool use_errno, char* fmt, ...);
    void info(char* fmt, ...);

    char* str_trimwhitespace(char *str);
    char** str_splitlines(char *str, size_t *line_count);
    char* str_replace(char *str, const char *search, const char *replacement);

    file_map* file_map_new(const char* filename);
    void  file_map_delete(file_map* map);
    char* file_map_copyto_string(file_map* map, char* str, size_t str_len);
    void file_map_copyto_utstring(file_map* map, UT_string* string);

#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */

