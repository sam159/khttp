#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <bits/stdio2.h>

#include "ut/utlist.h"

#include "log.h"

#define XX(n, s, f) |L##n
const log_level LALL = 0 LOG_LEVEL_TABLE(XX);
#undef XX

#define XX(n, s, f) [L##n]=s,
const char *log_level_name[] = {
    LOG_LEVEL_TABLE(XX)
};
#undef XX

log* log_new(const char* name, FILE *output) {
    log* l = calloc(1, sizeof(log));
    l->output = output;
    l->name = calloc(strlen(name)+1, sizeof(char));
    strcpy(l->name, name);
    l->running = false;
    l->output_closeonstop = false;
    return l;
}
void log_delete(log *l) {
    free(l->name);
    free(l);
}
bool log_start(log *l) {
    if (l->running == true) {
        return true;
    }
    int pfd[2];
    if (pipe(pfd) != 0) {
        LOG(LERROR, "Failed to create log pipe");
        return false;
    }
    l->pRead = pfd[0];
    l->pWrite = pfd[1];
    
    
    if (pthread_create(&l->thread, NULL, log_loop, (void*)l) != 0) {
        LOG(LERROR, "Failed to create log thread");
        close(l->pRead);
        close(l->pWrite);
        return false;
    }
    l->running = true;
    
    return true;
    
}
void log_stop(log *l) {
    l->running = false;
    close(l->pWrite);
    if (pthread_equal(l->thread, pthread_self())!=0) {
        pthread_detach(l->thread);
    } else {
        pthread_join(l->thread, NULL);
    }
    close(l->pRead);
    if (l->output_closeonstop == true
        && l->output != stdout
        && l->output != stderr) {
        fclose(l->output);
    }
}
void*log_loop(void* arg) {
    log *l = (log*)arg;
    void** buf = calloc(1, sizeof(void*));
    char* timestr = calloc(32, sizeof(char));
    time_t ctime;
    struct tm tinfo = {0};
    while(true) {
        //Read next message pointer from pipe
        if (read(l->pRead, buf, sizeof(void*)) <= 0) {
            //zero length indicates the write end closed (EOF)
            if (l->running == false) {
                break;
            }
            char errnostr[64];
            strerror_r(errno, errnostr, 64);
            fprintf(stderr, "log[%s] read failed: %s. logger aborted\n", l->name, errnostr);
            log_stop(l);
            break;
        }
        ctime = time(NULL);
        localtime_r(&ctime, &tinfo);
        if (strftime(timestr, 32, "%F %R", &tinfo) == 0) {
            strcpy(timestr, "N/A");
        }
        log_msg* msg = (log_msg*)(*buf);
        fprintf(l->output, "[%s][%s] %s\n", timestr, log_level_name[msg->level], msg->msg);
        fflush(l->output);
        log_msg_delete(msg);
    }
    free(buf);
    free(timestr);
}
void log_write(log *l, log_level level, const char* message) {
    if (l == NULL) {
        fprintf(stderr, "%s\n", message);
        return;
    }
    if (l->running == false) {
        return;
    }
    
    char* msgstr = calloc(strlen(message)+1, sizeof(char));
    strcpy(msgstr, message);
    log_msg *msg = log_msg_new(level, msgstr);
    write(l->pWrite, &msg, sizeof(log_msg*));
}

log_msg* log_msg_new(log_level level, char *msg) {
    log_msg *m = calloc(1, sizeof(log_msg));
    m->level = level;
    m->msg = msg;
    return m;
}
void log_msg_delete(log_msg* msg) {
    free(msg->msg);
    free(msg);
}

static log_register* _register;

void log_register_add(log *l, bool def, log_level levels) {
    if (l->running == false) {
        log_start(l);
    }
    log_register *elem;
    if (def == true) {
        LL_FOREACH(_register, elem) {
            elem->def = false;
        }
    }
    log_register *lr = calloc(1, sizeof(log_register));
    lr->l = l;
    lr->def = def;
    lr->levels = levels;
    LL_APPEND(_register, lr);
}
void log_register_remove(log *l) {
    log_register *elem, *tmp;
    LL_FOREACH_SAFE(_register, elem, tmp) {
        if (elem->l == l) {
            LL_DELETE(_register, elem);
            free(elem);
            break;
        }
    }
}
log* log_register_get(const char* name) {
    log_register *def = NULL, *elem;
    LL_FOREACH(_register, elem) {
        if (name != NULL && strcasecmp(name, elem->l->name) == 0) {
            return elem->l;
        }
        if (elem->def == true) {
            def = elem;
            if (name == NULL) {
                break;
            }
        }
    }
    return (def!=NULL) ? def->l : NULL;
}
void log_register_clear() {
    log_register *elem, *tmp;
    LL_FOREACH_SAFE(_register, elem, tmp) {
        LL_DELETE(_register, elem);
        log_stop(elem->l);
        log_delete(elem->l);
        free(elem);
    }
}
void log_register_write(log_level level, const char* fmt, ...) {
    char msg[LOG_LENGTH];
    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, LOG_LENGTH, fmt, va);
    va_end(va);
    
    int count=0;
    log_register *elem;
    LL_FOREACH(_register, elem) {
        if ((elem->levels & level) == level) {
            log_write(elem->l, level, msg);
            count++;
        }
    }
    //Write to default log if not matched against any registered
    if (count == 0) {
        log_write(log_register_get(NULL), level, msg);
    }
}