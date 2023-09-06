#ifndef MEDIA_LIBRARY_TEST_C
#define MEDIA_LIBRARY_TEST_C

#include "../test.c"

TEST_TEST_FUNCTION(mediaLibrary_getLibraryFreeFunction){
	MediaLibrary_freeFunction* mediaLibraryFreeFunctions[] = {
		(MediaLibrary_freeFunction*) mediaLibrary_freeShowLibrary,
		(MediaLibrary_freeFunction*) NULL,
		(MediaLibrary_freeFunction*) NULL,
		(MediaLibrary_freeFunction*) NULL,
		(MediaLibrary_freeFunction*) NULL,
		(MediaLibrary_freeFunction*) NULL,
		(MediaLibrary_freeFunction*) NULL,
		(MediaLibrary_freeFunction*) NULL,
		(MediaLibrary_freeFunction*) NULL,
	};
	
	ShowLibrary showLibrary = {0};

	int_fast8_t i;
	for(i = 0; i < LIBRARY_TYPE_NUM_ELEMENTS; i++){
		showLibrary.mediaType = i;

		MediaLibrary_freeFunction* libraryFreeFunction = mediaLibrary_getLibraryFreeFunction((Library*) &showLibrary);

		if(libraryFreeFunction != mediaLibraryFreeFunctions[i]){
			return TEST_FAILURE("Failed to return address of '%s' function.", "mediaLibrary_freeShowLibrary");
		}
	}

	return TEST_SUCCESS;
}

#endif