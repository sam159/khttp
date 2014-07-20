#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http.h"
#include "../main.h"

http_response* response_create_builtin(uint16_t code, char* errmsg) {
    http_response *resp = http_response_new(http_response_line_new(code));
    
    http_header_list_add(resp->headers, http_header_new(HEADER_CONTENT_TYPE, "text/html"), false);
    
    file_map* errorpage = map_file("content/error.html");
    http_response_append_body(resp, errorpage->map);
    free_mapped_file(errorpage);
    
    char buffer[1024] = {0};
    
    char* title_message = http_response_line_get_message(resp->resp);
    snprintf(buffer, 1023, "%s %hu - %s", (code >= 400) ? "Error" : "Response Code", code, title_message);
    resp->body = str_replace(resp->body, "{{title}}", buffer);
    resp->body = str_replace(resp->body, "{{body_title}}", buffer);
    
    resp->body = str_replace(resp->body, "{{message}}", errmsg);
    
    return resp;
}