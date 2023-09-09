#ifndef MEDIA_LIBRARY_C
#define MEDIA_LIBRARY_C

#include "mediaLibrary.h"

ERROR_CODE mediaLibrary_init(MediaLibrary* library, PropertyFile* properties){
	ERROR_CODE error;

	memset(library, 0, sizeof(*library));

	// Library_location.
	PROPERTIES_GET(properties, library->location, MEDIA_LIBRARY_LOCATION);
	// Library_file_name.
	PROPERTIES_GET(properties, library->libraryFileName, MEDIA_LIBRARY_LIBRARY_FILE_NAME);

	const bool isLibraryFilePathPathDeleimiterTerminated = library->location->value[library->location->valueLength - 1] == CONSTANTS_FILE_PATH_DIRECTORY_DELIMITER;

	const uint_fast64_t libraryFilePathLength = library->location->valueLength + library->libraryFileName->valueLength + (isLibraryFilePathPathDeleimiterTerminated ? 0 : 1);

	char* libraryFilePath = alloca(sizeof(*libraryFilePath) * (libraryFilePathLength + 1));
	memcpy(libraryFilePath, library->location->value, library->location->valueLength);

	if(!isLibraryFilePathPathDeleimiterTerminated){
		libraryFilePath[library->location->valueLength] = CONSTANTS_FILE_PATH_DIRECTORY_DELIMITER;
	}

	memcpy(libraryFilePath + library->location->valueLength + (isLibraryFilePathPathDeleimiterTerminated ? 0 : 1), library->libraryFileName->value, library->libraryFileName->valueLength);
	libraryFilePath[libraryFilePathLength] = '\0';

	// Allocate memory bucket for parsing operation of library file.
	uint_fast64_t libraryFileSize;
	if((error = util_getFileSize(libraryFilePath, &libraryFileSize)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	MemoryBucket bucket;
	if((error = util_blockAlloc(&bucket, libraryFileSize)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	LinkedList libraries = {0};	
	if((error = mediaLibrary_parseLibraryFile(library, libraryFilePath, &bucket, libraryFileSize, &libraries)) != ERROR_NO_ERROR){
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

	// Debug: ...
	if((error = mediaLibrary_addLibrary(library, LIBRARY_TYPE_MOVIE, "Movies HD", strlen("Movies HD"))) != ERROR_NO_ERROR){
		UTIL_LOG_CONSOLE_(LOG_ERR, "Adding library failed with error:'%s'.", util_toErrorString(error));

		exit(1);
	}

	return ERROR(error);
}

inline ERROR_CODE mediaLibrary_parseLibraryFile(MediaLibrary* library, const char* libraryFilePath, MemoryBucket* bucket, uint_fast64_t libraryFileSize, LinkedList* libraries){
	ERROR_CODE error;

	uint8_t* buffer = *bucket;

	if((error = util_loadFile(libraryFilePath, libraryFileSize, &buffer)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return mediaLibrary_parseLibraryFileContent_(libraries, buffer, libraryFileSize);

	return ERROR(ERROR_NO_ERROR);
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

ERROR_CODE mediaLibrary_addLibrary(MediaLibrary* library, const LibraryType type, const char* name, const uint_fast64_t nameLength){
	// Create Library struct.
	Library* _library;
	_mediaLibrary_initLibrary(&_library, type, name, nameLength);

	const uint_fast64_t librarySize = mediaLibrary_getLibrarySize(type);

	// Add Library struct to list of libraries.
	void* libraries = realloc(library->libraries, library->libraryStructsize + librarySize);
	// TODO:(jan) Check for 'ENOMEM' to see if the realloc call failed.
	if(libraries == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}else{
		library->libraries = libraries;
	}

	library->libraryStructsize += librarySize;

	// Sanitize library name.
	uint_fast64_t sanitizedLibraryNameLength = nameLength;
	char* sanitizedLibraryName = alloca(sizeof(*sanitizedLibraryName) * (nameLength + 1));

	mediaLibrary_sanitizeLibraryString(name, &sanitizedLibraryName, &sanitizedLibraryNameLength);

	ERROR_CODE error;
	if((error = _mediaLibrary_createLibraryDirectory(library, sanitizedLibraryName, sanitizedLibraryNameLength)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	if((error = _mediaLibrary_updateLibraryFile(library, sanitizedLibraryName, sanitizedLibraryNameLength)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	library->numLibraries += 1;

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE __INTERNAL_USE__ _mediaLibrary_updateLibraryFile(MediaLibrary* library, const char* name, const uint_fast64_t nameLength){

}

inline ERROR_CODE __INTERNAL_USE__ mediaLibrary_createLibraryDirectory_(MediaLibrary* library, const char* name, const uint_fast64_t nameLength){
	// Create directory.
	const bool isLibraryFilePathPathDeleimiterTerminated = library->location->value[library->location->valueLength - 1] == CONSTANTS_FILE_PATH_DIRECTORY_DELIMITER;

	const uint_fast64_t libraryDirectoryPathLength = library->location->valueLength + (isLibraryFilePathPathDeleimiterTerminated ? 0 : 1) + nameLength;

	char* libraryDirectoryPath = alloca(sizeof(*libraryDirectoryPath) * (libraryDirectoryPathLength + 1));
	memcpy(libraryDirectoryPath, library->location->value, library->location->valueLength);

	if(!isLibraryFilePathPathDeleimiterTerminated){
		libraryDirectoryPath[library->location->valueLength] = CONSTANTS_FILE_PATH_DIRECTORY_DELIMITER;
	}

	memcpy(libraryDirectoryPath + library->location->valueLength + (isLibraryFilePathPathDeleimiterTerminated ? 0 : 1), name, nameLength);
	libraryDirectoryPath[libraryDirectoryPathLength] = '\0';

	ERROR_CODE error;
	if((error = util_createDirectory(libraryDirectoryPath)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	return ERROR(ERROR_NO_ERROR);
}

inline void __INTERNAL_USE__ mediaLibrary_initLibrary_(Library** library, const LibraryType type, const char* name, const uint_fast64_t nameLength){
	*library = calloc(1, mediaLibrary_getLibrarySize(type));

	(*library)->nameLength = nameLength;

	(*library)->name = malloc(sizeof(*(*library)->name) * (nameLength + 1));
	memcpy((*library)->name, name, nameLength + 1);
}

inline void mediaLibrary_sanitizeLibraryString(const char* src, char** dst, uint_fast64_t* dstStringLength){
	memcpy(*dst, src, *dstStringLength + 1);
	
	// Colapse all double spaces.
	util_replaceAll(*dst, *dstStringLength, dstStringLength, "  ", 2, " ", 1);

	// Replace remaining white space.
	util_replaceAllChars(*dst, ' ', '_');
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

ssize_t mediaLibrary_getLibrarySize(LibraryType type){
	switch (type){
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

		readOffsert += mediaLibrary_getLibrarySize(_library->type);
	}
}

#endif