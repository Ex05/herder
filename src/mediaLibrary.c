#ifndef MEDIA_LIBRARY_C
#define MEDIA_LIBRARY_C

#include "mediaLibrary.h"

#include "arrayList.c"
#include "linkedList.c"
#include "util.c"

#define LIBRARY_FILE_NAME "lib_data"

local const Version MEDIA_LIBRARY_FILE_VERSION = {0, 1, 0};

local ERROR_CODE mediaLibrary_extractShowName(EpisodeInfo*, LinkedList*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_extractEpisodeNumber(EpisodeInfo*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_extractEpisodeName(EpisodeInfo*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_extractSeasonNumber(EpisodeInfo*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_containsShow(LinkedList*, Show**, const char*, const uint_fast64_t);

local Episode* mediaLibrary_containsEpisode(ArrayList*, const uint_fast16_t, const char*, const uint_fast64_t);

local ERROR_CODE medialibrary_initShow(Show*, const char*, const uint_fast64_t);

local ERROR_CODE medialibrary_initEpisode(Episode*, const uint_fast16_t, const char*, const uint_fast64_t, const char*, const uint_fast16_t);

local ERROR_CODE medialibrary_initSeason(Season*, const uint_fast16_t);

local bool mediaLibrary_isCharcterWordDelimiter(char);

local void mediaLibrary_freeShow(Show*);

local void mediaLibrary_freeSeason(Season*);

local void mediaLibrary_freeEpisode(Episode*);

local ARRAY_LIST_EXPAND_FUNCTION(mediaLibrary_expandImportList){
    return previousSize * 2;
}

local ARRAY_LIST_EXPAND_FUNCTION(mediaLibrary_seasonsExpandFunction){
    return previousSize + 1;
}

// TODO:(jan) Clean up this mess.
ERROR_CODE mediaLibrary_init(MediaLibrary* library, const char* libraryLocation, const uint_fast64_t libraryLocationLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    memset(library, 0, sizeof(*library));

    linkedList_init(&library->shows);

    if(!util_fileExists(libraryLocation)){
        if((error = util_createDirectory(libraryLocation)) != ERROR_NO_ERROR){
            return ERROR(error);
        }else{
            UTIL_LOG_INFO_("Successfully created directory: '%s'.", libraryLocation);
        }
    }

    library->libraryFileLocation = malloc(sizeof(*library->libraryFileLocation) * (libraryLocationLength + 1));
    if(library->libraryFileLocation == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    strncpy(library->libraryFileLocation, libraryLocation, libraryLocationLength + 1);
    library->libraryFileLocationLength = libraryLocationLength;

    const uint_fast64_t libraryFileNameLength = strlen(LIBRARY_FILE_NAME);
    const uint_fast64_t libraryFileLength = libraryLocationLength + libraryFileNameLength;

    char* libraryFile = alloca(sizeof(*libraryFile) * (libraryFileLength + 1));
    strncpy(libraryFile, libraryLocation, libraryLocationLength);

    strncpy(libraryFile + libraryLocationLength, LIBRARY_FILE_NAME, libraryFileNameLength);
    libraryFile[libraryFileLength] = '\0';

    if(!util_fileExists(libraryFile)){
        FILE* file = fopen(libraryFile, "w+");     

        if(file == NULL){
            UTIL_LOG_ERROR_("Failed to create file: '%s' - '%s'.", libraryFile, strerror(errno));

            return ERROR(ERROR_FAILED_TO_CREATE_FILE);
        }

        library->libraryFile = file;

        uint_fast8_t buffer[] = {
            MEDIA_LIBRARY_FILE_VERSION.release,
            MEDIA_LIBRARY_FILE_VERSION.update,
            MEDIA_LIBRARY_FILE_VERSION.hotfix
            };

        if(fwrite(buffer, 1, sizeof(MEDIA_LIBRARY_FILE_VERSION), file) != sizeof(Version)){
            UTIL_LOG_ERROR("Failed to write media library file version.");

            return ERROR(ERROR_WRITE_ERROR);
        }

        if(fflush(file) != 0){
            return ERROR(ERROR_DISK_ERROR);
        }
    }else{
        FILE* file = fopen(libraryFile, "r+");

        if(file == NULL){
            return ERROR(ERROR_FAILED_TO_OPEN_FILE);
        }

        library->libraryFile = file;

        if(fread(&library->version, 1, sizeof(library->version), file) != sizeof(library->version)){
            UTIL_LOG_ERROR("Failed to read media library file version.");

            return ERROR(ERROR_READ_ERROR);
        }

        if(MEDIA_LIBRARY_FILE_VERSION.release != library->version.release && MEDIA_LIBRARY_FILE_VERSION.update == library->version.update && MEDIA_LIBRARY_FILE_VERSION.hotfix == library->version.hotfix){
            UTIL_LOG_ERROR_("Version missmatch [%" PRIu8 ".%" PRIu8 ".%" PRIu8"].", library->version.release, library->version.update, library->version.hotfix);

            return ERROR(ERROR_VERSION_MISSMATCH);
        }

        int_fast8_t readBuffer[sizeof(uint64_t)];
        for(;;){
            // Show_Name.    
            if(fread(readBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
                  if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read show nameLength.");

                    return ERROR(ERROR_READ_ERROR);
                }
            }

            const uint_fast64_t showNameLength = util_byteArrayTo_uint64(readBuffer);
            
            // TODO:(jan) Free allocated memory on error.
            char* showName = alloca(sizeof(*showName) * (showNameLength + 1));
            if(fread(showName, 1, showNameLength + 1, file) != showNameLength + 1){
                if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read show name.");

                    error = ERROR_READ_ERROR;

                    goto label_return;
                }
            }

            Show* show;
            if((error = mediaLibrary_addShow(library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
                if(error != ERROR_DUPLICATE_ENTRY){
                    return ERROR(error);
                }
            }

            // Season_Number.
            if(fread(readBuffer, 1, sizeof(uint16_t), file) != sizeof(uint16_t)){
                if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read season number.");

                    error = ERROR_READ_ERROR;

                    goto label_return;
                }
            }

            const uint_fast16_t seasonNumber = util_byteArrayTo_uint16(readBuffer);

            Season* season;
            if((error = mediaLibrary_addSeason(library, &season, show, seasonNumber)) != ERROR_NO_ERROR){

                goto label_return;
            }

            // Episode_Number.
            if(fread(readBuffer, 1, sizeof(uint16_t), file) != sizeof(uint16_t)){
                if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read episode number.");

                    error = ERROR_READ_ERROR;

                    goto label_return;
                }
            }

            const uint_fast16_t episodeNumber = util_byteArrayTo_uint16(readBuffer);

            // Episode_Name.
            if(fread(readBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
                if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read episode nameLength.");

                    error = ERROR_READ_ERROR;

                    goto label_return;
                }
            }

            uint_fast64_t episodeNameLength = util_byteArrayTo_uint64(readBuffer);

            char* episodeName = alloca(sizeof(*episodeName) * (episodeNameLength + 1));
            if(fread(episodeName, 1, episodeNameLength + 1, file) != episodeNameLength + 1){
                if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read epsiode name.");

                    error = ERROR_READ_ERROR;

                    goto label_return;
                }
            }

            // File extension.
             if(fread(readBuffer, 1, sizeof(uint16_t), file) != sizeof(uint16_t)){
                if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read file extension length.");

                    error = ERROR_READ_ERROR;

                    goto label_return;
                }
            }

            uint_fast64_t fileExtensionLength = util_byteArrayTo_uint16(readBuffer);

            char* fileExtension = alloca(sizeof(*fileExtension) * (fileExtensionLength + 1));
            if(fread(fileExtension, 1, fileExtensionLength + 1, file) != fileExtensionLength + 1){
                if(feof(file) != 0){
                    break;
                }else{
                    UTIL_LOG_ERROR("Failed to read file extension.");

                    error = ERROR_READ_ERROR;

                    goto label_return;
                }
            }
            
            Episode* episode;
            error = mediaLibrary_addEpisode(library, &episode, show, season, episodeNumber, episodeName, episodeNameLength, fileExtension, fileExtensionLength,  false);

        label_return:
            if(error != ERROR_NO_ERROR){
                return ERROR(error);
            }
        }
    }
    
    return ERROR(ERROR_NO_ERROR);
}

inline void mediaLibrary_freeShow(Show* show){
    arrayList_free(&show->seasons);

    free(show->name);
    free(show);
}

inline void mediaLibrary_freeSeason(Season* season){
    arrayList_free(&season->episodes);
}

inline void mediaLibrary_freeEpisode(Episode* episode){
    free(episode->name);
    free(episode->fileExtension);
}

inline void mediaLibrary_free(MediaLibrary* library){
    LinkedListIterator it;
    linkedList_initIterator(&it, &library->shows);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

        ArrayListIterator seasonIterator;
        arrayList_initIterator(&seasonIterator, &show->seasons);

        while(ARRAY_LIST_ITERATOR_HAS_NEXT(&seasonIterator)){
            Season* season = ARRAY_LIST_ITERATOR_NEXT(&seasonIterator);

            ArrayListIterator episodeIterator;
            arrayList_initIterator(&episodeIterator, &season->episodes);

            while(ARRAY_LIST_ITERATOR_HAS_NEXT(&episodeIterator)){
                Episode* episode = ARRAY_LIST_ITERATOR_NEXT(&episodeIterator);

                mediaLibrary_freeEpisode(episode);

                free(episode);
            }

            mediaLibrary_freeSeason(season);

            free(season);
        }

        mediaLibrary_freeShow(show);
    }

    linkedList_free(&library->shows);

    if(library->libraryFile != NULL){
        fclose(library->libraryFile);
    }

    free(library->libraryFileLocation);
}

inline ERROR_CODE mediaLibrary_extractEpisodeInfo(EpisodeInfo* info, LinkedList* shows, char* fileName, const uint_fast64_t fileNameLength){
    char* lowerChaseFileName = alloca(sizeof(*lowerChaseFileName) * (fileNameLength + 1));
    util_stringCopy(lowerChaseFileName, fileName, fileNameLength + 1);
    util_toLowerChase(lowerChaseFileName);

    // NOTE:(jan) All the strlen calls are needed, as each of the 'medialibrary_extract*' calls modify the file name.
    ERROR_CODE error = ERROR_NO_ERROR;
    if(mediaLibrary_extractEpisodeNumber(info, lowerChaseFileName, fileNameLength) != ERROR_NO_ERROR){
        error = ERROR_INCOMPLETE;
    }

    if(mediaLibrary_extractSeasonNumber(info, lowerChaseFileName, strlen(lowerChaseFileName)) != ERROR_NO_ERROR){
        error = ERROR_INCOMPLETE;
    }

    if(mediaLibrary_extractShowName(info, shows, lowerChaseFileName, strlen(lowerChaseFileName)) != ERROR_NO_ERROR){
        error = ERROR_INCOMPLETE;
    }

    if(mediaLibrary_extractEpisodeName(info, lowerChaseFileName, strlen(lowerChaseFileName)) != ERROR_NO_ERROR){
        error = ERROR_INCOMPLETE;
    }

    return error;
}

inline ERROR_CODE mediaLibrary_extractEpisodeName(EpisodeInfo* info, char* fileName, uint_fast64_t fileNameLength){
    char* s;
    while((s = strstr(fileName, "__")) != NULL){
        util_stringCopy(s, s + 1, strlen(s));
    }

    if(*fileName == '_'){
        util_stringCopy(fileName, fileName + 1, strlen(fileName));
    }

    fileNameLength = strlen(fileName);

    int_fast64_t fileExtensionOffset = util_findLast(fileName, fileNameLength, '.');

    if(fileExtensionOffset != -1){
        fileName[fileExtensionOffset] = '\0';

        fileNameLength--;
    }

    if(fileNameLength <= 0){
        info->nameLength = 0;
        info->name = NULL;

        return ERROR(ERROR_INCOMPLETE);
    }

    info->name = malloc(sizeof(*info->name) * (fileNameLength + 1));
    if(info->name == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    strncpy(info->name, fileName, fileNameLength + 1);

    info->nameLength = fileNameLength;

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_extractEpisodeNumber(EpisodeInfo* info, char* fileName, const uint_fast64_t fileNameLength){
    ERROR_CODE error;
    if((error = util_extractPrefixedNumber(fileName, fileNameLength, &info->episode, 'e')) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if(info->episode == 0){
        return ERROR(ERROR_INCOMPLETE);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_extractSeasonNumber(EpisodeInfo* info, char* fileName, const uint_fast64_t fileNameLength){
    ERROR_CODE error;
    if((error = util_extractPrefixedNumber(fileName, fileNameLength, &info->season, 's')) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if(info->season == 0){
        return ERROR(ERROR_INCOMPLETE);
    }

    return ERROR(ERROR_NO_ERROR);
}

// TODO:(jan) Refactor this mess and add some guidance for readers.
ERROR_CODE mediaLibrary_extractShowName(EpisodeInfo* info, LinkedList* shows, char* fileName, const uint_fast64_t fileNameLength){
    // Split the filename into easily searchable word chunks.
    uint_fast64_t endIndex = 0;
    // We need the '/0' to terminate our loop.
    uint_fast64_t remainingCharacter = fileNameLength + 1;

    ArrayList wordList;
    arrayList_init(&wordList, 16, mediaLibrary_expandImportList);

    char* s = fileName;
    while(remainingCharacter > 0){
        char c;
        do{
            c = s[endIndex++];
        }while(c != '\0' && !mediaLibrary_isCharcterWordDelimiter(c));

        char* chunk = alloca(sizeof(*chunk) * endIndex);
        memcpy(chunk, s, endIndex - 1);
        chunk[endIndex - 1] = '\0';

        arrayList_add(&wordList, chunk);

        s += endIndex;
        
        remainingCharacter -= endIndex;
        endIndex = 0;
    }

    // Iterate over each word chunk and search the existing shows names for the best match.
    ArrayListIterator wordListIterator;
    arrayList_initIterator(&wordListIterator, &wordList);

    uint_fast32_t maxHits = 0;
    char* mostLikely = NULL;
    char* lowerChaseShowName = NULL;
    char* showName = NULL;
    uint_fast64_t nameLength;
    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&wordListIterator)){
        char* chunk = ARRAY_LIST_ITERATOR_NEXT(&wordListIterator);

        LinkedListIterator it;
        linkedList_initIterator(&it, shows);

        uint_fast32_t hits = 0;
        while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
            showName = ((Show*) LINKED_LIST_ITERATOR_NEXT(&it))->name;

            nameLength = strlen(showName);

            lowerChaseShowName = alloca(sizeof(*lowerChaseShowName) * (nameLength + 1));
            memcpy(lowerChaseShowName, showName, nameLength);
            lowerChaseShowName[nameLength] = '\0';

            util_toLowerChase(lowerChaseShowName);

            if(strstr(lowerChaseShowName, chunk) != NULL){
                hits++;
            }
        }

        if(hits > maxHits){
            mostLikely = showName;
        }
    }

    // Remove the show name from the fileName.
    if(mostLikely != NULL){
        info->showName = malloc(sizeof(*info->showName) * (nameLength + 1));
        if(info->showName == NULL){
            return ERROR(ERROR_OUT_OF_MEMORY);
        }

        memcpy(info->showName, showName, nameLength);
        util_replaceAllChars(info->showName, '_', ' ');
        info->showName[nameLength] = '\0';
        info->showNameLength = nameLength;

        char* beginIndex = strstr(fileName, showName);

        if(beginIndex == NULL){
            if(lowerChaseShowName != NULL){
                beginIndex = strstr(fileName, lowerChaseShowName);

                 if(beginIndex != NULL){
                    util_stringCopy(beginIndex, beginIndex + nameLength, strlen(beginIndex) + 1 - nameLength);
                }
            }
        }else{
            util_stringCopy(beginIndex, beginIndex + nameLength, strlen(beginIndex) + 1 - nameLength);
        }
    }else{
        info->showNameLength = 0;
        info->showName = NULL;

        return ERROR(ERROR_INCOMPLETE);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline bool mediaLibrary_isCharcterWordDelimiter(char c){
     return c == '.' || c == ' ' || c == '_';
}

ERROR_CODE mediaLibrary_import(MediaLibrary* library, const char* importDirectory){
    UTIL_LOG_CONSOLE(LOG_CRIT, "Function not implemented!~!");

    // const char* importDir = importDirectory != NULL ? importDirectory : (char*) library->importDirectory->buffer;

    // UTIL_LOG_CONSOLE_(LOG_INFO, "Import directory: '%s'.", importDir);

    // ArrayList importList;
    // arrayList_init(&importList, mediaLibrary_expandImportList);

    // medialibrary_walkDirectory(&importList, importDir);
    
    // ArrayListIterator it;
    // arrayList_initIterator(&it, &importList);

    // while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
    //     DirectoryEntry* directoryEntry = ARRAY_LIST_ITERATOR_NEXT(&it);

    //     char* file = directoryEntry->path;

    //     const uint_fast64_t filePathLength = directoryEntry->pathLength;

    //     EpisodeInfo info = {0, 0, -1, -1};
    //     mediaLibrary_extractEpisodeInfo(&info, library, directoryEntry->fileName, filePathLength);

    //     UTIL_LOG_CONSOLE_(LOG_INFO, "Show:%s\nSeason:%" PRIuFAST16 "\nEpisode:%" PRIuFAST16 "\nEpisode name:%s", info.showName == NULL ? "'NULL''" : info.showName, info.season, info.episode, info.name == NULL ? "'NULL'" : info.name);

    //     mediaLibrary_fillEpisodeInfo(&info); 

    //     Show* show = mediaLibrary_addShow(library, info.showName, strlen(info.showName));

    //     Season* season = mediaLibrary_addSeason(library, show, info.season);

    //     Episode* episode = mediaLibrary_addEpisode(library, show, season, file, filePathLength, info.episode, info.name , strlen(info.name), true);

    //     const uint_fast64_t seasonNumberLength = snprintf(NULL, 0, "%02" PRIuFAST16 "", season->number);
    //     const uint_fast64_t episodeNumberLength = snprintf(NULL, 0, "%02" PRIuFAST16 "", episode->number);

    //     episode->pathLength = season->pathLength + show->nameLength + episode->nameLength + directoryEntry->fileExtensionLength + seasonNumberLength + episodeNumberLength + 2 /*2x '_'*/+ 2/*'s', 'e'*/;
    //     episode->path = malloc(*episode->path * (episode->pathLength + 1/*'\0'*/));

    //     snprintf(episode->path, episode->pathLength + 1, "%s%s_s%02" PRIuFAST16 "e%02" PRIuFAST16 "_%s%s", season->path, show->name, season->number, episode->number, episode->name, directoryEntry->fileExtension);

    //     if(util_fileCopy(file, episode->path)){
    //         if(!util_deleteFile(file)){
    //             UTIL_LOG_ERR_("Failed to delete file '%s'.\n", file);
    //         }
    //     }

    //     free(directoryEntry->path);
    //     free(directoryEntry->fileName);
    //     free(directoryEntry);

    //     mediaLibrary_freeEpisodeInfo(&info);
    // }

    // arrayList_free(&importList);   

    return ERROR(ERROR_NO_ERROR);
}   

inline ERROR_CODE mediaLibrary_addShow(MediaLibrary* library, Show** show, const char* name, const uint_fast64_t nameLength){
    *show = NULL;

    char* noneWhiteSpaceName = alloca(sizeof(*noneWhiteSpaceName) * (nameLength + 1));
    memcpy(noneWhiteSpaceName, name, nameLength);
    noneWhiteSpaceName[nameLength] = '\0';

    uint64_t noneWhiteSpaceNameLength = nameLength;
    do{
        ERROR_CODE error;
        if((error = util_replace(noneWhiteSpaceName, nameLength + 1, &noneWhiteSpaceNameLength, "  ", 2, " ", 1)) != ERROR_NO_ERROR){
           return ERROR(error);
        }
    }
    while(noneWhiteSpaceNameLength < nameLength);

    util_replaceAllChars(noneWhiteSpaceName, ' ', '_');

    ERROR_CODE error;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if((error = mediaLibrary_containsShow(&library->shows, show, noneWhiteSpaceName, noneWhiteSpaceNameLength)) == ERROR_NO_ERROR){
        // NOTE: One could make a strong argument here to return "ERROR_DUPLICATE_ENTRY' but the current eco-system does not support nested suppresion of error. But because the caller can not distinguish from the outside, we are for now just gona pretend everything is 'OK' and return 'ERROR_NO_ERROR'. (jan - 2019.09.04)*/
        return ERROR(ERROR_NO_ERROR);
    }

    *show = malloc(sizeof(**show));
    if(*show == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    medialibrary_initShow(*show, noneWhiteSpaceName, noneWhiteSpaceNameLength);

    linkedList_add(&library->shows, *show);

    UTIL_LOG_DEBUG_("Added Show:'%s'.", (*show)->name);

    return ERROR(ERROR_NO_ERROR);
}  

inline ERROR_CODE medialibrary_getShow(MediaLibrary* library, Show** show,  const char* name, const uint_fast64_t nameLength){
    char* noneWhiteSpaceName = alloca(sizeof(*noneWhiteSpaceName) * (nameLength + 1));
    strncpy(noneWhiteSpaceName, name, nameLength + 1);

    util_replaceAllChars(noneWhiteSpaceName, ' ', '_');

    return mediaLibrary_containsShow(&library->shows, show, noneWhiteSpaceName, nameLength);
}

inline ERROR_CODE medialibrary_initShow(Show* show, const char* name, const uint_fast64_t nameLength){
    show->name = malloc(sizeof(*show->name) * (nameLength + 1));
    if(show->name == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    show->nameLength = nameLength;

    strncpy(show->name, name, nameLength + 1);

    arrayList_init(&show->seasons, 1, mediaLibrary_seasonsExpandFunction);

    return ERROR_NO_ERROR;
}

inline ERROR_CODE mediaLibrary_containsShow(LinkedList* shows, Show** show, const char* name, const uint_fast64_t nameLength){
    LinkedListIterator it;
    linkedList_initIterator(&it, shows);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show_ = LINKED_LIST_ITERATOR_NEXT(&it);

        if(show_->nameLength == nameLength && strncmp(name, show_->name, nameLength) == 0){
            *show = show_;

            return ERROR(ERROR_NO_ERROR);
        }
    }	    

    *show = NULL;

    return ERROR_(ERROR_ENTRY_NOT_FOUND, "Show: '%s'", name);
}

inline ERROR_CODE mediaLibrary_addSeason(MediaLibrary* library, Season** season, Show* show, const uint_fast16_t number){
    ERROR_CODE error = ERROR_NO_ERROR;

    *season = malloc(sizeof(**season));
    if(*season == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    if((error = medialibrary_initSeason(*season, number)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = arrayList_add(&show->seasons, *season)) != ERROR_NO_ERROR){
        goto label_return;
    }

    UTIL_LOG_DEBUG_("Added Season:%02" PRIuFAST16 " to '%s'.", (*season)->number,  show->name);

label_return:
    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE medialibrary_getSeason(Show* show, Season** season, const uint_fast16_t number){
    ArrayListIterator it;
    arrayList_initIterator(&it, &show->seasons);

	while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        Season* season_ = ARRAY_LIST_ITERATOR_NEXT(&it);

        if(season_->number == number){
            *season = season_;

            return ERROR(ERROR_NO_ERROR);
        }
    }	    

    *season = NULL;

    return ERROR(ERROR_ENTRY_NOT_FOUND);
}

inline ERROR_CODE medialibrary_initSeason(Season* season, const uint_fast16_t number){
    ERROR_CODE error = ERROR_NO_ERROR;

    memset(season, 0, sizeof(*season));
    season->number = number;

    error = arrayList_init(&season->episodes, 1, mediaLibrary_seasonsExpandFunction);

    return ERROR(error);
}

ERROR_CODE mediaLibrary_addEpisode(MediaLibrary* library, Episode** episode, Show* show, Season* season, const uint_fast16_t number, const char* name, const uint_fast64_t nameLength, const char* fileExtension,const uint_fast16_t fileExtensionLength, const bool saveToDisk){
    ERROR_CODE error = ERROR_NO_ERROR;

    *episode = malloc(sizeof(**episode));
    if(episode == NULL){
        error = ERROR_OUT_OF_MEMORY;

        goto label_return;
    }

    if((error = medialibrary_initEpisode(*episode, number, name, nameLength, fileExtension, fileExtensionLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = arrayList_add(&season->episodes, *episode)) != ERROR_NO_ERROR){
        goto label_return;
    }

    UTIL_LOG_DEBUG_("Added Episode:%02" PRIuFAST16 " '%s' of Season:%02" PRIuFAST16 " to '%s'.", (*episode)->number, (*episode)->name, season->number, show->name);

    if(saveToDisk){
        FILE* file = library->libraryFile;

        if(fseek(file, 0, SEEK_END) != 0){
            return ERROR_(ERROR_DISK_ERROR, "%s", strerror(errno));
        }

        int_fast8_t writeBuffer[sizeof(uint64_t)];

        // Show_Name.
        util_uint64ToByteArray(writeBuffer, show->nameLength);

        if(fwrite(writeBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write show name length to library file. '%s'.", strerror(errno));
        }

        if(fwrite(show->name, 1, show->nameLength + 1, file) != show->nameLength + 1){
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write show name length to library file. '%s'.", strerror(errno));
        }

        // Season_Number.
        util_uint16ToByteArray(writeBuffer, season->number);

        if(fwrite(writeBuffer, 1, sizeof(uint16_t), file) != sizeof(uint16_t)){            
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write season number to library file. '%s'.", strerror(errno));
        }

        // Episode_Number.
        util_uint16ToByteArray(writeBuffer, (*episode)->number);
        
        if(fwrite(writeBuffer, 1, sizeof(uint16_t), file) != sizeof(uint16_t)){            
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write episode number to library file. '%s'.", strerror(errno));
        }

        // Episode_Name.
        util_uint64ToByteArray(writeBuffer, (*episode)->nameLength);

        if(fwrite(writeBuffer, 1, sizeof(uint64_t), file) != sizeof(uint64_t)){            
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write episode name length to library file. '%s'.", strerror(errno));
        }

        if(fwrite((*episode)->name, 1, (*episode)->nameLength + 1, file) != (*episode)->nameLength + 1){            
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write episode name to library file. '%s'.", strerror(errno));
        }

        // FileExtension
        util_uint16ToByteArray(writeBuffer, (*episode)->fileExtensionLength);

        if(fwrite(writeBuffer, 1, sizeof(uint16_t), file) != sizeof(uint16_t)){            
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write file extension length to library file. '%s'.", strerror(errno));
        }

        if(fwrite((*episode)->fileExtension, 1, (*episode)->fileExtensionLength + 1, file) != (*episode)->fileExtensionLength + 1){            
            return ERROR_(ERROR_WRITE_ERROR, "Failed to write file extension to library file. '%s'.", strerror(errno));
        }

        fflush(file);
    }

label_return:
    return ERROR(error);
}

inline ERROR_CODE mediaLibrary_getEpisode(Season* season, Episode** episode, const uint_fast16_t number){
    ArrayListIterator it;
    arrayList_initIterator(&it, &season->episodes);

	while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        Episode* episode_ = ARRAY_LIST_ITERATOR_NEXT(&it);

        if(episode_->number == number){
            *episode = episode_;

            return ERROR(ERROR_NO_ERROR);
        }
    }

    *episode = NULL;

    return ERROR(ERROR_ENTRY_NOT_FOUND);
}

inline ERROR_CODE medialibrary_initEpisode(Episode* episode, const uint_fast16_t number, const char* name, const uint_fast64_t nameLength, const char* fileExtension, const uint_fast16_t fileExtensionLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    episode->nameLength = nameLength;
    episode->number = number;
    episode->fileExtensionLength = fileExtensionLength;

    episode->name = malloc(sizeof(*episode->name) * (nameLength + 1));
    if(episode->name == NULL){
        error = ERROR_OUT_OF_MEMORY;

        goto label_return;
    }
    strncpy(episode->name, name, nameLength + 1);

    episode->fileExtension = malloc(sizeof(*episode->fileExtension) * (fileExtensionLength + 1));
    if(episode->fileExtension == NULL){
        error = ERROR_OUT_OF_MEMORY;

        goto label_return;
    }
    strncpy(episode->fileExtension, fileExtension, fileExtensionLength + 1);

label_return:
    return ERROR(error);
}

inline Episode* mediaLibrary_containsEpisode(ArrayList* episodes,const uint_fast16_t number, const char* name, const uint_fast64_t nameLength){
    ArrayListIterator it;
    arrayList_initIterator(&it, episodes);

	while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        Episode* episode = ARRAY_LIST_ITERATOR_NEXT(&it);

        if(number == episode->number)
            return episode;

        if(name != NULL && episode->nameLength == nameLength && strncmp(name, episode->name, nameLength) == 0)
            return episode;
    }	    

    return NULL;
}

inline ERROR_CODE mediaLibrary_initEpisodeInfo(EpisodeInfo* info){
    memset(info, 0, sizeof(*info));

    return ERROR(ERROR_NO_ERROR);
}

inline void mediaLibrary_freeEpisodeInfo(EpisodeInfo* info){
    free(info->showName);
    free(info->name);
    // free(info->fileExtension);
}

#undef PROPERTY_LIBRARY_DIRECTORY_NAME
#undef PROPERTY_IMPORT_DIRECTORY_NAME    

#endif