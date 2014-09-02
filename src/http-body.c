#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <bits/stdio2.h>

#include "util.h"
#include "ut/utstring.h"
#include "http-body.h"

http_body_write_result _http_body_file_fill_buffer(char *buffer, size_t buf_len, size_t *read_len, FILE *src) {
    assert(buffer!=NULL);
    assert(buf_len>0);
    assert(src!=NULL);
    assert(read_len!=NULL);
    
    if (*read_len > buf_len) {
        *read_len = buf_len;
    }
    size_t read_count = fread(buffer, sizeof(char), *read_len, src);
    if (read_count < *read_len) {
        if (ferror(src) != 0) {
            return HBWRITE_ERROR;
        }
    }
    *read_len = read_count;
    if (feof(src) != 0) {
        return HBWRITE_DONE;
    }
    return HBWRITE_MORE;
}

http_body* http_body_new(http_body_type type, void* dataptr) {
    http_body *body = calloc(1, sizeof(http_body));
    ALLOC_CHECK(body);
    
    body->type = type;
    body->rOffset = 0;
    switch(body->type) {
        case BODY_NONE:
            break;
        case BODY_STRING:
            body->data.str = (char*)dataptr;
            break;
        case BODY_FILEMAP:
            body->data.filemap = (file_map*)dataptr;
            break;
        case BODY_FILE:
            body->data.file = (FILE*)dataptr;
            break;
        default:
            errno = EINVAL;
            fatal("Invalid http body type");
            break;
    }
    
    return body;
}
void http_body_clear(http_body *body) {
    assert(body!=NULL);
    switch(body->type) {
        case BODY_NONE:
            break;
        case BODY_STRING:
            if (body->data.str != NULL) {
                free(body->data.str);
            }
            break;
        case BODY_FILEMAP:
            if (body->data.filemap != NULL) {
                file_map_delete(body->data.filemap);
            }
            break;
        case BODY_FILE:
            if (body->data.file != NULL) {
                fclose(body->data.file);
            }
            break;
        default:
            errno = EINVAL;
            fatal("Invalid http body type");
            break;
    }
}
void http_body_delete(http_body *body) {
    assert(body!=NULL);
    
    http_body_clear(body);
    free(body);
}

size_t http_body_append_str(http_body *body, const char* str, ssize_t str_len) {
    assert(body!=NULL);
    assert(body->type==BODY_STRING);
    assert(str!=NULL);
    if (str_len == 0) {
        return 0;
    }
    size_t new_len = strlen(body->data.str)+str_len+1;
    
    body->data.str = realloc(body->data.str, new_len);
    ALLOC_CHECK(body->data.str);
    body->data.str[new_len-1] = '\0';
    strncat(body->data.str, str, new_len-1);
    return new_len-1;
}
size_t http_body_len(http_body *body) {
    assert(body!=NULL);
    size_t len = 0;
    switch(body->type) {
        case BODY_NONE:
            break;
        case BODY_STRING:
            if (body->data.str !=NULL) {
                len = strlen(body->data.str);
            }
            break;
        case BODY_FILEMAP:
            if (body->data.filemap != NULL) {
                len = body->data.filemap->size;
            }
            break;
        case BODY_FILE:
            if (body->data.file != NULL) {
                size_t curpos = ftell(body->data.file);
                fseek(body->data.file, 0, SEEK_END);
                len = ftell(body->data.file);
                fseek(body->data.file, curpos, SEEK_SET);
            }
            break;
        default:
            errno = EINVAL;
            fatal("Invalid http body type");
            break;
    }
    return len;
}
void http_body_set_type(http_body *body, http_body_type newtype) {
    assert(body!=NULL);
    http_body_clear(body);
    body->type = newtype;
}

//TODO: replace these with single function w/callbacks

