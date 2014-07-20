/* 
 * File:   main.h
 * Author: sam
 *
 * Created on 16 July 2014, 20:18
 */

#ifndef MAIN_H
#define	MAIN_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
    
    typedef struct file_map {
        char* map;
        size_t size;
    } file_map;

    int main(int argc, char** argv);

    void fatal(char* msg);
    void warning(char* msg, bool showPError);
    void info(char* msg, ...);

    char** str_splitlines(char *str, size_t *line_count);
    char* str_replace(char *str, const char *search, const char *replacement);

    file_map* map_file(const char* filename);
    void  free_mapped_file(file_map* map);


#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

