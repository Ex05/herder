#ifndef THREAD_POOL_C
#define THREAD_POOL_C

#include "threadPool.h"

#include "que.c"
#include "util.h"

local void* threadPool_threadFunc(void*);

inline ERROR_CODE threadPool_init(ThreadPool* threadPool, const uint_fast16_t numWorkers){
	memset(&threadPool->jobQue, 0, sizeof(threadPool->jobQue));
	
	threadPool->numWorkers = numWorkers;

	threadPool->threads = malloc(sizeof(pthread_t) * numWorkers);
	if(threadPool->threads == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}
 
	if(sem_init(&threadPool->numJobs, 0, 0) != 0){
		UTIL_LOG_ERROR("Failed to initialise 'semaphore'.");

		free(threadPool->threads);

		return ERROR(ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED);
	}

	 if(pthread_mutex_init(&threadPool->lock, NULL) != 0){
		UTIL_LOG_ERROR("Failed to initialise 'pthread_mutex'.");

		free(threadPool->threads);

		return ERROR(ERROR_PTHREAD_MUTEX_INITIALISATION_FAILED);
	}

	 uint_fast16_t i;
	 for(i = 0; i < numWorkers; i++){
		if(pthread_create(&threadPool->threads[i], NULL, threadPool_threadFunc, threadPool) != 0){
			UTIL_LOG_ERROR("Failed to create thread.");

			free(threadPool->threads);

			return ERROR(ERROR_PTHREAD_THREAD_CREATION_FAILED);
		}
	 }

	 if(sem_init(&threadPool->alive, 0, 0) != 0){
		UTIL_LOG_ERROR("Failed to initialise 'semaphore'.");

		return ERROR(ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED);
	 } 

	return ERROR(ERROR_NO_ERROR);
}

inline void** threadPool_free(ThreadPool* threadPool){
	// Set thread pool status to 'not alive'.
	sem_post(&threadPool->alive);

	// Free/unblock worker threads.
	uint_fast16_t i;
	for(i = 0; i < threadPool->numWorkers; i++){
		sem_post(&threadPool->numJobs);
	}

	void** returnValues = malloc(sizeof(*returnValues) * threadPool->numWorkers);
	for(i = 0; i < threadPool->numWorkers; i++){
		void* retVal;
		pthread_join(threadPool->threads[i], &retVal);

		returnValues[i] = retVal;
	}
		
	sem_destroy(&threadPool->numJobs);
	pthread_mutex_destroy(&threadPool->lock);

	Job* job;
	while((job = que_deque(&threadPool->jobQue)) != NULL){
		free(job);
	}

	free(threadPool->threads);

	return returnValues;
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

	void* returnvalue;

	int alive;
	do{
		sem_wait(&threadPool->numJobs);

		if(sem_getvalue(&threadPool->alive, &alive) == -1){
			UTIL_LOG_ERROR_("Invalid semaphore value '%d'.", alive);
		}

		if(alive != 0){
			break;
		}

		pthread_mutex_lock(&threadPool->lock);

		Job* job = que_deque(&threadPool->jobQue);

		pthread_mutex_unlock(&threadPool->lock);
		
		returnvalue = job->runnable(job->data);
		
		free(job);
	}while(alive == 0);
	
	return returnvalue;
}

ERROR_CODE threadPool_signalAll(ThreadPool* threadPool, const int signal){
	ERROR_CODE error = ERROR_NO_ERROR;

	uint_fast64_t i;
	for(i = 0; i < threadPool->numWorkers; i++){
		if(pthread_kill(threadPool->threads[i], signal) != 0){
			error = ERROR_INVALID_SIGNAL;
		}
	}

	return ERROR(error);
}

ERROR_CODE threadPool_signal(ThreadPool* threadPool, const uint_fast64_t workerThread, const int signal){
	if(pthread_kill(threadPool->threads[workerThread], signal) != 0){
		return ERROR(ERROR_INVALID_SIGNAL);
	}

	return ERROR(ERROR_NO_ERROR);
}

#endif