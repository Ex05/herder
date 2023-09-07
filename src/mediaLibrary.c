#ifndef MEDIA_LIBRARY_C
#define MEDIA_LIBRARY_C

#include "mediaLibrary.h"

ERROR_CODE mediaLibrary_init(MediaLibrary* library, PropertyFile* properties){
	ERROR_CODE error;

	memset(library, 0, sizeof(*library));

	// Library_location.
	PROPERTIES_GET(properties, library->location, MEDIA_LIBRARY_LOCATION);
	// Library_file_path.
	PROPERTIES_GET(properties, library->libraryFilePath, MEDIA_LIBRARY_LIBRARY_FILE_PATH);

	// Allocate memory bucket for parsing operation of library file.
	uint_fast64_t libraryFileSize;
	if((error = util_getFileSize(library->libraryFilePath->value, &libraryFileSize)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	MemoryBucket bucket;
	if((error = util_blockAlloc(&bucket, libraryFileSize)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	LinkedList libraries = {0};	
	if((error = mediaLibrary_parseLibraryFile(library, &bucket, libraryFileSize, &libraries)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	// Load libraries;
	LinkedListIterator it;
	linkedList_initIterator(&it, &libraries);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Library* library = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Library);

		UTIL_LOG_CONSOLE_(LOG_DEBUG, "Library: Type:[%d] Name:'%s'", library->type, library->name);

		// TODO: Load each libraries media/library file.
	}

	if((error = util_unMap(bucket, libraryFileSize)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return ERROR(error);
}

inline ERROR_CODE mediaLibrary_parseLibraryFile(MediaLibrary* library, MemoryBucket* bucket, uint_fast64_t libraryFileSize, LinkedList* libraries){
	ERROR_CODE error;

	uint8_t* buffer = *bucket;

	if((error = util_loadFile(library->libraryFilePath->value, libraryFileSize, &buffer)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return mediaLibrary_parseLibraryFileContent_(libraries, buffer, libraryFileSize);
}

inline ERROR_CODE __INTERNAL_USE__ mediaLibrary_parseLibraryFileContent_(LinkedList* libraries, uint8_t* buffer, uint_fast64_t bufferSize){
	// Iterate over library file entries.
	for(uint_fast64_t readOffset = 0; readOffset < bufferSize;){
		Library* library = malloc(sizeof(*library));

		library->type = buffer[readOffset];
		readOffset += 1;

		library->nameLength = buffer[readOffset];
		// Note: We store the string length including trailing '\0' on disk, so we have to subtract it here again. (jan - 2023.08.30) 
		library->nameLength -= 1;
		readOffset += 1;
	
		library->name = (char*) buffer + readOffset;

		readOffset += (library->nameLength + 1);

		LINKED_LIST_ADD_PTR(libraries, (&library));
	}

	return ERROR(ERROR_NO_ERROR);
}

void mediaLibrary_freeShowLibrary(ShowLibrary* library){
	free(library->shows);
	free(library->seasons);
	free(library->episodes);
	free(library->showNames);
	free(library->episodeNames);
}

MediaLibrary_freeFunction* mediaLibrary_getLibraryFreeFunction(Library* library){
	switch (library->type){
		case LIBRARY_TYPE_SHOW:{
			return (MediaLibrary_freeFunction*) mediaLibrary_freeShowLibrary;
		}
		case LIBRARY_TYPE_MOVIE:{
			break;
		}
		case LIBRARY_TYPE_AUDIO_ONLY_SHOW:{
			break;
		}
		case LIBRARY_TYPE_PODCAST:{
			break;
		}
		case LIBRARY_TYPE_AUDIO_BOOK:{
			break;
		}
		case LIBRARY_TYPE_BOOK:{
			break;
		}
		case LIBRARY_TYPE_PICTURE:{
			break;
		}
		case LIBRARY_TYPE_VIDEO:{
			break;
		}
		case LIBRARY_TYPE_DOCUMENT:{
			break;
		}
		default:{
			break;
		}
	}

	return NULL;
}

ssize_t mediaLibrary_sizeofLibrary(Library* library){
	switch (library->type){
		case LIBRARY_TYPE_SHOW:{
			return sizeof(ShowLibrary);
		}
		case LIBRARY_TYPE_MOVIE:{
			break;
		}
		case LIBRARY_TYPE_AUDIO_ONLY_SHOW:{
			break;
		}
		case LIBRARY_TYPE_PODCAST:{
			break;
		}
		case LIBRARY_TYPE_AUDIO_BOOK:{
			break;
		}
		case LIBRARY_TYPE_BOOK:{
			break;
		}
		case LIBRARY_TYPE_PICTURE:{
			break;
		}
		case LIBRARY_TYPE_VIDEO:{
			break;
		}
		case LIBRARY_TYPE_DOCUMENT:{
			break;
		}
		default:{
			break;
		}
	}

	return sizeof(Library);
}

inline void mediaLibrary_freeEpisodeInfo(EpisodeInfo* episodeInfo){
	free(episodeInfo->showName);
	free(episodeInfo->episodeName);
	free(episodeInfo->path);
}

inline void mediaLibrary_free(MediaLibrary* library){
	register int_fast8_t i;
	register intptr_t readOffsert = 0;
	for(i = 0; i < library->numLibraries; i++){
		Library* _library = *(library->libraries + readOffsert);

		MediaLibrary_freeFunction* libraryFreeFunction = mediaLibrary_getLibraryFreeFunction(_library);

		libraryFreeFunction(_library);

		readOffsert += mediaLibrary_sizeofLibrary(_library);
	}
}

#endif