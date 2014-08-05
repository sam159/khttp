/* 
 * File:   thread-pool.h
 * Author: sam
 *
 * Created on 05 August 2014, 13:32
 */

#ifndef THREAD_POOL_H
#define	THREAD_POOL_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <pthread.h>
#include <stdbool.h>
#include "queue.h"

    typedef struct thread {
        uint64_t tid;
        pthread_t pthread;
        struct thread_pool *pool;
        bool stop;
        struct thread* next;
    } thread;
    
    typedef void* (*thread_func)(void*);
    
    typedef struct thread_pool {
        char* name;
        bool shutdown;
        thread_func func;
        thread *threads;
        size_t thread_count;
        size_t min_threads;
        size_t max_threads;
        size_t queue_factor;
        thread *management_thread;
        queue *queue;
    } thread_pool;

    uint64_t thread_newid();
    thread* thread_new(thread_pool *pool);
    void thread_delete(thread *th);
    void thread_start(thread *th, thread_func func);
    void thread_stop(thread *th);
    bool thread_trystop(thread *th);
    void thread_kill(thread *th);
    
#define THREAD_POOL_FOREACH_THREAD(pool, th, i)             \
    i=0;                                                    \
    th = pool->thread_count > 0 ? pool->threads[i] : NULL;  \
    for(;i<pool->thread_count;th=pool->threads[i++])
    
    thread_pool* thread_pool_new(char *name, queue *queue);
    void thread_pool_delete(thread_pool *pool);
    void thread_pool_start(thread_pool *pool);
    void thread_pool_stop(thread_pool *pool);
    void thread_pool_add_thread(thread_pool *pool, thread *th);
    void thread_pool_remove_thread(thread_pool *pool, thread *th);
    
    void* thread_mgt(void* th);
    
#ifdef	__cplusplus
}
#endif

#endif	/* THREAD_POOL_H */

