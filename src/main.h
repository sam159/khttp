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
    
int main(int argc, char** argv);

void fatal(char* msg);
void warning(char* msg, bool showPError);
void info(char* msg, ...);

char** str_splitlines(char *str, size_t *line_count);


#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