http_body_write_result http_body_writeto_fd(http_body *body, int fd) {
    assert(body!=NULL);
    if (body->type!=BODY_NONE) {
        assert(body->data.ptr!=NULL);
    }
    
    const size_t buffer_len = 16*1024;
    char* buffer= NULL;
    if (body->type == BODY_FILE) {
        buffer = calloc(buffer_len, sizeof(char));
        ALLOC_CHECK(buffer);
    }
    
    while(body->rOffset<http_body_len(body)) {
        size_t write_len = http_body_len(body) - body->rOffset;
        errno = EINVAL;
        ssize_t written = -1;
        http_body_write_result result = HBWRITE_MORE;
        
        switch(body->type) {
            case BODY_STRING:
                written = write(fd, body->data.str+body->rOffset, write_len);
                break;
            case BODY_FILEMAP:
                written = write(fd, body->data.filemap->map+body->rOffset, write_len);
                break;
            case BODY_FILE:;
                size_t read_count = write_len;
                http_body_write_result read_res = _http_body_file_fill_buffer(buffer, buffer_len, &read_count, body->data.file);
                if (read_res == HBWRITE_ERROR) {
                    result = read_res; break;
                }
                if (read_count > 0) {
                    written = write(fd, buffer, read_count);
                }
                if (read_res == HBWRITE_DONE) {
                    result = read_res; break;
                }
                break;
        }//switch body->type
        
        if (written < 0 || result == HBWRITE_ERROR) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return HBWRITE_BLOCKED;
            }
            return HBWRITE_ERROR;
        }
        
        body->rOffset += written;
        if (result == HBWRITE_DONE) {
            break;
        }
    }//While data remaining
    
    if (buffer != NULL) {
        free(buffer);
    }
    errno = 0;
    return HBWRITE_DONE;
}
http_body_write_result http_body_writeto_str(http_body *body, char** str) {
    assert(body!=NULL);
    if (body->type!=BODY_NONE) {
        assert(body->data.ptr!=NULL);
    }
    assert(str!=NULL);
    
    char* buffer=NULL;
    const size_t buffer_len = 16*1024;
    if (body->type == BODY_FILE) {
        buffer = calloc(buffer_len, sizeof(char));
        ALLOC_CHECK(buffer);
    }
    
    while(body->rOffset<http_body_len(body)) {
        size_t write_len = http_body_len(body) - body->rOffset;
        if (*str==NULL) {
            *str = calloc(write_len, sizeof(char));
        } else {
            *str = realloc(*str, sizeof(char)*(strlen(*str)+write_len+1));
        }
        ALLOC_CHECK(*str);
        
        ssize_t written = -1;
        http_body_write_result result = HBWRITE_MORE;
        
        switch(body->type) {
            case BODY_STRING:
            case BODY_FILEMAP:;
                char* src = (body->type==BODY_STRING) ? body->data.str : body->data.filemap->map;
                strncat(*str, src+body->rOffset, write_len);
                written = write_len;
                break;
            case BODY_FILE:;
                size_t read_count = write_len;
                http_body_write_result read_res = _http_body_file_fill_buffer(buffer, buffer_len, &read_count, body->data.file);
                if (read_res == HBWRITE_ERROR) {
                    result = read_res; break;
                }
                if (read_count > 0) {
                    strncat(*str, buffer, read_count);
                    written = read_count;
                }
                if (read_res == HBWRITE_DONE) {
                    result = read_res; break;
                }
                break;
        }//Switch body->type
        
        body->rOffset += written;
        if (result == HBWRITE_ERROR) {
            if (buffer != NULL) {
                free(buffer);
            }
            return HBWRITE_ERROR;
        }
        if (result == HBWRITE_DONE) {
            break;
        }
    }//while data remaining
    
    if (buffer != NULL) {
        free(buffer);
    }
    return HBWRITE_DONE;
}
http_body_write_result http_body_writeto_utstring(http_body *body, UT_string *utstr) {
    assert(body!=NULL);
    if (body->type!=BODY_NONE) {
        assert(body->data.ptr!=NULL);
    }
    assert(utstr!=NULL);
    
    char* buffer=NULL;
    const size_t buffer_len = 16*1024;
    if (body->type == BODY_FILE) {
        buffer = calloc(buffer_len, sizeof(char));
        ALLOC_CHECK(buffer);
    }
    
    while(body->rOffset<http_body_len(body)) {
        size_t write_len = http_body_len(body) - body->rOffset;
        
        ssize_t written = -1;
        http_body_write_result result = HBWRITE_MORE;
        
        switch(body->type) {
            case BODY_STRING:
            case BODY_FILEMAP:;
                char* src = (body->type==BODY_STRING) ? body->data.str : body->data.filemap->map;
                utstring_bincpy(utstr, src, write_len);
                written = write_len;
                break;
            case BODY_FILE:;
                size_t read_count = write_len;
                http_body_write_result read_res = _http_body_file_fill_buffer(buffer, buffer_len, &read_count, body->data.file);
                if (read_res == HBWRITE_ERROR) {
                    result = read_res; break;
                }
                if (read_count > 0) {
                    utstring_bincpy(utstr, src, write_len);
                    written = read_count;
                }
                if (read_res == HBWRITE_DONE) {
                    result = read_res; break;
                }
                break;
        }//Switch body->type
        
        body->rOffset += written;
        if (result == HBWRITE_ERROR) {
            if (buffer != NULL) {
                free(buffer);
            }
            return HBWRITE_ERROR;
        }
        if (result == HBWRITE_DONE) {
            break;
        }
    }//while data remaining
    
    if (buffer != NULL) {
        free(buffer);
    }
    return HBWRITE_DONE;
}
