#include "herder.h"

#include "util.c"
#include "linkedList.c"
#include "http.c"
#include "argumentParser.c"
#include "propertyFile.c"
#include "mediaLibrary.c"

#define HTTP_PROCESSING_BUFFER_SIZE 8192 //16384

ERROR_CODE herder_pullShowList(LinkedList* shows, const char* host, const uint_fast16_t port){
    ERROR_CODE error = ERROR_NO_ERROR;
    
    void* httpProcessingBuffer;    
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/shows";
    const uint_fast64_t urlLength = strlen(url);

    HTTP_Request request;
    if((error = http_initRequest(&request, url, urlLength, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_GET)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_Response response;
    if((error = http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }

    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    if((error = http_closeConnection(socketFD)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

if(response.statusCode != _200_OK){
        error = ERROR_SERVER_ERROR;

        goto label_freeRequest;
    }

    uint_fast64_t readOffset = 0;

    const uint_fast64_t numShows = util_byteArrayTo_uint64(response.data + readOffset);
    readOffset += sizeof(uint64_t);

    if((error = linkedList_init(shows)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    uint_fast64_t i;
    for(i = 0; i < numShows; i++){
        const uint_fast64_t nameLength = util_byteArrayTo_uint64(response.data + readOffset);
        readOffset += sizeof(uint64_t);

        Show* show = malloc(sizeof(*show));
        medialibrary_initShow(show, (char*) response.data + readOffset, nameLength);

        readOffset += nameLength;

        linkedList_add(shows, show);
    }

label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }

    return ERROR(error);
}

ERROR_CODE herder_addShow(Property* remoteHost, Property* remotePort, const char* showName, const uint_fast64_t showNameLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/addShow";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    util_uint64ToByteArray(request.data + request.dataLength, showNameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, showName, showNameLength);
    request.dataLength += showNameLength;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }
    
    http_closeConnection(socketFD);

    // TODO:(jan) Decide if this is too clever, since 'data' equals 'httpProcessingBuffer' (alocated via mmap), we can always get the return code, it might just have garbage in it.
    const uint_fast64_t returnCode = util_byteArrayTo_uint64(response.data);
    if(response.statusCode != _200_OK){
        error = ERROR_INVALID_STATUS_CODE;

        UTIL_LOG_CONSOLE_(LOG_ERR, "Server_status:'%s'.", http_getStatusMsg(response.statusCode));
    }else{
        if(response.dataLength != sizeof(uint64_t)){
            error = ERROR_INVALID_CONTENT_LENGTH;
        }else{
            if(returnCode != ERROR_NO_ERROR){
                if(returnCode == ERROR_DUPLICATE_ENTRY){
                    error = ERROR_DUPLICATE_ENTRY;
                }else{
                    error = ERROR_INVALID_RETURN_CODE;
                }
            }
        }
    }
    
label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }

    return ERROR(error);
}

ERROR_CODE herder_removeShow(Property* remoteHost, Property* remotePort, const char* showName, const uint_fast64_t showNameLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/removeShow";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    util_uint64ToByteArray(request.data + request.dataLength, showNameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, showName, showNameLength);
    request.dataLength += showNameLength;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }
    
    http_closeConnection(socketFD);

    // TODO:(jan) Decide if this is too clever, since 'data' equals 'httpProcessingBuffer' (alocated via mmap), we can always get the return code, it might just have garbage in it.
    const ERROR_CODE returnCode = util_byteArrayTo_uint64(response.data);
    if(response.statusCode != _200_OK){
        error = ERROR_INVALID_STATUS_CODE;

        UTIL_LOG_CONSOLE_(LOG_ERR, "Server_status:'%s'.", http_getStatusMsg(response.statusCode));
    }else{
        if(response.dataLength != sizeof(uint64_t)){
            error = ERROR_INVALID_CONTENT_LENGTH;
        }else{            
            error = returnCode;
        }
    }
    
label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }

    return ERROR(error);
}

ERROR_CODE herder_addEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory,  char* filePath, const uint_fast64_t filePathLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    EpisodeInfo* episodeInfo = alloca(sizeof(*episodeInfo));
    if((error = mediaLibrary_initEpisodeInfo_(episodeInfo, filePath, filePathLength)) != ERROR_NO_ERROR){
        goto label_freeEpisodeInfo;
    }
    
    if((error = herder_extractShowInfo(remoteHost, remotePort, episodeInfo)) != ERROR_NO_ERROR){
        goto label_freeEpisodeInfo;
    }

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/add";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    // ShowName.
    util_uint64ToByteArray(request.data + request.dataLength, episodeInfo->showNameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, episodeInfo->showName, episodeInfo->showNameLength);
    request.dataLength += episodeInfo->showNameLength;

    // EpisodeName.
    util_uint64ToByteArray(request.data + request.dataLength, episodeInfo->nameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, episodeInfo->name, episodeInfo->nameLength);
    request.dataLength += episodeInfo->nameLength;

    // Season.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->season);
    request.dataLength += sizeof(uint16_t);

    // Episode.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->episode);
    request.dataLength += sizeof(uint16_t);   

    // FileExtension.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->fileExtensionLength);
    request.dataLength += sizeof(uint16_t);   

    memcpy(request.data + request.dataLength, episodeInfo->fileExtension, episodeInfo->fileExtensionLength);
    request.dataLength += episodeInfo->showNameLength;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }

    http_closeConnection(socketFD);

    uint_fast64_t readOffset = 0;
    
    error = util_byteArrayTo_uint64(response.data + readOffset);
    readOffset += sizeof(uint64_t);

    if(error != ERROR_NO_ERROR){
        if(error == ERROR_NAME_MISSMATCH || ERROR_DUPLICATE_ENTRY){
            UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add Season:%02" PRIdFAST16 " Episode:%02" PRIdFAST16 " of '%s' to the library, an entry for this episode %s. [%s]", episodeInfo->season, episodeInfo->episode, episodeInfo->showName, error == ERROR_NAME_MISSMATCH ? "with a different name is already present" : "is already present.", util_toErrorString(error));
        }
        
        goto label_freeRequest;
    }

    const uint_fast64_t fileExtensionOffset = util_findLast(filePath, filePathLength, '.');
    const uint_fast64_t fileExtensionLength = filePathLength - fileExtensionOffset;
    const char* fileExtension = filePath + (filePathLength - fileExtensionLength);

    char* path;
    uint_fast64_t pathLength;
    HERDER_CONSTRUCT_FILE_PATH(&path, &pathLength, episodeInfo);

    const uint_fast64_t fileDstLength = (libraryDirectory->entry->length - 1) + pathLength + fileExtensionLength;

    char* fileDst = alloca(sizeof(*fileDst) * (fileDstLength + 1));
    uint_fast64_t writeOffset = 0;

    strncpy(fileDst + writeOffset, (char*) libraryDirectory->buffer, libraryDirectory->entry->length - 1);
    writeOffset += libraryDirectory->entry->length - 1;

    strncpy(fileDst + writeOffset, path, pathLength);
    writeOffset += pathLength;

    strncpy(fileDst + writeOffset, fileExtension, fileExtensionLength);
    writeOffset += fileExtensionLength;
    fileDst[writeOffset] = '\0';

    if((error = util_createAllDirectories(fileDst, fileDstLength)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    if((error = util_fileCopy(filePath, fileDst)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    if(util_deleteFile(filePath) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    // TODO: Inform server that something went wrong and stuff should be removed from the library in case of failed file copy. (jan - 2018.11.28)

label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }
    
label_freeEpisodeInfo:
    mediaLibrary_freeEpisodeInfo(episodeInfo);

    return ERROR(error);
}

ERROR_CODE herder_extractShowInfo(Property* remoteHost, Property* remotePort, EpisodeInfo* episodeInfo){
    ERROR_CODE error;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const uint_fast64_t fileNameOffset = util_findLast(episodeInfo->path, episodeInfo->pathLength, '/') + 1;

    const char* fileName = episodeInfo->path + fileNameOffset;
    const uint_fast64_t fileNameLength = episodeInfo->pathLength - fileNameOffset;

    const char url[] = "/extractShowInfo";

    if((error = util_getFileExtension(&(episodeInfo->fileExtension), episodeInfo->path, episodeInfo->pathLength)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    episodeInfo->fileExtensionLength = strlen(episodeInfo->fileExtension);

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    util_uint64ToByteArray(request.data + request.dataLength, fileNameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, fileName, fileNameLength);
    request.dataLength += fileNameLength;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }

    http_closeConnection(socketFD);

    if(response.statusCode == _200_OK){
        uint_fast64_t readOffset = 0;

        uint_fast64_t returnCode = util_byteArrayTo_uint64(response.data + readOffset);
        readOffset += sizeof(uint64_t);

        if(returnCode != ERROR_NO_ERROR && returnCode != ERROR_INCOMPLETE){
            error = returnCode;
        }else{
            episodeInfo->showNameLength = util_byteArrayTo_uint64(response.data + readOffset);
            readOffset += sizeof(uint64_t);

            if(episodeInfo->showNameLength != 0){
                 episodeInfo->showName = malloc(sizeof(*episodeInfo->showName) * (episodeInfo->showNameLength) + 1);
                if(episodeInfo->showName == NULL){
                    return ERROR(ERROR_OUT_OF_MEMORY);
                }

                strncpy(episodeInfo->showName, (char*) (response.data + readOffset), episodeInfo->showNameLength);
                episodeInfo->showName[episodeInfo->showNameLength] = '\0';

                readOffset += episodeInfo->showNameLength;
            }else{
                episodeInfo->showName = NULL;
            }            

            episodeInfo->nameLength = util_byteArrayTo_uint64(response.data + readOffset);
            readOffset += sizeof(uint64_t);

            if(episodeInfo->nameLength != 0){
                episodeInfo->name = malloc(sizeof(*episodeInfo->name) * (episodeInfo->nameLength + 1));

                 if(episodeInfo->name == NULL){
                    return ERROR(ERROR_OUT_OF_MEMORY);
                }

                strncpy(episodeInfo->name, (char*) (response.data + readOffset), episodeInfo->nameLength);
                readOffset += episodeInfo->nameLength;
            }else{
                episodeInfo->name = NULL;
            }

            episodeInfo->season = util_byteArrayTo_uint16(response.data + readOffset);
            readOffset += sizeof(uint16_t);

            episodeInfo->episode = util_byteArrayTo_uint16(response.data + readOffset);
            readOffset += sizeof(uint16_t);
        }
    }else{
        error = ERROR_INVALID_STATUS_CODE;

        UTIL_LOG_CONSOLE_(LOG_ERR, "Server_status:'%s'.", http_getStatusMsg(response.statusCode));
    }

label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }

    return ERROR(error);
}

ERROR_CODE herder_add(Property* remoteHost, Property* remotePort, Property* libraryDirectory, EpisodeInfo* episodeInfo){
    ERROR_CODE error;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/add";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    // ShowName.
    util_uint64ToByteArray(request.data + request.dataLength, episodeInfo->showNameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, episodeInfo->showName, episodeInfo->showNameLength);
    request.dataLength += episodeInfo->showNameLength;

    // EpisodeName.
    util_uint64ToByteArray(request.data + request.dataLength, episodeInfo->nameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, episodeInfo->name, episodeInfo->nameLength);
    request.dataLength += episodeInfo->nameLength;

    // Season.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->season);
    request.dataLength += sizeof(uint16_t);

    // Episode.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->episode);
    request.dataLength += sizeof(uint16_t);   

    // FileExtension.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->fileExtensionLength);
    request.dataLength += sizeof(uint16_t);   

    memcpy(request.data + request.dataLength, episodeInfo->fileExtension, episodeInfo->fileExtensionLength);
    request.dataLength += episodeInfo->showNameLength;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }

    http_closeConnection(socketFD);

    uint_fast64_t readOffset = 0;
    
    error = util_byteArrayTo_uint64(response.data + readOffset);
    readOffset += sizeof(uint64_t);

    if(error != ERROR_NO_ERROR){
        if(error == ERROR_NAME_MISSMATCH || ERROR_DUPLICATE_ENTRY){
            UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add Season:%02" PRIdFAST16 " Episode:%02" PRIdFAST16 " of '%s' to the library, an entry for this episode %s. [%s]", episodeInfo->season, episodeInfo->episode, episodeInfo->showName, error == ERROR_NAME_MISSMATCH ? "with a different name is already present" : "is already present.", util_toErrorString(error));
        }
        
        goto label_freeRequest;
    }

    char* path;
    uint_fast64_t pathLength;
    HERDER_CONSTRUCT_FILE_PATH(&path, &pathLength, episodeInfo);

    const uint_fast64_t fileDstLength = (libraryDirectory->entry->length - 1) + pathLength + episodeInfo->fileExtensionLength;

    char* fileDst = alloca(sizeof(*fileDst) * (fileDstLength + 1));
    uint_fast64_t writeOffset = 0;

    strncpy(fileDst + writeOffset, (char*) libraryDirectory->buffer, libraryDirectory->entry->length - 1);
    writeOffset += libraryDirectory->entry->length - 1;

    strncpy(fileDst + writeOffset, path, pathLength);
    writeOffset += pathLength;

    strncpy(fileDst + writeOffset, episodeInfo->fileExtension, episodeInfo->fileExtensionLength);
    writeOffset += episodeInfo->fileExtensionLength;
    fileDst[writeOffset] = '\0';

    if((error = util_createAllDirectories(fileDst, fileDstLength)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    if((error = util_fileCopy(episodeInfo->path, fileDst)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    if(util_deleteFile(episodeInfo->path) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }
    
    // TODO: Inform server that something went wrong and stuff should be removed from the library in case of failed file copy. (jan - 2018.11.28)

label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }
    
    return ERROR(error);
}

ERROR_CODE herder_renameEpisode(Property* remoteHost, Property* remotePort){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* userInput;
    int_fast64_t userInputLength;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    LinkedList shows;
    if((error = herder_pullShowList(&shows, host, port)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if(shows.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO, "Library is empty.");
    }else{
        LinkedListIterator it;
        linkedList_initIterator(&it, &shows);

        uint_fast64_t i = 0;
        while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
            char* show = LINKED_LIST_ITERATOR_NEXT(&it);
            
            UTIL_LOG_CONSOLE_(LOG_INFO, "%02" PRIuFAST64 ":'%s'.", i, show);

            i++;
        }
    }

label_readUserInput:
    UTIL_LOG_CONSOLE(LOG_INFO, "\nPlease select a show.");

    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    char* selectedShow = NULL;

    int_fast64_t value;
    if(util_stringToInt(userInput, &value) != ERROR_NO_ERROR){
        selectedShow = userInput;
    }

    if(value + 1 > (int_fast64_t) shows.length || value < 0){
        free(userInput);

        UTIL_LOG_CONSOLE(LOG_ERR, "Invalid selection.");

        goto label_readUserInput;
    }

    LinkedListIterator it;
    linkedList_initIterator(&it, &shows);

    int_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        char* show = LINKED_LIST_ITERATOR_NEXT(&it);

        if(value == i++ && selectedShow == NULL){
            selectedShow = show;

            break;
        }

        if(strncmp(userInput, show, userInputLength + 1) == 0){
            selectedShow = show;

            break;
        }
    }

    if(selectedShow == NULL || selectedShow == userInput){
        free(userInput);

        UTIL_LOG_CONSOLE(LOG_ERR, "Invalid selection.");

        goto label_readUserInput;
    }

    const uint_fast64_t showNameLength = strlen(selectedShow);
    char* showName = alloca(sizeof(*showName) * (showNameLength + 1));
    strncpy(showName, selectedShow, showNameLength + 1);

    linkedList_initIterator(&it, &shows);
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        char* show = LINKED_LIST_ITERATOR_NEXT(&it);
        
        free(show);
    }

    free(userInput);

    linkedList_free(&shows);

    // Select season.
    Show* show = alloca(sizeof(*show));
    if((error = medialibrary_initShow(show, showName, showNameLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = herder_pullShowInfo(remoteHost, remotePort, show)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Seasons:");

    Season** seasons = alloca(sizeof(*seasons) * show->seasons.length);
    mediaLibrary_sortSeasons(&seasons, &show->seasons);

    uint_fast64_t j;
    for(j = 0; j < show->seasons.length; j++){
        UTIL_LOG_CONSOLE_(LOG_INFO, "\t%" PRIuFAST16 ".", seasons[j]->number);
    }
    
    UTIL_LOG_CONSOLE(LOG_INFO, "\nPlease select a season.");

label_selectSeason:
    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    if(util_stringToInt(userInput, &value) != ERROR_NO_ERROR){
        free(userInput);

        goto label_selectSeason;
    }

    if(value < 1){
        free(userInput);

        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid selection.");

        goto label_selectSeason;
    }

    Season* selectedSeason = NULL;
    for(j = 0; j < show->seasons.length; j++){
       if((int_fast64_t) (seasons[j]->number) == value){
           selectedSeason = seasons[j];

           break;
       }
    }

    free(userInput);

    if(selectedSeason == NULL){
        goto label_selectSeason;
    }

    UTIL_LOG_CONSOLE_(LOG_INFO, "Season:'%02" PRIuFAST16 "'.", selectedSeason->number);

    // Select Episode.    
    Episode** episodes = alloca(sizeof(*episodes) * selectedSeason->episodes.length);
    mediaLibrary_sortEpisodes(&episodes, &selectedSeason->episodes);

    for(j = 0; j < selectedSeason->episodes.length; j++){
        UTIL_LOG_CONSOLE_(LOG_INFO, "\t%" PRIuFAST16 ": '%s'", episodes[j]->number, episodes[j]->name);
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "\nPlease select an episode.");

label_selectEpisode:
    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    if(util_stringToInt(userInput, &value) != ERROR_NO_ERROR){
        free(userInput);

        goto label_selectEpisode;
    }

    if(value < 1){
        free(userInput);

        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid selection.");

        goto label_selectEpisode;
    }

    Episode* selectedEpisode = NULL;
    for(j = 0; j < selectedSeason->episodes.length; j++){
       if((int_fast64_t) (episodes[j]->number) == value){
           selectedEpisode = episodes[j];

           break;
       }
    }

    free(userInput);

    if(selectedEpisode == NULL){
        goto label_selectSeason;
    }

label_enterNewName:
    UTIL_LOG_CONSOLE(LOG_INFO, "\nPlease enter a new episode name.");

    char* newEpisodeName;
    int_fast64_t newEpisodeNameLength;

    if((error = util_readUserInput(&newEpisodeName, &newEpisodeNameLength)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    UTIL_LOG_CONSOLE_(LOG_INFO, "\nShow:'%s', s%02" PRIuFAST16 "e%02" PRIuFAST16 " - '%s'", showName, selectedSeason->number, selectedEpisode->number, userInput);

    UTIL_LOG_CONSOLE_(LOG_INFO, "\nRename episode to '%s'? Yes/No.", userInput);

    util_toLowerChase(userInput);

    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        free(userInput);

        free(newEpisodeName);

        goto label_freeShow;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Input:'%s' [%" PRIdFAST64 "]", userInput, userInputLength);

    if(strncmp(userInput, "no", 3) == 0 || strncmp(userInput, "n", 2) == 0){
        free(userInput);
        free(newEpisodeName);

        UTIL_LOG_CONSOLE(LOG_DEBUG, "NO");

        goto label_enterNewName;
    }else{
        if(userInputLength != 0 && strncmp(userInput, "yes", 4) != 0 && strncmp(userInput, "y", 2) != 0){
            UTIL_LOG_CONSOLE(LOG_DEBUG, "Invalid !!!");

            goto label_enterNewName;
        }else{
            UTIL_LOG_CONSOLE(LOG_DEBUG, "yes");
        }
    }

    // Send rename packet.
    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/updateShowInfo";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    // 1. Type:[ShowName, SeasonNumber, EpisodeInfo],
    // 2. Old value/s.
    // 3. New value/s.

    request.data[request.dataLength] = UPDATE_INFO_PACKET_TYPE_UPDATE_EPISODE_INFO;
    request.dataLength += sizeof(uint8_t);   

    // Show_Name.
    util_uint64ToByteArray(request.data + request.dataLength, show->nameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, show->name, show->nameLength + 1);
    request.dataLength += show->nameLength + 1;

    // Season_Number.
    util_uint16ToByteArray(request.data + request.dataLength, selectedSeason->number);
    request.dataLength += sizeof(uint16_t);   

    // Episode_Number.
    util_uint16ToByteArray(request.data + request.dataLength, selectedEpisode->number);
    request.dataLength += sizeof(uint16_t);   

    // New values

    // Season_Number.
    util_uint16ToByteArray(request.data + request.dataLength, selectedSeason->number);
    request.dataLength += sizeof(uint16_t);   

    // Episode_Number.
    util_uint16ToByteArray(request.data + request.dataLength, selectedEpisode->number);
    request.dataLength += sizeof(uint16_t);   

    // Episode_Name.
    util_uint64ToByteArray(request.data + request.dataLength, newEpisodeNameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, newEpisodeName, newEpisodeNameLength + 1);
    request.dataLength += newEpisodeNameLength + 1;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }

    http_closeConnection(socketFD);

    if(response.statusCode != _200_OK){
        error = ERROR_INVALID_STATUS_CODE;

        UTIL_LOG_CONSOLE_(LOG_ERR, "Server_status:'%s'.", http_getStatusMsg(response.statusCode));
    }else{
        if(response.dataLength != sizeof(uint64_t)){
            error = ERROR_INVALID_CONTENT_LENGTH;
        }else{
            const uint_fast64_t returnCode = util_byteArrayTo_uint64(response.data);
            if(returnCode != ERROR_NO_ERROR){
               error = returnCode;
            }
        }
    }

label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }

    free(userInput);

    // Free show.1
    LinkedListIterator seasonIterator;
label_freeShow:
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

label_return:
    return ERROR(error);
}

/*  printf("#define HASH_MP4 %d\n", util_hashString(".mp4"));
    printf("#define HASH_MKV %d\n", util_hashString(".mkv"));
    printf("#define HASH_AVI %d\n", util_hashString(".avi")); */
local inline bool herder_walkDirectoryAcceptFunction(const char* s, const uint_fast64_t length){
    #define HASH_MP4 -1839843325
    #define HASH_MKV -1840171254
    #define HASH_AVI -1938587738

    switch(util_hashString(s, length)){
        case HASH_MP4:
        case HASH_MKV:
        case HASH_AVI:{
            return true;
        }

        default:
            return false;
    }

    #undef HASH_MP4
    #undef HASH_MKV
    #undef HASH_AVI
}

ERROR_CODE herder_pullShowInfo(Property* remoteHost, Property* remotePort, Show* show){
    ERROR_CODE error;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/showInfo";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    util_uint64ToByteArray(request.data + request.dataLength, show->nameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, show->name, show->nameLength + 1);
    request.dataLength += show->nameLength + 1;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }

    http_closeConnection(socketFD);

    if(response.statusCode == _200_OK){
        uint_fast64_t readOffset = 0;

        error = util_byteArrayTo_uint64(response.data);
        readOffset += sizeof(uint64_t);

        if(error != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        // Num_Seasons.
        uint_fast64_t numSeasons = util_byteArrayTo_uint64(response.data + readOffset);
        readOffset += sizeof(uint64_t);

        if(numSeasons != 0){                
            for( ; numSeasons > 0; numSeasons--){
                // Season_Number.
                const uint_fast16_t seasonNumber = util_byteArrayTo_uint16(response.data + readOffset);
                readOffset += sizeof(uint16_t);

                Season* season;
                season = malloc(sizeof(*season));
                if((error = medialibrary_initSeason(season, seasonNumber)) != ERROR_NO_ERROR){
                    goto label_freeRequest;
                }
                
                if((error = linkedList_add(&show->seasons, season)) != ERROR_NO_ERROR){
                    goto label_freeRequest;
                }

                // Num_Episodes.
                uint_fast64_t numEpisodes = util_byteArrayTo_uint64(response.data + readOffset);
                readOffset += sizeof(uint64_t);

                for( ; numEpisodes > 0; numEpisodes--){
                    // Episode_Number.
                    const uint_fast16_t episodeNumber = util_byteArrayTo_uint16(response.data + readOffset);
                    readOffset += sizeof(uint16_t);

                    // Episode_NameLength.
                    uint_fast64_t nameLength = util_byteArrayTo_uint64(response.data + readOffset);
                    readOffset += sizeof(uint64_t);

                    // Episode_Name.
                    char* name = alloca(sizeof(*name) * (nameLength + 1));
                    memcpy(name, response.data + readOffset, nameLength + 1);
                    readOffset += nameLength + 1;

                    Episode* episode;
                    episode = malloc(sizeof(*episode));
                    if((medialibrary_initEpisode(episode, episodeNumber, name, nameLength, "", 0)) != ERROR_NO_ERROR){
                        goto label_freeRequest;
                    }

                    if((error = linkedList_add(&season->episodes, episode)) != ERROR_NO_ERROR){
                        goto label_freeRequest;
                    }
                }
            }
        }
    }else{
        error = ERROR_INVALID_STATUS_CODE;

        UTIL_LOG_CONSOLE_(LOG_ERR, "Server_status:'%s'.", http_getStatusMsg(response.statusCode));
    }

label_freeRequest:
    http_freeHTTP_Request(&request);

label_freeResponse:
    http_freeHTTP_Response(&response);

label_unMap:
    if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
    }

    return ERROR(error);
}

ERROR_CODE herder_walkDirectory(LinkedList* list, const char* directory){
    ERROR_CODE error = ERROR_NO_ERROR;

    DIR* currentDirectory = opendir(directory);
    if(currentDirectory == NULL){
        error = ERROR_FAILED_TO_OPEN_DIRECTORY;

        goto label_closeDir;
    }

    struct dirent* directoryEntry;

    const uint_fast64_t directoryLength = strlen(directory);

    while((directoryEntry = readdir(currentDirectory)) != NULL){
        // Avoid reentering current and parent directory.
        const uint_fast64_t currentEntryLength = strlen(directoryEntry->d_name);
        if(strncmp(directoryEntry->d_name, ".", currentEntryLength) == 0 || strncmp(directoryEntry->d_name, "..", currentEntryLength) == 0){
            continue;
        }

        if(directoryEntry->d_type == DT_DIR){                      
            const uint_fast64_t directoryPathLength = directoryLength + currentEntryLength + 1;  

            char* directoryPath;
            directoryPath = alloca(sizeof(*directoryPath) * (directoryPathLength + 1));
            strncpy(directoryPath, directory, directoryLength + 1);     

            util_append(directoryPath + directoryLength, directoryPathLength - 1 - directoryLength, directoryEntry->d_name, currentEntryLength);        
            directoryPath[directoryPathLength - 1] = '/';
            directoryPath[directoryPathLength] = '\0';
          
            if((error = herder_walkDirectory(list, directoryPath)) != ERROR_NO_ERROR){
                goto label_closeDir;
            }
        }else{                                
            const uint_fast64_t pathLength = directoryLength + currentEntryLength; 

            char* path;
            path = malloc(sizeof(*path) * (pathLength + 1));
            strncpy(path, directory, directoryLength + 1);     

            util_append(path + directoryLength, pathLength - directoryLength, directoryEntry->d_name, currentEntryLength);        
            path[pathLength] = '\0';

            DirectoryEntry* dirEnt = malloc(sizeof(*dirEnt));
            dirEnt->path = path;                  
            dirEnt->pathLength = pathLength;
            dirEnt->fileName = path + (pathLength - currentEntryLength);
            dirEnt->fileNameLength = currentEntryLength;

            if((error = linkedList_add(list, dirEnt)) !=ERROR_NO_ERROR){
                goto label_closeDir;
            }
        }
    }

label_closeDir:
    closedir(currentDirectory);
        
    return ERROR(error);
}

#undef HTTP_PROCESSING_BUFFER_SIZE