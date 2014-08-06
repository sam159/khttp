#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>

#include "util.h"
#include "log.h"

void fatal(char* fmt, ...) {
    char msg[LOG_LENGTH] = {0};
    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, LOG_LENGTH, fmt, va);
    va_end(va);
    
    LOG(LFATAL, msg);
    log_register_clear();
    exit(EXIT_FAILURE);
}
void warning(bool use_errno, char* fmt, ...) {
    char msg[LOG_LENGTH] = {0};
    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, LOG_LENGTH, fmt, va);
    va_end(va);
    
    if (use_errno == true) {
        char *errnostr = calloc(64, sizeof(char));
        strerror_r(errno, errnostr, 64);
        char *errstr = calloc(strlen(msg)+strlen(errnostr)+3, sizeof(char));
        strcat(errstr, msg);
        strcat(errstr, ": ");
        strcat(errstr, errnostr);
        LOG(LWARNING, errstr);
        free(errnostr);
        free(errstr);
    } else {
        LOG(LWARNING, msg);
    }
}
void info(char* fmt, ...) {
    char msg[LOG_LENGTH] = {0};
    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, LOG_LENGTH, fmt, va);
    va_end(va);
    
    LOG(LINFO, msg);
}

char** str_splitlines(char *str, size_t *line_count) {
    char **result;
    *line_count = 0;
    char *tmp = str;
    
    while(*tmp) {
        if (*tmp == '\n') {
            (*line_count)++;
        }
        tmp++;
    }
    if (*line_count == 0) {
        result = calloc(1, sizeof(char*));
        result[0] = calloc(strlen(str), sizeof(char));
        strcpy(result[0], str);
        return result;
    }
    result = calloc(*line_count, sizeof(char*));
    if (result == NULL) {
        fatal("calloc failed");
    }
    
    size_t i=0, linelen = 0;
    char* saveptr=NULL;
    char *line = strtok_r(str, "\n", &saveptr);
    while(line) {
        linelen = strlen(line);
        result[i] = calloc(linelen+1, sizeof(char));
        if (result[i] == NULL) {
            fatal("calloc failed");
        }
        strcpy(result[i], line);
        if (result[i][linelen-1] == '\r') {
            result[i][linelen-1] = '\0';
            result[i] = realloc(result[i], linelen);
        }
        line = strtok_r(NULL, "\n", &saveptr);
        i++;
    }
    
    return result;
}
/* str_trimwhitespace
 * Credit to https://stackoverflow.com/a/122721/428
 */
char* str_trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

file_map* file_map_new(const char* filename) {
    
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        warning(true, "Failed to open file for memory mapping");
        return NULL;
    }
    size_t size = lseek(fd, 0L, SEEK_END);
    void* map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        warning(true, "Failed to mmap file");
        close(fd);
        return NULL;
    }
    close(fd);
    
    file_map* filemap = calloc(1, sizeof(file_map));
    filemap->map = (char*)map;
    filemap->size = size;
    return filemap;
}
void file_map_delete(file_map* file) {
    if (munmap((void*)file->map, file->size) < 0) {
        warning(true, "failed to unmap file");
    }
    free(file);
}

char* str_replace(char *haystack, const char *search, const char *replacement) {
    
    size_t haystacklen = strlen(haystack);
    size_t searchlen = strlen(search);
    size_t replacementlen = strlen(replacement);
    
    char* result = haystack;
    
    if (searchlen > haystacklen || searchlen == 0) {
        return result;
    }
    if (strstr(replacement, search) != NULL) {
        warning(false, "str_replace: replacement should not contain the search criteria");
    }
    int count = 0;
    while(count++ < 1000) {
        char* pos = strstr(result, search);
        if (pos == NULL) {
            break;
        }
        uint32_t start = (pos - result) / sizeof(char);
        uint32_t end = start + searchlen;
        
        size_t resultlen = strlen(result);
        size_t newlen = resultlen + replacementlen - searchlen;
        
        char* newstr = calloc(newlen+1, sizeof(char));
        strncpy(newstr, result, start);
        strcat(newstr, replacement);
        strcat(newstr, pos+(searchlen*sizeof(char)));
        
        free(result);
        result = newstr;
    }
    return result;
}