#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <magic.h>
#include <string.h>

#include "ut/utlist.h"
#include "mime.h"
#include "main.h"

mime_type *mime_list = NULL;

int mime_init(const char* file) {
    mime_destroy();
    return mime_load(&mime_list, NULL);
}
void mime_destroy() {
    if (mime_list != NULL) {
        mime_free(mime_list);
    }
    mime_list = NULL;
}

int mime_load(mime_type **list, const char* file) {
    FILE *mimetypes = fopen(file == NULL ? MIME_DEFAULT_FILE : file, "r");
    if (mimetypes == NULL) {
        return -1;
    }
    mime_type *new_list = NULL;
    size_t count = 1024;
    char* buffer = calloc(count, sizeof(char));
    ssize_t linelength;
    while((linelength = getline(&buffer, &count, mimetypes)) >= 0) {
        
        if (linelength == 0) {
            continue;
        }
        if (buffer[0] == '#' || buffer[0] == '\n') {
            continue;
        }
        char* line = strndup(buffer, linelength);
        char mime[512], extstr[512];
        if (sscanf(line, "%511s%*[ \t]%511[a-z0-9 \t]", mime, extstr) == 2) {
            char* saveptr=NULL;
            char* ext = strtok_r(extstr, " \t", &saveptr);
            while(ext != NULL) {
                mime_type *new = calloc(1, sizeof(mime_type));
                new->extension = strdup(ext);
                new->mime = strdup(mime);
                LL_APPEND(new_list, new);
                ext = strtok_r(NULL, " \t", &saveptr);
            }
        }
        free(line);
    }
    free(buffer);
    fclose(mimetypes);
    
    *list = new_list;
    
    count = 0;
    mime_type *elem;
    LL_COUNT(new_list, elem, count);
    return count;
}
void mime_free(mime_type *list) {
    if (list == NULL) {
        list = mime_list;
    }
    mime_type *elem, *tmp;
    LL_FOREACH_SAFE(list, elem, tmp) {
        free(elem->extension);
        free(elem->mime);
        free(elem);
    }
}
void mime_print_all(mime_type *list) {
    mime_type *elem;
    LL_FOREACH(list, elem) {
        printf("%s\t\t%s\n", elem->mime, elem->extension);
    }
}

const char* mime_get_type(const char* filename, const char* fallback) {
    return mime_list_get_type(mime_list, filename, fallback);
}
const char* mime_list_get_type(mime_type *list, const char* filename, const char* fallback) {
    char *ext = strrchr(filename, '.');
    if (ext != NULL && strlen(ext) > 1) {
        ext++; //Skip .
        mime_type *elem;
        LL_FOREACH(list, elem) {
            if (strcmp(ext, elem->extension) == 0) {
                return elem->mime;
            }
        }
    }
    return mime_get_type_magic(filename, fallback);
}
const char* mime_get_type_magic(const char* filename, const char* fallback) {
    magic_t magic;
    
    magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, NULL);
    magic_compile(magic, NULL);
    
    const char* mime = magic_file(magic, filename);
    magic_close(magic);
    
    if (mime != NULL) {
        return mime;
    }
    
    return fallback;
}
