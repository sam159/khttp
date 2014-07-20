/* 
 * File:   request.h
 * Author: sam
 *
 * Created on 18 July 2014, 14:43
 */

#ifndef REQUEST_H
#define	REQUEST_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../ut/utstring.h"
#include "http.h"
    
    http_response *parse_request(http_request *req, char *input);


#ifdef	__cplusplus
}
#endif

#endif	/* REQUEST_H */

