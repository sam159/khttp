#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "ut/utlist.h"
#include "thread-pool.h"
#include "util.h"

uint64_t thread_newid(){
    static uint64_t id = 1;
    return __atomic_fetch_add(&id, 1, __ATOMIC_SEQ_CST);
}
thread* thread_new(thread_pool *pool) {
    thread* t = calloc(1, sizeof(thread));
    t->tid = thread_newid();
    t->pool = pool;
    t->stop = false;
    return t;
}
void thread_delete(thread *th) {
    free(th);
}
void thread_start(thread *th, thread_func func) {
    info("Starting thread [%s][%lu]", th->pool->name, th->tid);
    if (pthread_create(&th->pthread, NULL, func, (void*)th)!=0) {
        fatal("Failed to start thread");
    }
}
void thread_stop(thread *th) {
    info("Stopping thread [%s][%lu]", th->pool->name, th->tid);
    if (thread_trystop(th)==false) {
        thread_kill(th);
    }
}
bool thread_trystop(thread *th) {
    void* ret;
    struct timespec waittime;
    waittime.tv_nsec = 0;
    waittime.tv_sec = 2;
    if (pthread_timedjoin_np(th->pthread, &ret, &waittime)!=0) {
        return false;
    }
    return true;
}
void thread_kill(thread* th) {
    pthread_cancel(th->pthread);
    void* ret;
    pthread_join(th->pthread, &ret);
}

thread_pool* thread_pool_new(char *name, queue *queue) {
    thread_pool* pool = calloc(1, sizeof(thread_pool));
    pool->name = calloc(strlen(name)+1, sizeof(char));
    strcpy(pool->name, name);
    pool->queue = queue;
    pool->shutdown = false;
    pool->min_threads=1;
    pool->max_threads=1;
    pool->queue_factor=5;
    return pool;
}
void thread_pool_delete(thread_pool *pool) {
    free(pool->name);
    free(pool->management_thread);
    thread *elem, *tmp;
    LL_FOREACH_SAFE(pool->threads, elem, tmp){
        LL_DELETE(pool->threads, elem);
        thread_delete(elem);
    }
    free(pool->threads);
    free(pool);
}
void thread_pool_start(thread_pool *pool) {
    info("Starting thread pool %s", pool->name);
    pool->management_thread = thread_new(pool);
    thread_start(pool->management_thread, thread_pool_loop);
}
void thread_pool_stop(thread_pool *pool) {
    info("Stopping thread pool %s", pool->name);
    pool->shutdown = true;
    void* ret;
    if (pthread_join(pool->management_thread->pthread, &ret) != 0) {
        fatal("Could not join thread pool manager");
    }
}
void thread_pool_add_thread(thread_pool *pool, thread *th) {
    thread_start(th, pool->func);
    LL_PREPEND(pool->threads, th);
    pool->thread_count++;
}
void thread_pool_remove_thread(thread_pool *pool, thread *th) {
    LL_DELETE(pool->threads, th);
    pool->thread_count--;
}

void* thread_pool_loop(void* arg) {
    thread* th = (thread*)arg;
    thread_pool *pool = th->pool;
    
    if (pool->min_threads > 0) {
        info("Starting initial threads for %s", pool->name);
        //Start initial threads
        for(int i=0; i<pool->min_threads;i++) {
            thread *th = thread_new(pool);
            thread_pool_add_thread(pool, th);
        }
    }
    if (pool->min_threads > pool->max_threads) {
        warning(false, "thread pool %s: min threads exceeds max threads", pool->name);
        pool->max_threads = pool->min_threads;
    }
    
    //Management loop
    struct timespec loopdelay;
    loopdelay.tv_nsec = 250*1000;//250ms
    loopdelay.tv_sec = 0;
    
    int64_t last_queue_count = 0;
    while(pool->shutdown == false) {
        size_t queue_size = queue_count(pool->queue);
        int64_t queue_count_diff = queue_size - last_queue_count;
        int64_t watermark = pool->thread_count * pool->queue_factor;
        if (queue_count_diff > watermark && pool->thread_count < pool->max_threads) {
            //New thread
            thread *th = thread_new(pool);
            thread_pool_add_thread(pool, th);
            last_queue_count = queue_size;
        } else if ((queue_count_diff*-1) > watermark && pool->thread_count > pool->min_threads) {
            //Remove thread (stop the first in the list)
            pool->threads->stop = true;
            last_queue_count = queue_size;
        }
        
        //cleanup finished threads
        void* ret;
        thread *elem,*tmp;
        LL_FOREACH_SAFE(pool->threads, elem, tmp) {
            int joinresult = pthread_tryjoin_np(elem->pthread, &ret);
            if (joinresult != 0) {
                if (joinresult == EBUSY) {
                    continue;
                }
                errno = joinresult;
                fatal("thread try join failed");
            }
            info("Close thread [%s][%lu]", pool->name, elem->tid);
            thread_pool_remove_thread(pool, elem);
            thread_delete(elem);
        }
        
        nanosleep(&loopdelay, NULL);
    }
    //Wait until queue is empty
    while(queue_count(pool->queue) > 0) {
        nanosleep(&loopdelay, NULL);
    }
    
    //Shutdown threads
    thread *elem, *tmp;
    LL_FOREACH(pool->threads, elem) {
        elem->stop = true;
    }
    
    //Ping the queue to wake up the threads (this should prompt them to check the stop flag)
    queue_ping(pool->queue);
    
    //Remove threads
    LL_FOREACH_SAFE(pool->threads, elem, tmp) {
        thread_stop(elem);
        thread_pool_remove_thread(pool, elem);
        thread_delete(elem);
    }
}