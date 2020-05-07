#ifndef CACHE_H
#define CACHE_H

#include "util.h"

#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "linkedList.h"

typedef struct{
    LinkedList elements;
    uint_fast64_t maxSize;
    uint_fast64_t currentSize;
    sem_t activeAcesses;
    pthread_mutex_t lock;
    uint_fast64_t threads;
}Cache;

typedef struct{
    uint8_t* data;
    uint_fast64_t size;
    struct timespec timeCheckin;
    struct timespec timeLastHit;
    uint32_t totalHits;
    uint32_t hitsSizeLastCheck;
    uint_fast64_t fileLocationLength;
    uint_fast64_t symbolicFileLocationLength;
    uint_fast64_t fileExtensionOffset;
    char* fileLocation;
    char* symbolicFileLocation;
}CacheObject;

ERROR_CODE cache_init(Cache*, const uint_fast64_t, const uint_fast64_t);

void cache_free(Cache*);

ERROR_CODE cache_get(Cache*, CacheObject**, char*, const uint_fast64_t);

ERROR_CODE cache_load(Cache*, CacheObject**, char*, const uint_fast64_t, char*, const uint_fast64_t);

#endif