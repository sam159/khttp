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

#include <stdbool.h>
    
    typedef struct file_map {
        char* map;
        size_t size;
    } file_map;
    
    void fatal(char* msg);
    void warning(bool showPError, char* msg, ...);
    void info(char* msg, ...);

    char* str_trimwhitespace(char *str);
    char** str_splitlines(char *str, size_t *line_count);
    char* str_replace(char *str, const char *search, const char *replacement);

    file_map* file_map_new(const char* filename);
    void  file_map_delete(file_map* map);


#ifdef	__cplusplus
}
#endif

#endif	/* UTIL_H */
