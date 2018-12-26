#ifndef CACHE_C
#define CACHE_C

#include "cache.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

local ERROR_CODE cache_initCacheObject(CacheObject*, uint8_t*, const uint_fast64_t, char*, const uint_fast64_t, char*, const uint_fast64_t);

ARRAY_LIST_EXPAND_FUNCTION(cache_expandCacheElements){
    return previousSize * 2;
}

inline ERROR_CODE cache_init(Cache* cache, const uint_fast64_t numThreads, const uint_fast64_t size){
    memset(cache, 0, sizeof(*cache));

    if(sem_init(&cache->activeAcesses, 0, numThreads)){        
        return ERROR(ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED);
    }

    if(pthread_mutex_init(&cache->lock, NULL)){     
        return ERROR(ERROR_PTHREAD_MUTEX_INITIALISATION_FAILED);
    }

    cache->maxSize = size;
    cache->threads = numThreads;

    ERROR_CODE error;
    if((error = arrayList_init(&cache->elements, 64, cache_expandCacheElements)) != ERROR_NO_ERROR){
        goto label_return;
    }

label_return:
    return ERROR(error);
}

inline ERROR_CODE cache_initCacheObject(CacheObject* cacheObject, uint8_t* data, const uint_fast64_t size, char* fileLocation, const uint_fast64_t fileLocationLength, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){
    cacheObject->data = data;
    cacheObject->size = size;
    cacheObject->fileLocation = fileLocation;
    cacheObject->fileLocationLength = fileLocationLength;

    cacheObject->symbolicFileLocation = malloc(sizeof(*cacheObject->symbolicFileLocation) * (symbolicFileLocationLength + 1));
    memcpy(cacheObject->symbolicFileLocation, symbolicFileLocation, symbolicFileLocationLength);
    cacheObject->symbolicFileLocation[symbolicFileLocationLength] = '\0';

    if(cacheObject->symbolicFileLocation == NULL){
        return  ERROR(ERROR_OUT_OF_MEMORY);
    }

    cacheObject->symbolicFileLocationLength = symbolicFileLocationLength;

    clock_gettime(CLOCK_MONOTONIC, &cacheObject->timeCheckin);
    cacheObject->timeLastHit = cacheObject->timeCheckin;

    cacheObject->totalHits = 1;

    // Note:(jan) If there is no fileExtension, the offset will be 0, and the HTTP_ContentType hash will just be the entire file path.
    cacheObject->fileExtensionOffset = util_findLast(fileLocation, fileLocationLength, '.') + 1;

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE cache_free(Cache* cache){
    ArrayListIterator it;
    arrayList_initIterator(&it, &cache->elements);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        CacheObject* cacheObject =  ARRAY_LIST_ITERATOR_NEXT(&it);

        free(cacheObject->data);
        free(cacheObject->fileLocation);
        free(cacheObject->symbolicFileLocation);

        free(cacheObject);
    }

    arrayList_free(&cache->elements);

    sem_destroy(&cache->activeAcesses);

    pthread_mutex_destroy(&cache->lock);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE cache_get(Cache* cache, CacheObject** cacheObject, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){    
    sem_wait(&cache->activeAcesses);

    ArrayListIterator it;
    arrayList_initIterator(&it, &cache->elements);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        CacheObject* o =  ARRAY_LIST_ITERATOR_NEXT(&it);

        if(strncmp(symbolicFileLocation, o->symbolicFileLocation, symbolicFileLocationLength > o->symbolicFileLocationLength ? symbolicFileLocationLength : o->symbolicFileLocationLength) == 0){
            *cacheObject = o;

            goto label_return;
        }
    }

label_return:
    sem_post(&cache->activeAcesses);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE cache_load(Cache* cache, CacheObject** cacheObject, char* fileLocation, const uint_fast64_t fileLocationLength, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){
	struct stat fileInfo;
    
    if(lstat(fileLocation, &fileInfo) == -1){
        return ERROR_(ERROR_FAILED_TO_RETRIEV_FILE_INFO, "File:'%s'", fileLocation);
    }

    if(!S_ISREG(fileInfo.st_mode)){
        return ERROR(ERROR_FAILED_TO_RETRIEV_FILE_INFO);
    }
    
    const uint_fast64_t fileSize = fileInfo.st_size;
    
    FILE* file;
    if((file = fopen(fileLocation, "r")) == NULL){
        return ERROR(ERROR_FAILED_TO_LOAD_FILE);
    }    

    uint8_t* data = malloc(sizeof(*data) * fileSize);

    if(fread(data, sizeof(uint8_t), fileSize, file) != fileSize){
        return ERROR(ERROR_FAILED_TO_LOAD_FILE);
    }

    *cacheObject = malloc(sizeof(**cacheObject));
    if(*cacheObject == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    ERROR_CODE error;
    if((error = cache_initCacheObject(*cacheObject, data, fileSize, fileLocation, fileLocationLength, symbolicFileLocation, symbolicFileLocationLength)) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if(fclose(file) != 0){
        return ERROR(ERROR_FAILED_TO_CLOSE_FILE);
    }

    if(cache->currentSize + (*cacheObject)->size > cache->maxSize){
        UTIL_LOG_CRITICAL("// TODO:(jan) Remove stuff from cache.");
    }

    pthread_mutex_lock(&cache->lock);

    uint_fast64_t i;
    for(i = 0; i < cache->threads; i++){
        sem_wait(&cache->activeAcesses);
    }

    arrayList_add(&cache->elements, *cacheObject);

    cache->currentSize += (*cacheObject)->size;
    
    for(; i > 0; i--){
        sem_post(&cache->activeAcesses);
    }
    
    pthread_mutex_unlock(&cache->lock);

    return ERROR(ERROR_NO_ERROR);
}

#endif