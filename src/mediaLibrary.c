#ifndef MEDIA_LIBRARY_C
#define MEDIA_LIBRARY_C

#include "mediaLibrary.h"

#include "linkedList.c"
#include "util.c"

#define LIBRARY_FILE_NAME "lib_data"

local const Version MEDIA_LIBRARY_FILE_VERSION = {0, 1, 0};

local ERROR_CODE mediaLibrary_extractShowName(EpisodeInfo*, LinkedList*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_extractEpisodeNumber(EpisodeInfo*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_extractEpisodeName(EpisodeInfo*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_extractSeasonNumber(EpisodeInfo*, char*, const uint_fast64_t);

local ERROR_CODE mediaLibrary_containsShow(LinkedList*, Show**, const char*, const uint_fast64_t);

local ERROR_CODE medialibrary_initShow(Show*, const char*, const uint_fast64_t);

local ERROR_CODE medialibrary_initEpisode(Episode*, const uint_fast16_t, const char*, const uint_fast64_t, const char*, const uint_fast16_t);

local ERROR_CODE medialibrary_initSeason(Season*, const uint_fast16_t);

local bool mediaLibrary_isCharcterWordDelimiter(char);

local void mediaLibrary_freeShow(Show*);

local void mediaLibrary_freeSeason(Season*);

local void mediaLibrary_freeEpisode(Episode*);

local ERROR_CODE mediaLibrary_readShowNameLength(FILE*, uint_fast64_t*);

local ERROR_CODE mediaLibrary_readShowName(FILE*, char**, uint_fast64_t);

local ERROR_CODE mediaLibrary_readSeasonNumber(FILE*, uint_fast16_t*);

local ERROR_CODE mediaLibrary_readEpisodeNumber(FILE*, uint_fast16_t*);

local ERROR_CODE mediaLibrary_readEpisodeNumber(FILE*, uint_fast16_t*);

local ERROR_CODE mediaLibrary_readEpisodeNameLength(FILE*, uint_fast64_t*);

local ERROR_CODE mediaLibrary_readEpisodeName(FILE*, char**, uint_fast64_t);

local ERROR_CODE mediaLibrary_readFileExtensionLength(FILE*, uint_fast16_t*);

local ERROR_CODE mediaLibrary_readFileExtension(FILE*, char**, uint_fast64_t);

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

        if(fread(&library->version, sizeof(library->version), 1, file) != 1){
            UTIL_LOG_ERROR("Failed to read media library file version.");

            return ERROR(ERROR_READ_ERROR);
        }

        if(MEDIA_LIBRARY_FILE_VERSION.release != library->version.release && MEDIA_LIBRARY_FILE_VERSION.update == library->version.update && MEDIA_LIBRARY_FILE_VERSION.hotfix == library->version.hotfix){
            UTIL_LOG_ERROR_("Version missmatch [%" PRIu8 ".%" PRIu8 ".%" PRIu8"].", library->version.release, library->version.update, library->version.hotfix);

            return ERROR(ERROR_VERSION_MISSMATCH);
        }

        for(;;){
            // Show_Name.
            uint_fast64_t showNameLength;
            if((error = mediaLibrary_readShowNameLength(file, &showNameLength)) != ERROR_NO_ERROR){
                if(error == ERROR_END_OF_FILE){
                    break;
                }

                goto label_return;
            }

            char* showName; 
            showName = alloca(sizeof(*showName) * (showNameLength + 1));

            if((error = mediaLibrary_readShowName(file, &showName, showNameLength)) != ERROR_NO_ERROR){
                goto label_return;
            }

            const bool entryWasRemoved = showNameLength != 0 && *showName == '\0';

            Show* show;
            if(!entryWasRemoved && (error = mediaLibrary_addShow(library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
                if(error != ERROR_DUPLICATE_ENTRY){
                    return ERROR(error);
                }
            }

            // Season_Number.
            uint_fast16_t seasonNumber;
            if((error = mediaLibrary_readSeasonNumber(file, &seasonNumber)) != ERROR_NO_ERROR){
                goto label_return;
            }

            Season* season;
            if(!entryWasRemoved && (error = medialibrary_getSeason(show, &season, seasonNumber)) != ERROR_NO_ERROR){
                if(error == ERROR_ENTRY_NOT_FOUND){
                    if((error = mediaLibrary_addSeason(library, &season, show, seasonNumber)) != ERROR_NO_ERROR){
                        goto label_return;
                    }
                }else{
                    goto label_return;
                }
            }

            // Episode_Number.
            uint_fast16_t episodeNumber;
            if((error = mediaLibrary_readEpisodeNumber(file, &episodeNumber)) != ERROR_NO_ERROR){
                goto label_return;
            }

            // Episode_Name.
            uint_fast64_t episodeNameLength;
            if((error = mediaLibrary_readEpisodeNameLength(file, &episodeNameLength)) != ERROR_NO_ERROR){
                goto label_return;
            }

            char* episodeName; 
            episodeName = alloca(sizeof(*episodeName) * (episodeNameLength + 1));

            if((error = mediaLibrary_readEpisodeName(file, &episodeName, episodeNameLength)) != ERROR_NO_ERROR){
                goto label_return;
            }

            // File_Extension.
            uint_fast16_t fileExtensionLength;
            if((error = mediaLibrary_readFileExtensionLength(file, &fileExtensionLength)) != ERROR_NO_ERROR){
                goto label_return;
            }

            char* fileExtension; 
            fileExtension = alloca(sizeof(*fileExtension) * (fileExtensionLength + 1));

            if((error = mediaLibrary_readFileExtension(file, &fileExtension, fileExtensionLength)) != ERROR_NO_ERROR){
                goto label_return;
            }
            
            Episode* episode;
            if(!entryWasRemoved){
                error = mediaLibrary_addEpisode(library, &episode, show, season, episodeNumber, episodeName, episodeNameLength, fileExtension, fileExtensionLength,  false);
            }            

        label_return:
            if(error != ERROR_NO_ERROR){
                return ERROR(error);
            }
        }
    }
    
    return ERROR(ERROR_NO_ERROR);
}

inline void mediaLibrary_freeShow(Show* show){
    linkedList_free(&show->seasons);

    free(show->name);
    free(show);
}

inline void mediaLibrary_freeSeason(Season* season){
    linkedList_free(&season->episodes);
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

        LinkedListIterator seasonIterator;
        linkedList_initIterator(&seasonIterator, &show->seasons);

        while(LINKED_LIST_ITERATOR_HAS_NEXT(&seasonIterator)){
            Season* season = LINKED_LIST_ITERATOR_NEXT(&seasonIterator);

            LinkedListIterator episodeIterator;
            linkedList_initIterator(&episodeIterator, &season->episodes);

            while(LINKED_LIST_ITERATOR_HAS_NEXT(&episodeIterator)){
                Episode* episode = LINKED_LIST_ITERATOR_NEXT(&episodeIterator);

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
    // NOTE:(jan) All the strlen calls are needed, as each of the 'medialibrary_extract*' calls modify the file name.
    ERROR_CODE error = ERROR_NO_ERROR;
    if(mediaLibrary_extractSeasonNumber(info, fileName, strlen(fileName)) != ERROR_NO_ERROR){
        error = ERROR_INCOMPLETE;
    }

    if(mediaLibrary_extractEpisodeNumber(info, fileName, strlen(fileName)) != ERROR_NO_ERROR){
        error = ERROR_INCOMPLETE;
    }

    if(mediaLibrary_extractShowName(info, shows, fileName, strlen(fileName)) != ERROR_NO_ERROR){
        error = ERROR_INCOMPLETE;
    }

    if(mediaLibrary_extractEpisodeName(info, fileName, strlen(fileName)) != ERROR_NO_ERROR){
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

    util_replaceAllChars(info->name, '_', ' ');

    info->nameLength = fileNameLength;

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_extractPrefixedNumber(char* fileName, uint_fast64_t fileNameLength, int_fast16_t* value, const char prefix){
    char* lowerChaseFileName = alloca(sizeof(*lowerChaseFileName) * (fileNameLength + 1));
    util_stringCopy(lowerChaseFileName, fileName, fileNameLength + 1);
    util_toLowerChase(lowerChaseFileName);

    int_fast64_t offset;
    int_fast64_t writeOffset = 0;
    while((offset = util_findFirst(lowerChaseFileName, fileNameLength, prefix)) != -1){
        lowerChaseFileName += ++offset;
        fileNameLength -= offset;
        writeOffset += offset;

        uint_fast64_t i;
        for(i = 0; i < fileNameLength; i++){
            const char c = lowerChaseFileName[i];

            if(isdigit(c) == 0){
                break;
            }
        }

        if(i > 0){
            char* numberString = alloca(sizeof(*numberString) * (i + 1));
            strncpy(numberString, lowerChaseFileName, i);
            numberString[i] = '\0';

            util_stringCopy(fileName + writeOffset - 1, fileName + writeOffset + i, strlen(fileName + writeOffset) + 1 - i);

            char* endPtr;
            *value = strtol(numberString, &endPtr, 10);

            if((*value == 0 && endPtr == numberString) || *value == LONG_MIN || *value == LONG_MAX){
                *value = 0;

                return ERROR_(ERROR_CONVERSION_ERROR, "'%s' is not a valid number.", numberString);
            }

            return ERROR(ERROR_NO_ERROR);
        }
    }

    *value = 0;

    return ERROR(ERROR_ENTRY_NOT_FOUND);
}

inline ERROR_CODE mediaLibrary_extractEpisodeNumber(EpisodeInfo* info, char* fileName, const uint_fast64_t fileNameLength){
    ERROR_CODE error;
    if((error = mediaLibrary_extractPrefixedNumber(fileName, fileNameLength, &info->episode, 'e')) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if(info->episode == 0){
        return ERROR(ERROR_INCOMPLETE);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_extractSeasonNumber(EpisodeInfo* info, char* fileName, const uint_fast64_t fileNameLength){
    ERROR_CODE error;
    if((error = mediaLibrary_extractPrefixedNumber(fileName, fileNameLength, &info->season, 's')) != ERROR_NO_ERROR){
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

    LinkedList wordList;
    linkedList_init(&wordList);

    char* s = alloca(sizeof(*s) * (fileNameLength + 1));
    strncpy(s, fileName, fileNameLength + 1);
    util_toLowerChase(s);

    while(remainingCharacter > 0){
        char c;
        do{
            c = s[endIndex++];
        }while(c != '\0' && !mediaLibrary_isCharcterWordDelimiter(c));

        if(endIndex == 1){
            goto label_continue; 
        }

        char* chunk = alloca(sizeof(*chunk) * endIndex);
        memcpy(chunk, s, endIndex - 1);
        chunk[endIndex - 1] = '\0';

        linkedList_add(&wordList, chunk);

label_continue:
        s += endIndex;
        
        remainingCharacter -= endIndex;
        endIndex = 0;
    }

    // Iterate over each word chunk and search the existing shows names for the best match.
    LinkedListIterator showIterator;
    linkedList_initIterator(&showIterator, shows);

    uint_fast32_t maxHits = 0;
    Show* mostLikely = NULL;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&showIterator)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&showIterator);

        char* lowerChaseShowName = alloca(sizeof(*lowerChaseShowName) * (show->nameLength + 1));
        strncpy(lowerChaseShowName, show->name, show->nameLength + 1);
        util_toLowerChase(lowerChaseShowName);

        LinkedListIterator wordListIterator;
        linkedList_initIterator(&wordListIterator, &wordList);

        uint_fast32_t hits = 0;
        while(LINKED_LIST_ITERATOR_HAS_NEXT(&wordListIterator)){
            char* chunk = LINKED_LIST_ITERATOR_NEXT(&wordListIterator);

            if(strstr(lowerChaseShowName, chunk) != NULL){
                hits++;
            }
        }

        if(hits > maxHits){
            mostLikely = show;

            maxHits = hits;
        }
    }

    linkedList_free(&wordList);

    if(mostLikely != NULL){
        info->showName = malloc(sizeof(*info->showName) * (mostLikely->nameLength + 1));
        if(info->showName == NULL){
            return ERROR(ERROR_OUT_OF_MEMORY);
        }
        strncpy(info->showName, mostLikely->name, mostLikely->nameLength + 1);
        info->showNameLength = mostLikely->nameLength;

        char* lowerCaseFileName = alloca(sizeof(*s) * (fileNameLength + 1));
        strncpy(lowerCaseFileName, fileName, fileNameLength + 1);
        util_toLowerChase(lowerCaseFileName);

        char* lowerCaseShowName = alloca(sizeof(*lowerCaseShowName) * (mostLikely->nameLength + 1));
        strncpy(lowerCaseShowName, mostLikely->name, mostLikely->nameLength + 1);
        util_toLowerChase(lowerCaseShowName);
        util_replaceAllChars(lowerCaseShowName, ' ', '_');

        char* beginIndex = strstr(lowerCaseFileName, lowerCaseShowName);

        if(beginIndex != NULL){
            const intptr_t offset = beginIndex - lowerCaseFileName ;

            const bool leadingWordDelimiterPresent = (fileName + offset)[mostLikely->nameLength] == '_';

            util_stringCopy(fileName + offset, (fileName + offset + (leadingWordDelimiterPresent? 1 : 0)) + mostLikely->nameLength, strlen(fileName + offset) + 1 - mostLikely->nameLength - (leadingWordDelimiterPresent? 1 : 0));
        }

        return ERROR(ERROR_NO_ERROR);
    }

    info->showName = NULL;
    info->showNameLength = 0;

    return ERROR(ERROR_INCOMPLETE);
}

inline bool mediaLibrary_isCharcterWordDelimiter(char c){
     return c == '.' || c == ' ' || c == '_';
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

inline ERROR_CODE mediaLibrary_removeShow(MediaLibrary* library, const char* name, const uint_fast64_t nameLength){
    Show* show = NULL;

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

    ERROR_CODE error;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if((error = mediaLibrary_containsShow(&library->shows, &show, noneWhiteSpaceName, noneWhiteSpaceNameLength)) == ERROR_ENTRY_NOT_FOUND){        
        return ERROR(error);
    }

    linkedList_remove(&library->shows, show);
    
    LinkedListIterator seasonIterator;
    linkedList_initIterator(&seasonIterator, &show->seasons);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&seasonIterator)){
        Season* season = LINKED_LIST_ITERATOR_NEXT(&seasonIterator);

        LinkedListIterator episodeIterator;
        linkedList_initIterator(&episodeIterator, &season->episodes);

        while(LINKED_LIST_ITERATOR_HAS_NEXT(&episodeIterator)){
            Episode* episode = LINKED_LIST_ITERATOR_NEXT(&episodeIterator);

            mediaLibrary_freeEpisode(episode);

            free(episode);
        }

        mediaLibrary_freeSeason(season);

        free(season);
    }

    if((error = medialibrary_removeShowFrromLibraryFile(library, name)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    UTIL_LOG_DEBUG_("Removed Show:'%s'.", show->name);

label_freeShow:
    mediaLibrary_freeShow(show);

    return ERROR(error);
}

ERROR_CODE medialibrary_removeShowFrromLibraryFile(MediaLibrary* library, const char* show){
    ERROR_CODE error = ERROR_NO_ERROR;

    const uint_fast64_t showLength = strlen(show);

    FILE* file = library->libraryFile;

    if(fseek(file, 0, SEEK_SET) != 0){
        error = ERROR_DISK_ERROR;

        goto label_return;
    }

    if(fseek(file, sizeof(library->version), SEEK_CUR) != 0){
        error = ERROR_DISK_ERROR;

        goto label_return;
    }

    int_fast8_t readBuffer[sizeof(uint64_t)];
    const int_fast8_t writeBuffer[1] = {0};
    for(;;){
        // Show_Name.
        uint_fast64_t showNameLength;
        if((error = mediaLibrary_readShowNameLength(file, &showNameLength)) != ERROR_NO_ERROR){
            goto label_return;
        }

        char* showName; 
        showName = alloca(sizeof(*showName) * (showNameLength + 1));

        if((error = mediaLibrary_readShowName(file, &showName, showNameLength)) != ERROR_NO_ERROR){
            goto label_return;
        }

        const long showNameOffset = ftell(file) - showNameLength;

        bool fillZero;
        if((fillZero = strncmp(show, showName, showLength) == 0)){
            // Overwrite  showName.
            if(fseek(file, showNameOffset, SEEK_SET) != 0){
                error = ERROR_DISK_ERROR;

                goto label_return;
            }

            if(fwrite(writeBuffer, 1, showNameLength + 1, file) != 0){
                error = ERROR_WRITE_ERROR;

                goto label_return;
            }

            // Season_Number.
            if(fwrite(writeBuffer, 1, sizeof(uint16_t), file) != 0){
                error = ERROR_WRITE_ERROR;

                goto label_return;
            }

            // Episode_Number.
            if(fwrite(writeBuffer, 1, sizeof(uint16_t), file) != 0){
                error = ERROR_WRITE_ERROR;

                goto label_return;
            }
        }else{
            // Skip Season_Number & Episode_Number.
            if(fseek(file, sizeof(uint16_t) * 2, SEEK_CUR) != 0){
                error = ERROR_DISK_ERROR;

                goto label_return;
            }
        }

        // Episode_Name.
        if(fread(readBuffer, sizeof(uint64_t), 1, file) != 1){
            if(feof(file) != 0){
                break;
            }else{
                UTIL_LOG_ERROR("Failed to read episode nameLength.");

                error = ERROR_READ_ERROR;

                goto label_return;
            }
        } 

        uint_fast64_t episodeNameLength = util_byteArrayTo_uint64(readBuffer);

        if(fillZero){
            if(fwrite(writeBuffer, 1, episodeNameLength + 1, file) != 0){
                error = ERROR_WRITE_ERROR;

                goto label_return;
            }
        }else{
            if(fseek(file, episodeNameLength + 1, SEEK_CUR) != 0){
                error = ERROR_DISK_ERROR;

                goto label_return;
            }
        }        

        // File_Extension.
        if(fread(readBuffer, sizeof(uint16_t), 1, file) != 1){
            if(feof(file) != 0){
                break;
            }else{
                UTIL_LOG_ERROR("Failed to read file extension length.");

                error = ERROR_READ_ERROR;

                goto label_return;
            }
        }

        uint_fast64_t fileExtensionLength = util_byteArrayTo_uint16(readBuffer);

         if(fillZero){
            if(fwrite(writeBuffer, 1, fileExtensionLength + 1, file) != 0){
                error = ERROR_WRITE_ERROR;

                goto label_return;
            }
        }else{
            if(fseek(file, fileExtensionLength + 1, SEEK_CUR) != 0){
                error = ERROR_DISK_ERROR;

                goto label_return;
            }
        }

    label_return:
        if(error != ERROR_NO_ERROR){
            return ERROR(error);
        }        
    }

    return ERROR(error);
}

inline ERROR_CODE medialibrary_getShow(MediaLibrary* library, Show** show,  const char* name, const uint_fast64_t nameLength){
    char* noneWhiteSpaceName = alloca(sizeof(*noneWhiteSpaceName) * (nameLength + 1));
    strncpy(noneWhiteSpaceName, name, nameLength + 1);

    return mediaLibrary_containsShow(&library->shows, show, noneWhiteSpaceName, nameLength);
}

inline ERROR_CODE medialibrary_initShow(Show* show, const char* name, const uint_fast64_t nameLength){
    show->name = malloc(sizeof(*show->name) * (nameLength + 1));
    if(show->name == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    show->nameLength = nameLength;

    strncpy(show->name, name, nameLength + 1);

    linkedList_init(&show->seasons);

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

    if((error = linkedList_add(&show->seasons, *season)) != ERROR_NO_ERROR){
        goto label_return;
    }

    UTIL_LOG_DEBUG_("Added Season:%02" PRIuFAST16 " to '%s'.", (*season)->number,  show->name);

label_return:
    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE medialibrary_getSeason(Show* show, Season** season, const uint_fast16_t number){
    LinkedListIterator it;
    linkedList_initIterator(&it, &show->seasons);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Season* season_ = LINKED_LIST_ITERATOR_NEXT(&it);

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

    error = linkedList_init(&season->episodes);

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

    if((error = linkedList_add(&season->episodes, *episode)) != ERROR_NO_ERROR){
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
    LinkedListIterator it;
    linkedList_initIterator(&it, &season->episodes);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Episode* episode_ = LINKED_LIST_ITERATOR_NEXT(&it);

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

inline ERROR_CODE mediaLibrary_initEpisodeInfo(EpisodeInfo* info){
    memset(info, 0, sizeof(*info));

    return ERROR(ERROR_NO_ERROR);
}

inline void mediaLibrary_freeEpisodeInfo(EpisodeInfo* info){
    free(info->showName);
    free(info->name);
    // free(info->fileExtension);
}

inline ERROR_CODE mediaLibrary_readShowNameLength(FILE* file, uint_fast64_t* showNameLength){
    int8_t readBuffer[sizeof(uint64_t)];
    if(fread(readBuffer, sizeof(uint64_t), 1, file) != 1){
        if(feof(file) != 0){
            __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_END_OF_FILE);
            return ERROR(ERROR_END_OF_FILE);
        }else{
            return ERROR_(ERROR_READ_ERROR, "Failed to read show nameLength. [%s]", util_toErrorString(ERROR_READ_ERROR));
        }
    }

    *showNameLength = util_byteArrayTo_uint64(readBuffer);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_readShowName(FILE* file, char** showName, uint_fast64_t showNameLength){
    if(fread(*showName, showNameLength + 1, 1, file) != 1){
        return ERROR_(ERROR_READ_ERROR, "Failed to read show name. [%s]", util_toErrorString(ERROR_READ_ERROR));
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_readSeasonNumber(FILE* file, uint_fast16_t* showNameLength){
    int8_t readBuffer[sizeof(uint16_t)];
    if(fread(readBuffer, sizeof(uint16_t), 1, file) != 1){
        return ERROR_(ERROR_READ_ERROR, "Failed to read season number. [%s]", util_toErrorString(ERROR_READ_ERROR));
    }

    *showNameLength = util_byteArrayTo_uint16(readBuffer);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_readEpisodeNumber(FILE* file, uint_fast16_t* episodeNumber){
    int8_t readBuffer[sizeof(uint16_t)];
    if(fread(readBuffer, sizeof(uint16_t), 1, file) != 1){
        return ERROR_(ERROR_READ_ERROR, "Failed to read episode number. [%s]", util_toErrorString(ERROR_READ_ERROR));
    }

    *episodeNumber = util_byteArrayTo_uint16(readBuffer);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_readEpisodeNameLength(FILE* file, uint_fast64_t* episodeNameLength){
    int8_t readBuffer[sizeof(uint_fast64_t)];
    if(fread(readBuffer, sizeof(uint_fast64_t), 1, file) != 1){
        return ERROR_(ERROR_READ_ERROR, "Failed to read episode number. [%s]", util_toErrorString(ERROR_READ_ERROR));
    }

    *episodeNameLength = util_byteArrayTo_uint64(readBuffer);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_readEpisodeName(FILE* file, char** episodeName, uint_fast64_t episodeNameLength){
    if(fread(*episodeName, episodeNameLength + 1, 1, file) != 1){
        return ERROR_(ERROR_READ_ERROR, "Failed to read episode name. [%s]", util_toErrorString(ERROR_READ_ERROR));
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_readFileExtensionLength(FILE* file, uint_fast16_t* fileExtensionLength){
    int8_t readBuffer[sizeof(uint16_t)];
    if(fread(readBuffer, sizeof(uint16_t), 1, file) != 1){
        return ERROR_(ERROR_READ_ERROR, "Failed to read episode number. [%s]", util_toErrorString(ERROR_READ_ERROR));
    }

    *fileExtensionLength = util_byteArrayTo_uint16(readBuffer);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE mediaLibrary_readFileExtension(FILE* file, char** fileExtension, uint_fast64_t fileExtensionLength){
    if(fread(*fileExtension, fileExtensionLength + 1, 1, file) != 1){
        return ERROR_(ERROR_READ_ERROR, "Failed to read episode name. [%s]", util_toErrorString(ERROR_READ_ERROR));
    }

    return ERROR(ERROR_NO_ERROR);
}

#undef PROPERTY_LIBRARY_DIRECTORY_NAME
#undef PROPERTY_IMPORT_DIRECTORY_NAME    

#endif