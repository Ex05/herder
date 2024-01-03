#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "util.h"

#include <pthread.h>
#include <semaphore.h>

#include "que.h"

#define THREAD_POOL_RUNNABLE_RETURN(data) \
void* returnData = malloc(sizeof(data)); \
memcpy(returnData, &data, sizeof(data)); \
ERROR_CODE unused ## data = ERROR(data); \
unused ## data = unused ## data; \
return returnData;


#define THREAD_POOL_RUNNABLE_RETURN_(type, data) \
type* returnData = malloc(sizeof(data)); \
*returnData = data; \
ERROR_CODE unused ## data = ERROR(data); \
unused ## data = unused ## data; \
return returnData;

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
	void* data;
	Runnable* runnable;
}Job;

ERROR_CODE threadPool_init(ThreadPool*, const uint_fast16_t);

void threadPool_free(ThreadPool*);

ERROR_CODE threadPool_run(ThreadPool*, Runnable*, void*);

ERROR_CODE threadPool_signalAll(ThreadPool*, const int);

ERROR_CODE threadPool_signal(ThreadPool*, const uint_fast64_t, const int);

#endif