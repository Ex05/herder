#ifndef CACHE_C
#define CACHE_C

#include "cache.h"

 scope_local ERROR_CODE cache_initCacheObject(CacheObject*, uint8_t*, const uint_fast64_t, char*, const uint_fast64_t, char*, const uint_fast64_t);

 scope_local void cache_freeCacheObject(CacheObject*);

inline ERROR_CODE cache_init(Cache* cache, const uint_fast64_t numThreads, const uint_fast64_t size){
	memset(cache, 0, sizeof(*cache));

	if(sem_init(&cache->activeAcesses, 0, numThreads)){
		return ERROR(ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED);
	}

	if(pthread_mutex_init(&cache->lock, NULL)){
		return ERROR(ERROR_PTHREAD_MUTEX_INITIALISATION_FAILED);
	}

	cache->maxSize = size;
	cache->numActiveThreads = numThreads;

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE cache_initCacheObject(CacheObject* cacheObject, uint8_t* data, const uint_fast64_t size, char* fileLocation, const uint_fast64_t fileLocationLength, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){
	cacheObject->data = data;
	cacheObject->size = size;

	// FileLocation.
	cacheObject->fileLocation = malloc(sizeof(*cacheObject->fileLocation) * (fileLocationLength + 1));
	if(cacheObject->fileLocation == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	memcpy(cacheObject->fileLocation, fileLocation, fileLocationLength);
	cacheObject->fileLocation[fileLocationLength] = '\0';

	cacheObject->fileLocationLength = fileLocationLength;

	// Symbolic fileLocation.
	cacheObject->symbolicFileLocation = malloc(sizeof(*cacheObject->symbolicFileLocation) * (symbolicFileLocationLength + 1));
	if(cacheObject->symbolicFileLocation == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	memcpy(cacheObject->symbolicFileLocation, symbolicFileLocation, symbolicFileLocationLength);
	cacheObject->symbolicFileLocation[symbolicFileLocationLength] = '\0';

	cacheObject->symbolicFileLocationLength = symbolicFileLocationLength;

	cacheObject->totalHits = 1;

	// Note: If there is no fileExtension, the offset will be (-1) + 1, and the HTTP_ContentType hash will just be the entire file path. (jan - 2022.09.16)
	cacheObject->fileExtensionOffset = util_findLast(fileLocation, fileLocationLength, '.') + 1;

	cacheObject->httpContentType = http_getContentType(cacheObject->fileLocation + cacheObject->fileExtensionOffset, cacheObject->fileLocationLength - cacheObject->fileExtensionOffset);

	clock_gettime(CLOCK_MONOTONIC, &cacheObject->timeCheckin);
	cacheObject->timeLastHit = cacheObject->timeCheckin;

	return ERROR(ERROR_NO_ERROR);
}

inline void cache_free(Cache* cache){
	LinkedListIterator it;
	linkedList_initIterator(&it, &cache->elements);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		CacheObject* cacheObject = LINKED_LIST_ITERATOR_NEXT_PTR(&it, CacheObject);

		free(cacheObject->data);
		free(cacheObject->fileLocation);
		free(cacheObject->symbolicFileLocation);

		free(cacheObject);
	}

	linkedList_free(&cache->elements);

	sem_destroy(&cache->activeAcesses);

	pthread_mutex_destroy(&cache->lock);
}

inline ERROR_CODE cache_get(Cache* cache, CacheObject** cacheObject, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){
	ERROR_CODE error = ERROR_ENTRY_NOT_FOUND;

	sem_wait(&cache->activeAcesses);

	LinkedListIterator it;
	linkedList_initIterator(&it, &cache->elements);
	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		CacheObject* o = LINKED_LIST_ITERATOR_NEXT_PTR(&it, CacheObject);
		
		if(strncmp(symbolicFileLocation, o->symbolicFileLocation, symbolicFileLocationLength > o->symbolicFileLocationLength ? symbolicFileLocationLength : o->symbolicFileLocationLength) == 0){
			*cacheObject = o;

			// Update cache entry access time.
			clock_gettime(CLOCK_MONOTONIC, &o->timeLastHit);
			o->totalHits++;

			error = ERROR_NO_ERROR;

			goto label_return;
		}
	}

label_return:
	sem_post(&cache->activeAcesses);

	return ERROR(error);
}

inline ERROR_CODE cache_load(Cache* cache, CacheObject** cacheObject, char* fileLocation, const uint_fast64_t fileLocationLength, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){
	struct stat fileInfo;
	
	if(lstat(fileLocation, &fileInfo) == -1){
		return ERROR_(ERROR_FAILED_TO_RETRIEV_FILE_INFO, "File:'%s'", fileLocation);
	}

	if(!S_ISREG(fileInfo.st_mode)){
		return ERROR_(ERROR_FAILED_TO_RETRIEV_FILE_INFO, "File:'%s'", fileLocation);
	}
	
	const uint_fast64_t fileSize = fileInfo.st_size;
	
	FILE* file;
	if((file = fopen(fileLocation, "r")) == NULL){
		return ERROR(ERROR_FAILED_TO_LOAD_FILE);
	}	

	uint8_t* data = malloc(sizeof(*data) * fileSize);
	if(data == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	if(fread(data, sizeof(uint8_t), fileSize, file) != fileSize){
		return ERROR(ERROR_FAILED_TO_LOAD_FILE);
	}

	if(fclose(file) != 0){
		return ERROR(ERROR_FAILED_TO_CLOSE_FILE);
	}

	return cache_add(cache, cacheObject, data, fileSize, fileLocation, fileLocationLength, symbolicFileLocation, symbolicFileLocationLength);
}

ERROR_CODE cache_add(Cache* cache, CacheObject** cacheObject, uint8_t* data, const uint_fast64_t bufferSize, char* fileLocation, const uint_fast64_t fileLocationLength, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){
	ERROR_CODE error;

	*cacheObject = malloc(sizeof(**cacheObject));
	if(*cacheObject == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	if((error = cache_initCacheObject(*cacheObject, data, bufferSize, fileLocation, fileLocationLength, symbolicFileLocation, symbolicFileLocationLength)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	// Lock cache.
	pthread_mutex_lock(&cache->lock);

	uint_fast64_t i;
	for(i = 0; i < cache->numActiveThreads; i++){
		sem_wait(&cache->activeAcesses);
	}

	label_removeStaleCacheObjects:
	if(cache->currentSize + (*cacheObject)->size > cache->maxSize){
		// TODO: Implement better algorithem to delete old cacheObjects.

		uint_fast64_t maxSize = 0;
		CacheObject* staleObject = NULL;

		LinkedListIterator it;
		linkedList_initIterator(&it, &cache->elements);

		while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
			CacheObject* o = LINKED_LIST_ITERATOR_NEXT(&it);

			if(o->size >= maxSize){
				staleObject = o;
			}
		}

		cache->currentSize -= staleObject->size;

		if(cache->currentSize + (*cacheObject)->size > cache->maxSize){
			goto label_removeStaleCacheObjects;
		}
		
		// Unlock cache to remove entry.
		for(; i > 0; i--){
			sem_post(&cache->activeAcesses);
		}

		pthread_mutex_unlock(&cache->lock);

		ERROR_CODE error;
		if((error = cache_remove(cache, staleObject)) != ERROR_NO_ERROR){
			return ERROR(error);
		}

		// Relock cache.
		pthread_mutex_lock(&cache->lock);

		for(i = 0; i < cache->numActiveThreads; i++){
			sem_wait(&cache->activeAcesses);
		}
	}

	linkedList_add(&cache->elements, cacheObject, sizeof(CacheObject*));

	cache->currentSize += (*cacheObject)->size;

	// Unlock cache.
	for(; i > 0; i--){
		sem_post(&cache->activeAcesses);
	}

	pthread_mutex_unlock(&cache->lock);

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE cache_remove(Cache* cache, CacheObject* cacheObject){
	ERROR_CODE error;

	pthread_mutex_lock(&cache->lock);

	uint_fast64_t i;
	for(i = 0; i < cache->numActiveThreads; i++){
		sem_wait(&cache->activeAcesses);
	}

	if((error = linkedList_remove(&cache->elements, &cacheObject, sizeof(CacheObject*))) != ERROR_NO_ERROR){
		return ERROR_(error, "Failed to remove cacheobject '%s' from cache. [%s]", cacheObject->symbolicFileLocation, util_toErrorString(error));
	}

	cache->currentSize += cacheObject->size;

	for(; i > 0; i--){
		sem_post(&cache->activeAcesses);
	}

	pthread_mutex_unlock(&cache->lock);

	cache_freeCacheObject(cacheObject);

	free(cacheObject);

	return ERROR(error);
}

void cache_freeCacheObject(CacheObject* cacheObject){
	free(cacheObject->data);
	free(cacheObject->fileLocation);
	free(cacheObject->symbolicFileLocation);
}

#endif