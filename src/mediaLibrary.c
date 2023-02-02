# ifndef MEDIA_LIBRARY_C
#define MEDIA_LIBRARY_C

#include "mediaLibrary.h"

ERROR_CODE mediaLibrary_init(MediaLibrary* library, PropertyFile* properties){
	ERROR_CODE error;

	memset(library, 0, sizeof(*library));

	// LibraryLocation.
	PROPERTIES_GET(properties, library->location, MEDIA_LIBRARY_LOCATION);

	// TODO: Load all libraries in lib directory.
	LinkedList libraries = {0};
	if((error = util_listDirectoryContent(&libraries, library->location->value, WALK_DIRECTORY_FILTER_DIRECTORIES_ONLY)) != ERROR_NO_ERROR){
		return ERROR(error);
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &libraries);
	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		char* directory = LINKED_LIST_ITERATOR_NEXT_PTR(&it, char);

		UTIL_LOG_CONSOLE_(LOG_DEBUG, "Directory: '%s'.", directory);
	}

	return ERROR(error);
}

MEDIALIBRARY_FREE_FUNCTION(mediaLibrary_freeShowLibrary){
	// TODO: Free library content.
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, &library->data);

	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Show* show = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Show);

		mediaLibrary_freeShow(show);

		free(show);
	}

	doublyLinkedList_free(&library->data);

	free(library->name);
}

MediaLibrary_freeFunction* mediaLibrary_getLibraryFreeFunction(Library* library){
	switch (library->mediaType){
			case MEDIA_TYPE_SHOW:{
				return mediaLibrary_freeShowLibrary;
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
		}

		return NULL;
}

inline void mediaLibrary_freeShow(Show* show){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, &show->seasons);

	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Season* season = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Season);

		mediaLibrary_freeSeason(season);

		free(season);
	}

	free(show->name);
}

inline void mediaLibrary_freeSeason(Season* season){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, &season->episodes);

	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Episode* episode = DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(&it, Episode);

		mediaLibrary_freeEpisode(episode);

		free(episode);
	}
}

inline void mediaLibrary_freeEpisode(Episode* episode){
	free(episode->name);
}

inline void mediaLibrary_freeEpisodeInfo(EpisodeInfo* episodeInfo){
	free(episodeInfo->showName);
	free(episodeInfo->episodeName);
	free(episodeInfo->path);
}

inline void mediaLibrary_free(MediaLibrary* library){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, &library->libraries);

	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Library* library = DOUBLY_LINKED_LIST_ITERATOR_NEXT(&it);

		// Retrieve the correct function to free this type of library.
		MediaLibrary_freeFunction* mediaLibraryFreeFunction = mediaLibrary_getLibraryFreeFunction(library);

		// Free the library content.
		mediaLibraryFreeFunction(library);
	}

	doublyLinkedList_free(&library->libraries);	
}

#endif