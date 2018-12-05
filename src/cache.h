#ifndef CACHE_H
#define CACHE_H

#include "util.h"

#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "arrayList.h"

typedef struct{
    ArrayList elements;
    uint_fast64_t maxSize;
    uint_fast64_t currentSize;
    sem_t activeAcesses;
    pthread_mutex_t lock;
    uint_fast64_t threads;
}Cache;

typedef struct{
    uint8_t* data;
    uint_fast64_t size;
    uint_fast64_t fileLocationLength;
    uint_fast64_t symbolicFileLocationLength;
    uint_fast64_t fileExtensionOffset;
    uint_fast64_t totalHits;
    char* fileLocation;
    char* symbolicFileLocation;
    struct timespec timeCheckin;
    struct timespec timeLastHit;
}CacheObject;

ERROR_CODE cache_init(Cache*, const uint_fast64_t, const uint_fast64_t);

ERROR_CODE cache_free(Cache*);

ERROR_CODE cache_get(Cache*, CacheObject**, char*, const uint_fast64_t);

ERROR_CODE cache_load(Cache*, CacheObject**, char*, const uint_fast64_t, char*, const uint_fast64_t);

#endif