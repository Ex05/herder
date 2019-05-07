#ifndef MEDIA_LIBRARY_H
#define MEDIA_LIBRARY_H

#include "util.h"

#include <dirent.h>

#include "linkedList.h"
#include "propertyFile.h"

typedef struct{
    FILE* libraryFile;
    char* libraryFileLocation;
    uint_fast64_t libraryFileLocationLength;
    LinkedList shows;
    Version version;
    volatile bool initialised;
}MediaLibrary;

typedef struct{
    char* name;
    uint_fast64_t nameLength;
    LinkedList seasons;
}Show;

typedef struct{
    uint_fast16_t number;
    LinkedList episodes;
}Season;

typedef struct{
    uint_fast64_t nameLength;
    uint_fast16_t number;
    uint_fast16_t fileExtensionLength;
    char* fileExtension;
    char* name;
}Episode;

typedef struct{
    char* showName;
    char* name;
    char* fileExtension;
    uint_fast64_t showNameLength;
    uint_fast64_t nameLength;
    uint_fast16_t fileExtensionLength;
    int_fast16_t season;
    int_fast16_t episode;
}EpisodeInfo;

ERROR_CODE mediaLibrary_init(MediaLibrary*, const char*, const uint_fast64_t);

void mediaLibrary_free(MediaLibrary*);

ERROR_CODE mediaLibrary_import(MediaLibrary*, const char*);

ERROR_CODE mediaLibrary_addShow(MediaLibrary*, Show**, const char*, const uint_fast64_t);

ERROR_CODE mediaLibrary_removeShow(MediaLibrary*, const char*, const uint_fast64_t);

ERROR_CODE medialibrary_getShow(MediaLibrary*, Show**, const char*, const uint_fast64_t);

ERROR_CODE mediaLibrary_addSeason(MediaLibrary*, Season**, Show*, const uint_fast16_t);

ERROR_CODE medialibrary_getSeason(Show*, Season**, const uint_fast16_t);

ERROR_CODE mediaLibrary_addEpisode(MediaLibrary*, Episode**, Show*, Season*, const uint_fast16_t, const char*, const uint_fast64_t, const char*, const uint_fast16_t, const bool);

ERROR_CODE mediaLibrary_getEpisode(Season*, Episode**, const uint_fast16_t);

ERROR_CODE mediaLibrary_extractEpisodeInfo(EpisodeInfo*, LinkedList*, char*, const uint_fast64_t);

ERROR_CODE mediaLibrary_initEpisodeInfo(EpisodeInfo*);

ERROR_CODE mediaLibrary_extractPrefixedNumber(char*, uint_fast64_t, int_fast16_t*, const char);

ERROR_CODE medialibrary_removeShowFrromLibraryFile(MediaLibrary*, const char*);

void mediaLibrary_freeEpisodeInfo(EpisodeInfo*);

#endif

/*
Show_Name,
Season_Number,
Episode_Number,
Episode_Name,
File_Extension
*/

/*
URL: '/showInfo'
    Num_Seasons,
        {
            Number,
            Num_Episodes,
            {
                Number,
                NameLength,
                Name                
            }
        }
*/