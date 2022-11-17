#ifndef CACHE_H
#define CACHE_H

#include "util.h"

#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "linkedList.h"

#include "http.h"

typedef struct{
	LinkedList elements;
	uint_fast64_t maxSize;
	uint_fast64_t currentSize;
	uint_fast64_t numActiveThreads;
	sem_t activeAcesses;
	pthread_mutex_t lock;
}Cache;

typedef struct{
	uint8_t* data;
	uint_fast64_t size;
	struct timespec timeCheckin;
	struct timespec timeLastHit;
	uint_fast64_t totalHits;
	uint_fast64_t fileLocationLength;
	uint_fast64_t symbolicFileLocationLength;
	uint_fast64_t fileExtensionOffset;
	HTTP_ContentType httpContentType;
	char* fileLocation;
	char* symbolicFileLocation;
}CacheObject;

ERROR_CODE cache_init(Cache*, const uint_fast64_t, const uint_fast64_t);

void cache_free(Cache*);

ERROR_CODE cache_get(Cache*, CacheObject**, char*, const uint_fast64_t);

ERROR_CODE cache_load(Cache*, CacheObject**, char*, const uint_fast64_t, char*, const uint_fast64_t);

ERROR_CODE cache_add(Cache*, CacheObject**, uint8_t*, const uint_fast64_t, char*, const uint_fast64_t, char*, const uint_fast64_t);

ERROR_CODE cache_remove(Cache*, CacheObject*);

#endif