/* 
 * File:   log.h
 * Author: sam
 *
 * Created on 06 August 2014, 12:06
 */

#ifndef LOG_H
#define	LOG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#define LOG_LEVEL_TABLE(XX)         \
    XX(DEBUG,   "DEBUG",    1 << 0) \
    XX(INFO,    "INFO",     1 << 1) \
    XX(WARNING, "WARNING",  1 << 2) \
    XX(ERROR,   "ERROR",    1 << 3) \
    XX(FATAL,   "FATAL",    1 << 4)
    
#define XX(n, s, f) L##n=f,
    typedef enum log_level {
        LOG_LEVEL_TABLE(XX)
    } log_level;
#undef XX
    
#define LOG(level, fmt, ...) log_register_write(level, fmt, ##__VA_ARGS__)
    
#define LOG_LENGTH 256
    
    extern const log_level LALL;
    
    extern const char *log_level_name[];

    typedef struct log {
        char* name;
        int pRead, pWrite;
        FILE *output;
        bool output_closeonstop;
        pthread_t thread;
        bool running;
    } log;
    
    typedef struct log_msg {
        log_level level;
        char* msg;
    } log_msg;
    
    typedef struct log_register {
        log *l;
        bool def;
        log_level levels;
        struct log_register *next;
    } log_register;
    
    log* log_new(const char* name, FILE *output);
    void log_delete(log *l);
    bool log_start(log *l);
    void log_stop(log *l);
    void*log_loop(void* arg);
    void log_write(log *l, log_level level, const char* message);
    
    log_msg* log_msg_new(log_level level, char *msg);
    void log_msg_delete(log_msg* msg);
    
    void log_register_add(log *l, bool def, log_level levels);
    void log_register_remove(log *l);
    log* log_register_get(const char* name);
    void log_register_clear();
    void log_register_write(log_level level, const char* fmt, ...);
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* LOG_H */

