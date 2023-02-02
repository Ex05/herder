#ifndef CACHE_TEST_C
#define CACHE_TEST_C

#include "../test.c"

TEST_TEST_SUIT_CONSTRUCT_FUNCTION(cache, Cache, cache){
	*cache = malloc(sizeof(**cache));

	ERROR_CODE error;
	if((error = cache_init(*cache, 1, MB(1))) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(cache, Cache, cache){
	cache_free(cache);

	free(cache);

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_FUNCTION_(cache_add, Cache, cache){
	char* data = malloc(sizeof(*data) * 7);
	strncpy(data, "123abc", 7);

	ERROR_CODE error;

	CacheObject* cacheObject;
	if((error = cache_add(cache, &cacheObject, (uint8_t*) data, 7, NULL, 0, "/data", 5)) != ERROR_NO_ERROR){
		return TEST_FAILURE("ERROR: Failed to add data['%s'] to cache. (%s)", data, util_toErrorString(error));
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &cache->elements);

	CacheObject* _cacheObject = LINKED_LIST_ITERATOR_NEXT_PTR(&it, CacheObject);
	if(memcmp(cacheObject->data, _cacheObject->data, cacheObject->size) != 0){
		return TEST_FAILURE("ERROR: Failed to add cach object to cache '%s'!= '%s'.", cacheObject->data, _cacheObject->data);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(cache_load, Cache, cache){
 ERROR_CODE error;

	#define TEST_FILE_NAME "/tmp/herder_cache_test_file_XXXXXX"

	char* filePath = malloc(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
	strcpy(filePath, TEST_FILE_NAME);

	#undef TEST_FILE_NAME

	int tempFileDescriptor = mkstemp(filePath);
	if(tempFileDescriptor < 1){
		return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
	}

	uint8_t* buffer = malloc(sizeof(*buffer) * 256);
	memset(buffer, 8, 64);
	memset(buffer + 64, 16, 64);
	memset(buffer + 128, 8, 64);
	memset(buffer + 192, 32, 64);

	if(write(tempFileDescriptor, buffer, 256) != 256){
		return TEST_FAILURE("Failed to write media library file version. Expected to write %d bytes.", 256);
	}

	close(tempFileDescriptor);

	free(buffer);
	
	char symbolicFileLocation[] = "/herderTestFile";

	CacheObject* cacheObject;
	if((error = cache_load(cache, &cacheObject, filePath, strlen(filePath), symbolicFileLocation, strlen(symbolicFileLocation))) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to load cache object. '%s'", util_toErrorString(error));
	}

	if(cacheObject->size != 256){
		return TEST_FAILURE("Cache object size %" PRIdFAST64 " != %d", cacheObject->size, 256);
	}

	uint_fast16_t i;
	for(i = 0; i < 256; i++){
		if(i < 64){
			if(cacheObject->data[i] != 8){
				return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 8);
			}
		}else if(i < 128){
			if(cacheObject->data[i] != 16){
				return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 16);
			}
		}else if(i < 192){
			if(cacheObject->data[i] != 8){
				return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 8);
			}
		}else if(i < 256){
			if(cacheObject->data[i] != 32){
				return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 32);
			}
		}
	}

	if((error = util_deleteFile(filePath)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete test file: '%s'. '%s'.", filePath, util_toErrorString(error));
	}

	free(filePath);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(cache_get, Cache, cache){
	char* data = malloc(sizeof(*data) * 7);
	strncpy(data, "123abc", 7);

	ERROR_CODE error;

	CacheObject* cacheObject;
	if((error = cache_add(cache, &cacheObject, (uint8_t*) data, 7, NULL, 0, "/data", 5)) != ERROR_NO_ERROR){
		return TEST_FAILURE("ERROR: Failed to add data['%s'] to cache. (%s)", data, util_toErrorString(error));
	}

	if((error = cache_get(cache, &cacheObject, "/data", 5)) != ERROR_NO_ERROR){
		return TEST_FAILURE("ERROR: Failed to retrieve data['%s'] from cache. (%s)", data, util_toErrorString(error));
	}

	if(memcmp(cacheObject->data, cacheObject->data, cacheObject->size) != 0){
		return TEST_FAILURE("ERROR: Failed to retrieve cach object from cache '%s'!= '%s'.", cacheObject->data, 
		cacheObject->data);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(cache_remove, Cache, cache){
	char* data_a = malloc(sizeof(*data_a) * 7);
	strncpy(data_a, "123abc", 7);

	char* data_b = malloc(sizeof(*data_b) * 7);
	strncpy(data_b, "xyz098", 7);

	ERROR_CODE error;

	CacheObject* cacheObject_a;
	if((error = cache_add(cache, &cacheObject_a, (uint8_t*) data_a, 7, NULL, 0, "/data_a", 7)) != ERROR_NO_ERROR){
		return TEST_FAILURE("ERROR: Failed to add data['%s'] to cache. (%s)", data_a, util_toErrorString(error));
	}

	CacheObject* cacheObject_b;
	if((error = cache_add(cache, &cacheObject_b, (uint8_t*) data_b, 7, NULL, 0, "/data_b", 7)) != ERROR_NO_ERROR){
		return TEST_FAILURE("ERROR: Failed to add data['%s'] to cache. (%s)", data_a, util_toErrorString(error));
	}

	if((error = cache_remove(cache, cacheObject_a)) != ERROR_NO_ERROR){
		return TEST_FAILURE("ERROR: Failed to remove cache object '%s' from cache. (%s)", cacheObject_a->symbolicFileLocation, util_toErrorString(error));
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &cache->elements);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		CacheObject* _cacheObject = LINKED_LIST_ITERATOR_NEXT_PTR(&it, CacheObject);

		if(memcmp("123abc", _cacheObject->data, _cacheObject->size) == 0){
			return TEST_FAILURE("ERROR: Failed to add cach object to cache '%s'!= '%s'.", "123abc", _cacheObject->data);
		}
	}

	return TEST_SUCCESS;
}

#endif