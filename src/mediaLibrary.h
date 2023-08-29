#ifndef MEDIA_LIBRARY_H
#define MEDIA_LIBRARY_H

#include "util.h"
#include "doublyLinkedList.h"
#include "properties.h"
#include <stdint.h>

typedef enum{
	MEDIA_TYPE_SHOW = 0,
	MEDIA_TYPE_MOVIE,
	MEDIA_TYPE_AUDIO_ONLY_SHOW,
	MEDIA_TYPE_PODCAST,
	MEDIA_TYPE_AUDIO_BOOK,
	MEDIA_TYPE_BOOK,
	MEDIA_TYPE_PICTURE,
	MEDIA_TYPE_VIDEO,
	MEDIA_TYPE_DOCUMENT,
	MEDIA_TYPE_NUM_ELEMENTS
}MediaType;

typedef enum{
	MKV,
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
	MediaType mediaType;
}Library;

typedef struct{
	MediaType mediaType;
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
	int8_t numLibraries;
	Library** libraries;
}MediaLibrary;

/*
	Most used operations:
		- List shows -> Select show -> List episodes -> Select episode.
		- Add episode.

	Library:
	All counting variables are signed at this point

	library_directory: (settings file){
		library_name:{
			library_type: 1 byte;

			case: TYPE_SHOW:{
				show:{
					show_nameLength: 2 bytes;
					show_name: (incl. 0 termination);
					num_seasons: 1 byte;

					seasson:{
						season_number: 1 byte;
						num_episodes: 2 bytes;

						episode:{
							episode_number: 2 bytes;
							episode_nameLength: 2 bytes;
							episode_name: (incl. 0 termination);
						}
					}
				}
			}
		}
	}
*/

#define MEDIALIBRARY_FREE_FUNCTION(functionName) void functionName(Library* _library)
typedef MEDIALIBRARY_FREE_FUNCTION(MediaLibrary_freeFunction);

ERROR_CODE mediaLibrary_init(MediaLibrary*, PropertyFile*);

ERROR_CODE mediaLibrary_createLibrary(MediaLibrary*, char*, const int_fast64_t);

void mediaLibrary_free(MediaLibrary*);

void mediaLibrary_freeEpisodeInfo(EpisodeInfo*);

#endif