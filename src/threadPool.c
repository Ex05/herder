#ifndef THREAD_POOL_C
#define THREAD_POOL_C

#include "threadPool.h"

local void* threadPool_threadFunc(void*);

// TODO:(jan) Cleanup error handling in 'threadpool_init' .
inline ERROR_CODE threadPool_init(ThreadPool* threadPool, const uint_fast16_t numWorkers){
    threadPool->numWorkers = numWorkers;

    threadPool->threads = malloc(sizeof(pthread_t) * numWorkers);
    if(threadPool->threads == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    if(threadPool->threads == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    if(sem_init(&threadPool->numJobs, 0, 0)){
        UTIL_LOG_ERROR("Failed to initialise 'semaphore'.");

        free(threadPool->threads);

        return ERROR(ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED);
    }

     if(pthread_mutex_init(&threadPool->lock, NULL)){
        UTIL_LOG_ERROR("Failed to initialise 'pthread_mutex'.");

        free(threadPool->threads);

        return ERROR(ERROR_PTHREAD_MUTEX_INITIALISATION_FAILED);
    }

    ERROR_CODE error;
    if((error = que_init(&threadPool->jobQue)) != ERROR_NO_ERROR){
        return ERROR(error);
    }

     uint_fast16_t i;
     for(i = 0; i < numWorkers; i++){
        if(pthread_create(&threadPool->threads[i], NULL, threadPool_threadFunc, threadPool) != 0){
            UTIL_LOG_ERROR("Failed to create thread.");

            free(threadPool->threads);

            return ERROR(ERROR_PTHREAD_THREAD_CREATION_FAILED);
        }
     }

      if(sem_init(&threadPool->alive, 0, 0)){
        UTIL_LOG_ERROR("Failed to initialise 'semaphore'.");

        return ERROR(ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED);
      }  

    return ERROR(ERROR_NO_ERROR);
}

inline void threadPool_free(ThreadPool* threadPool){
    // Set threadPool status to 'Not alive'.
    sem_post(&threadPool->alive);

    // Free each worker from 'sem_wait(&threadPool->numJobs)'
    uint_fast16_t i;
    for(i = 0; i < threadPool->numWorkers; i++){
        sem_post(&threadPool->numJobs);
    }

    for(i = 0; i < threadPool->numWorkers; i++){
        // TODO:(jan) Collect exit status of each thread and report on it.
        pthread_join(threadPool->threads[i], NULL);
    }
        
    sem_destroy(&threadPool->numJobs);
    pthread_mutex_destroy(&threadPool->lock);

    Job* job;
    while((job = que_deque(&threadPool->jobQue)) != NULL){
        free(job);
    }   

    free(threadPool->threads);
}

ERROR_CODE threadPool_run(ThreadPool* threadPool, Runnable* runnable, void* data){
    pthread_mutex_lock(&threadPool->lock);

    Job* job = malloc(sizeof(*job));
    if(job == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    job->runnable = runnable;
    job->data = data;

    que_enque(&threadPool->jobQue, job);

    pthread_mutex_unlock(&threadPool->lock);
    
    sem_post(&threadPool->numJobs);

    return ERROR(ERROR_NO_ERROR);
}

void* threadPool_threadFunc(void* data){
    ThreadPool* threadPool = (ThreadPool*) data;

    void* httpProcessingBuffer;
    if(util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        return NULL;
    }

    for(;;){
        sem_wait(&threadPool->numJobs);

        int alive;
        if(sem_getvalue(&threadPool->alive, &alive) == -1){
            UTIL_LOG_ERROR_("Invalid semaphore value '%d'.", alive);
        }

        if(alive != 0){
            break;
        }

        pthread_mutex_lock(&threadPool->lock);

        Job* job = que_deque(&threadPool->jobQue);

        pthread_mutex_unlock(&threadPool->lock);
        
        // NOTE: Until we can make sure we are not leaking memory, we just clear everything. (jan - 2019.03.06)
        // TODO: Clear only the actually used memory after we send/received and handled our packets. (jan - 2019.03.06)
        memset(httpProcessingBuffer, 0, HTTP_PROCESSING_BUFFER_SIZE);
        job->buffer = httpProcessingBuffer;

        job->runnable(job);
        
        free(job);
    }
    
    util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);

    return NULL;
}

#endif