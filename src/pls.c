#ifndef PLS_C
#define PLS_C

#include "pls.h"

ERROR_CODE pls_savePlayList(Property* libraryDirectory, const char* fileName, Show* show){
    ERROR_CODE error;

    FILE* file = fopen(fileName, "w+");

    if(file == NULL){
        UTIL_LOG_ERROR_("Failed to create file: '%s' - '%s'.", fileName, strerror(errno));

        return ERROR(ERROR_FAILED_TO_CREATE_FILE);
    }

    fprintf(file, "[playlist]\n\n");

    Season** seasons = alloca(sizeof(*seasons) * show->seasons.length);
    if((error = mediaLibrary_sortSeasons(&seasons, &show->seasons)) != ERROR_NO_ERROR){
        goto label_close;
    }

    uint_fast64_t numEntries;
    register uint_fast64_t i;    
    for(i = 0, numEntries = 1; i < show->seasons.length; i++){
        Season* season = seasons[i];

        Episode** episodes = alloca(sizeof(*episodes) * season->episodes.length);
        if((error = mediaLibrary_sortEpisodes(&episodes, &season->episodes)) != ERROR_NO_ERROR){
            goto label_close;
        }

        register uint_fast64_t j;
        for(j = 0; j < season->episodes.length; j++, numEntries++){
            Episode* episode = episodes[j];

            EpisodeInfo episodeInfo;
            mediaLibrary_fillEpisodeInfo(&episodeInfo, show, season, episode);

            char* relativePath;
            uint_fast64_t relativePathLength;
            HERDER_CONSTRUCT_RELATIVE_FILE_PATH(&relativePath, &relativePathLength, &episodeInfo);

            const uint_fast64_t absoluteFilePathLength = (libraryDirectory->entry->length - 1) + relativePathLength + 1;

            char* absoluteFilePath = alloca(sizeof(*absoluteFilePath) * (absoluteFilePathLength + 1));
            uint_fast64_t writeOffset = 0;

            strncpy(absoluteFilePath + writeOffset, (char*) libraryDirectory->buffer, libraryDirectory->entry->length - 1);
            writeOffset += libraryDirectory->entry->length - 1;

            strncpy(absoluteFilePath + writeOffset, relativePath, relativePathLength);
            writeOffset += relativePathLength;
            
            absoluteFilePath[writeOffset] = '\0';

            fprintf(file, "File%" PRIuFAST64 "=%s\n", numEntries, absoluteFilePath);            
            fprintf(file, "Title%" PRIuFAST64 "=%s s%02" PRIdFAST16 "e%02" PRIdFAST16 " %s.%s\n\n", numEntries,episodeInfo.showName, episodeInfo.season, episodeInfo.episode, episodeInfo.name, episodeInfo.fileExtension);     
        }
    }

    fprintf(file, "NumberOfEntries=%" PRIuFAST64 "\n", numEntries);
    fprintf(file, "Version=2");

label_close:
    if(fflush(file) != 0){
        return ERROR(ERROR_DISK_ERROR);
    }

    fclose(file);

    return ERROR(error);
}

#endif