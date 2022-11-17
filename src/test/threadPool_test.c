#ifndef THREAD_POOL_TEST_C
#define THREAD_POOL_TEST_C

#include "../test.c"

local uint_fast64_t counter = 0;

local pthread_mutex_t counterLock;

local sem_t threadSync;

THREAD_POOL_RUNNABLE(test_threadPoolRunner){
	pthread_mutex_lock(&counterLock);

	uint_fast64_t i;
	for(i = 0; i < 10; i++){
		counter++;
	}

	pthread_mutex_unlock(&counterLock);

	sem_post(&threadSync);

	return NULL;
}

TEST_TEST_FUNCTION(threadPool_run){
	ThreadPool threadPool;
	threadPool_init(&threadPool, 4);

	ERROR_CODE error;
	if((error = pthread_mutex_init(&counterLock, NULL)) != 0){
		return TEST_FAILURE("Failed to initialise 'pthread_mutex'. '%d'", error);
	}

	if((error = sem_init(&threadSync, 0, 0)) != 0){
		return TEST_FAILURE("Failed to initialise 'semaphore'. '%d'", error);
	}

	threadPool_run(&threadPool, test_threadPoolRunner, NULL);
	threadPool_run(&threadPool, test_threadPoolRunner, NULL);
	threadPool_run(&threadPool, test_threadPoolRunner, NULL);
	threadPool_run(&threadPool, test_threadPoolRunner, NULL);

	uint_fast64_t i;
	for(i = 4; i > 0; i--){
		sem_wait(&threadSync);
	}

	if(counter != 40){
		return TEST_FAILURE("Counter value '%" PRIdFAST64 "' != '%d'.", counter, 40);
	}

	pthread_mutex_destroy(&counterLock);

	sem_destroy(&threadSync);

	void** returnValues = threadPool_free(&threadPool);

	free(returnValues);

	return TEST_SUCCESS;
}

#endif