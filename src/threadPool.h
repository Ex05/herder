#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <semaphore.h>

#include "util.h"

#include "que.h"

typedef struct{
    pthread_t* threads;
    uint_fast16_t numWorkers;
    pthread_mutex_t lock;
    sem_t numJobs;
    sem_t alive;
    Que jobQue;
}ThreadPool;

#define THREAD_POOL_RUNNABLE(functionName) void* functionName(void* data)
#define THREAD_POOL_RUNNABLE_(functionName, dataType, varName) void* functionName(dataType* varName)
typedef THREAD_POOL_RUNNABLE(Runnable);

typedef struct{
    void* buffer;
    void* data;
    Runnable* runnable;
}Job;

ERROR_CODE threadPool_init(ThreadPool*, const uint_fast16_t);

void threadPool_free(ThreadPool*);

ERROR_CODE threadPool_run(ThreadPool*, Runnable*, void*);

#endif