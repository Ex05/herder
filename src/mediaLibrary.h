# ifndef MEDIA_LIBRARY_H
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
	MEDIA_TYPE_DOCUMENT
}MediaType;

typedef enum{
	MKV,
}VideoFormat;

typedef struct{
	char* name;
	int_fast64_t nameLength;
	DoublyLinkedList seasons;
}Show;

typedef struct{
	uint_fast64_t number;
	DoublyLinkedList episodes;
}Season;

typedef struct{
	char* name;
	int_fast64_t nameLength;
	int_fast16_t number;
	VideoFormat VideoFormat;
}Episode;

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
	uint_fast16_t fileExtensionLength;
	int_fast16_t seasonNumber;
	int_fast16_t episodeNumber;
}EpisodeInfo;

typedef struct{
	char* name;
	int_fast64_t nameLength;
	MediaType mediaType;
	DoublyLinkedList data;
}Library;

typedef struct{
	DoublyLinkedList libraries;
	Property* location;
}MediaLibrary;

#define MEDIALIBRARY_FREE_FUNCTION(functionName) void functionName(Library* library)
typedef MEDIALIBRARY_FREE_FUNCTION(MediaLibrary_freeFunction);

ERROR_CODE mediaLibrary_init(MediaLibrary*, PropertyFile*);

ERROR_CODE mediaLibrary_createLibrary(MediaLibrary*, char*, const int_fast64_t);

void mediaLibrary_free(MediaLibrary*);

void mediaLibrary_freeShow(Show*);

void mediaLibrary_freeSeason(Season*);

void mediaLibrary_freeEpisode(Episode*);

void mediaLibrary_freeEpisodeInfo(EpisodeInfo*);

#endif