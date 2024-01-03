#ifndef MEDIA_LIBRARY_TEST_C
#define MEDIA_LIBRARY_TEST_C

#include "../test.c"
#include <string.h>

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

TEST_TEST_FUNCTION(mediaLibrary_parseLibraryFileContent_){
	uint8_t buffer[] = {0x0, 0x5, 0x74, 0x65, 0x73, 0x74, 0x0, 0x0, 0x5, 0x31, 0x32, 0x33, 0x34, 0x0, 0x1, 0x6, 0x35, 0x36, 0x37, 0x38, 0x39, 0x0};

	const uint_fast64_t bufferSize = UTIL_ARRAY_LENGTH(buffer);

	LinkedList libraries = {0};

	if(mediaLibrary_parseLibraryFileContent_(&libraries, buffer, bufferSize) != ERROR_NO_ERROR){
		return TEST_FAILURE("%s", "Failed to parse library file.");
	}

	LinkedListIterator it;
	linkedList_initIterator(&it, &libraries);
	
	// Entry_0:
	{
		Library* library = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Library);

		if(library->type != LIBRARY_TYPE_MOVIE){
			return TEST_FAILURE("Library type '%d' != '%d'", library->type,  LIBRARY_TYPE_MOVIE);
		}

		if(strncmp(library->name, "56789", library->nameLength + 1) != 0){
			return TEST_FAILURE("Library name '%s' != '%s'", library->name, "56789");
		}

		free(library);
	}

	// Entry_1:
	{
		Library* library = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Library);

		if(library->type != LIBRARY_TYPE_SHOW){
			return TEST_FAILURE("Library type '%d' != '%d'", library->type,  LIBRARY_TYPE_MOVIE);
		}

		if(strncmp(library->name, "1234", library->nameLength + 1) != 0){
			return TEST_FAILURE("Library name '%s' != '%s'", library->name, "1234");
		}

		free(library);
	}

	// Entry_2:
	{
		Library* library = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Library);

		if(library->type != LIBRARY_TYPE_SHOW){
			return TEST_FAILURE("Library type '%d' != '%d'", library->type,  LIBRARY_TYPE_MOVIE);
		}

		if(strncmp(library->name, "test", library->nameLength + 1) != 0){
			return TEST_FAILURE("Library name '%s' != '%s'", library->name, "test");
		}

		free(library);
	}

	linkedList_free(&libraries);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(mediaLibrary_sanitizeLibraryString){
	char s[] = "Moovies   HD";

	uint_fast64_t sanitizedStringLength = strlen(s);
	char* sanitizedString = alloca(sizeof(*sanitizedString) * (sanitizedStringLength+ 1));

	mediaLibrary_sanitizeLibraryString(s, &sanitizedString, &sanitizedStringLength);

	if(strncmp(sanitizedString, "Moovies_HD", sanitizedStringLength + 1) != 0){
		return TEST_FAILURE("Sanitized string '%s' != '%s'", sanitizedString, "Moovies_HD");
	}

	return TEST_SUCCESS;
}

#endif