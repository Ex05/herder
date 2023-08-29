#ifndef MEDIA_LIBRARY_C
#define MEDIA_LIBRARY_C

#include "mediaLibrary.h"

ERROR_CODE mediaLibrary_init(MediaLibrary* library, PropertyFile* properties){
	ERROR_CODE error;

	memset(library, 0, sizeof(*library));

	// LibraryLocation.
	PROPERTIES_GET(properties, library->location, MEDIA_LIBRARY_LOCATION);

	LinkedList libraries = {0};
	if((error = util_listDirectoryContent(&libraries, library->location->value, library->location->valueLength, WALK_DIRECTORY_FILTER_DIRECTORIES_ONLY)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &libraries);
	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		char* path = LINKED_LIST_ITERATOR_NEXT_PTR(&it, char);

	// TODO: Check for 'type' file.
	char* directory;
	uint_fast64_t directoryLength;
	if((error = util_getTailDirectory(&directory, &directoryLength, path, strlen(path))) != ERROR_NO_ERROR){
		UTIL_LOG_INFO_("Failed to retrieve tail directory name from path: '%s'. Skipping directory.", path);

		continue;
	}

		UTIL_LOG_CONSOLE_(LOG_DEBUG, "Directory: '%s'.", directory);
	}

	return ERROR(error);
}

void mediaLibrary_freeShowLibrary(ShowLibrary* library){
	free(library->shows);
	free(library->seasons);
	free(library->episodes);
	free(library->showNames);
	free(library->episodeNames);
}

MediaLibrary_freeFunction* mediaLibrary_getLibraryFreeFunction(Library* library){
	switch (library->mediaType){
		case MEDIA_TYPE_SHOW:{
			return (MediaLibrary_freeFunction*) mediaLibrary_freeShowLibrary;
		}
		case MEDIA_TYPE_MOVIE:{
			break;
		}
		case MEDIA_TYPE_AUDIO_ONLY_SHOW:{
			break;
		}
		case MEDIA_TYPE_PODCAST:{
			break;
		}
		case MEDIA_TYPE_AUDIO_BOOK:{
			break;
		}
		case MEDIA_TYPE_BOOK:{
			break;
		}
		case MEDIA_TYPE_PICTURE:{
			break;
		}
		case MEDIA_TYPE_VIDEO:{
			break;
		}
		case MEDIA_TYPE_DOCUMENT:{
			break;
		}
		default:{
			break;
		}
	}

	return NULL;
}

ssize_t mediaLibrary_sizeofLibrary(Library* library){
	switch (library->mediaType){
		case MEDIA_TYPE_SHOW:{
			return sizeof(ShowLibrary);
		}
		case MEDIA_TYPE_MOVIE:{
			break;
		}
		case MEDIA_TYPE_AUDIO_ONLY_SHOW:{
			break;
		}
		case MEDIA_TYPE_PODCAST:{
			break;
		}
		case MEDIA_TYPE_AUDIO_BOOK:{
			break;
		}
		case MEDIA_TYPE_BOOK:{
			break;
		}
		case MEDIA_TYPE_PICTURE:{
			break;
		}
		case MEDIA_TYPE_VIDEO:{
			break;
		}
		case MEDIA_TYPE_DOCUMENT:{
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
	}
}

#endif