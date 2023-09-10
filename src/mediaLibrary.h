#ifndef MEDIA_LIBRARY_H
#define MEDIA_LIBRARY_H

#include "linkedList.h"
#include "util.h"
#include "doublyLinkedList.h"
#include "properties.h"

typedef enum{
	LIBRARY_TYPE_SHOW = 0,
	LIBRARY_TYPE_MOVIE,
	LIBRARY_TYPE_AUDIO_ONLY_SHOW,
	LIBRARY_TYPE_PODCAST,
	LIBRARY_TYPE_AUDIO_BOOK,
	LIBRARY_TYPE_BOOK,
	LIBRARY_TYPE_PICTURE,
	LIBRARY_TYPE_VIDEO,
	LIBRARY_TYPE_DOCUMENT,

	// Ignore, used for enumerating over elements.
	LIBRARY_TYPE_NUM_ELEMENTS
}LibraryType;

typedef enum{
	MKV,
	AV1
}VideoFormat;

typedef struct{
	int16_t episodeNumber;
	int16_t nameLength;
	char* name;
}MediaLibrary_Episode;

typedef struct{
	int8_t seasonNumber;
	int16_t numEpisodes;
	MediaLibrary_Episode** episodes;
}MediaLibrary_Season;

typedef struct{
	int8_t numSeasons;
	int16_t showNameLength;
	MediaLibrary_Season** seasons;
}MediaLibrary_Show;

typedef struct{
	char* showName;
	char* episodeName;
	char* path;
	char* fileName;
	char* fileExtension;
	int_fast64_t showNameLength;
	int_fast64_t episodeNameLength;
	int_fast64_t pathLength;
	int_fast64_t fileNameLength;
	int_fast16_t fileExtensionLength;
	int_fast16_t seasonNumber;
	int_fast16_t episodeNumber;
}EpisodeInfo;

typedef struct{
	LibraryType type;
	int_fast64_t nameLength;
	char* name;
}Library;

typedef struct{
	LibraryType mediaType;
	int_fast64_t nameLength;
	char* libraryName;
	VideoFormat VideoFormat;
	MediaLibrary_Show* shows;
	MediaLibrary_Season* seasons;
	MediaLibrary_Episode* episodes;
	char** showNames;
	char** episodeNames;
}ShowLibrary;

typedef struct{
	Property* location;
	Property* libraryFileName;
	LinkedList libraries;
}MediaLibrary;

/*
	Most used operations:
		- List shows -> Select show -> List episodes -> Select episode.
		- Add episode.

	Library:
	All counting variables are signed at this point

	library_directory: ('.library' file){
		library:{
			library_type: 		1 byte;
			library_nameLength: 1 byte;	// As defined by FILE_NAME_MAX = 255.
			library_name:		~ bytes; 	// (incl. 0 termination);
		}
	}

TYPE_SHOW:{
	show:{
		show_nameLength: 1 bytes;
		show_name: (incl. 0 termination);
		num_seasons: 1 byte;

		seasson:{
			season_number: 1 byte;
			num_episodes: 2 bytes;
			icon_pathLength: 2 bytes; // As defined by PATH_MAX = 4096.
			icon_path: 	~ bytes;

			episode:{
				episode_number: 2 bytes;
				episode_nameLength: 2 bytes;
				episode_name: (incl. 0 termination);
			}
		}
	}
}
*/

#define MEDIA_LIBRARY_FREE_FUNCTION(functionName) void functionName(Library* _library)
typedef MEDIA_LIBRARY_FREE_FUNCTION(MediaLibrary_freeFunction);

#define MEDIA_LIBRARY_MEDIA_LIBRARY_FILE_PATH(library, name) uint_fast64_t name ## Length; \
char* name; \
do{ \
	const bool isLibraryFilePathPathDeleimiterTerminated = (library)->location->value[(library)->location->valueLength - 1] == CONSTANTS_FILE_PATH_DIRECTORY_DELIMITER; \
	name ## Length = (library)->location->valueLength + (library)->libraryFileName->valueLength + (isLibraryFilePathPathDeleimiterTerminated ? 0 : 1); \
	\
	name = alloca(sizeof(*name) * (name ## Length + 1)); \
	memcpy(name, (library)->location->value, (library)->location->valueLength); \
	\
	if(!isLibraryFilePathPathDeleimiterTerminated){ \
		name[(library)->location->valueLength] = CONSTANTS_FILE_PATH_DIRECTORY_DELIMITER; \
	} \
	\
	memcpy(name + (library)->location->valueLength + (isLibraryFilePathPathDeleimiterTerminated ? 0 : 1), (library)->libraryFileName->value, (library)->libraryFileName->valueLength); \
	name[name ## Length] = '\0'; \
}while(0);

ERROR_CODE mediaLibrary_init(MediaLibrary*, PropertyFile*);

ERROR_CODE mediaLibrary_createLibrary(MediaLibrary*, char*, const int_fast64_t);

void mediaLibrary_free(MediaLibrary*);

void mediaLibrary_freeEpisodeInfo(EpisodeInfo*);

ERROR_CODE mediaLibrary_parseLibraryFile(MediaLibrary*, const char*, MemoryBucket*, uint_fast64_t, LinkedList*);

ERROR_CODE mediaLibrary_parseLibraryFileContent_(LinkedList*, uint8_t*, uint_fast64_t);

ssize_t mediaLibrary_getLibrarySize(LibraryType);

void __INTERNAL_USE__ _mediaLibrary_initLibrary(Library**, const LibraryType, const char*, const uint_fast64_t);

void mediaLibrary_sanitizeLibraryString(const char*, char**, uint_fast64_t*);

void mediaLibrary_reverseSanitize(char*, const uint_fast64_t);

ERROR_CODE mediaLibrary_addLibrary(MediaLibrary*, const LibraryType, const char*, const uint_fast64_t);

ERROR_CODE __INTERNAL_USE__ _mediaLibrary_createLibraryDirectory(MediaLibrary*, const char*, const uint_fast64_t);

ERROR_CODE __INTERNAL_USE__ _mediaLibrary_updateLibraryFile(MediaLibrary*, const LibraryType, const char*, const uint_fast64_t);
#endif