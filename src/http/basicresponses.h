/* 
 * File:   basicresponses.h
 * Author: sam
 *
 * Created on 20 July 2014, 15:19
 */

#ifndef BASICRESPONSES_H
#define	BASICRESPONSES_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include "http.h"
#include "../main.h"
    
    http_response* response_create_builtin(uint16_t code, char* errmsg);

#ifdef	__cplusplus
}
#endif

#endif	/* BASICRESPONSES_H */

