#include "herder.h"

#include "util.c"
#include "linkedList.c"
#include "http.c"
#include "argumentParser.c"
#include "propertyFile.c"
#include "mediaLibrary.c"

#define HTTP_PROCESSING_BUFFER_SIZE 8192 //16384

ERROR_CODE herder_pullShowList(LinkedList* shows, Property* remoteHost, Property* remotePort){
    ERROR_CODE error = ERROR_NO_ERROR;
    
    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

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
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

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
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

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

ERROR_CODE herder_addEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory, EpisodeInfo* episodeInfo){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

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
    HERDER_CONSTRUCT_RELATIVE_FILE_PATH(&path, &pathLength, episodeInfo);

    const uint_fast64_t fileDstLength = (libraryDirectory->entry->length - 1) + pathLength + episodeInfo->fileExtensionLength + 1;

    char* fileDst = alloca(sizeof(*fileDst) * (fileDstLength + 1));
    uint_fast64_t writeOffset = 0;

    strncpy(fileDst + writeOffset, (char*) libraryDirectory->buffer, libraryDirectory->entry->length - 1);
    writeOffset += libraryDirectory->entry->length - 1;

    strncpy(fileDst + writeOffset, path, pathLength);
    writeOffset += pathLength;

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

ERROR_CODE herder_removeEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory, EpisodeInfo* episodeInfo){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/removeEpisode";

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

    // Season.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->season);
    request.dataLength += sizeof(uint16_t);

    // Episode.
    util_uint16ToByteArray(request.data + request.dataLength, episodeInfo->episode);
    request.dataLength += sizeof(uint16_t);

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
        UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to remove Season:%02" PRIdFAST16 " Episode:%02" PRIdFAST16 " of '%s' from the library. [%s]", episodeInfo->season, episodeInfo->episode, episodeInfo->showName, util_toErrorString(error));

        goto label_freeRequest;
    }

    char* relativePath;
    uint_fast64_t relativePathLength;
    HERDER_CONSTRUCT_RELATIVE_FILE_PATH(&relativePath, &relativePathLength, episodeInfo);

    const uint_fast64_t absoluteFilePathLength = (libraryDirectory->entry->length - 1) + relativePathLength + 1;

    char* absoluteFilePath = alloca(sizeof(*absoluteFilePath) * (absoluteFilePathLength + 1));
    uint_fast64_t writeOffset = 0;

    strncpy(absoluteFilePath + writeOffset, (char*) libraryDirectory->buffer, libraryDirectory->entry->length - 1);
    writeOffset += libraryDirectory->entry->length - 1;

    strncpy(absoluteFilePath + writeOffset, relativePath, relativePathLength);
    writeOffset += relativePathLength;
    
    absoluteFilePath[writeOffset] = '\0';

    // Remove file/episode from disk.
    if((error = util_deleteFile(absoluteFilePath)) != ERROR_NO_ERROR){
        UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to remove Season:%02" PRIdFAST16 " Episode:%02" PRIdFAST16 " of '%s' from the library. [%s]", episodeInfo->season, episodeInfo->episode, episodeInfo->showName, util_toErrorString(error));

        goto label_freeRequest;
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

ERROR_CODE herder_extractShowInfo(Property* remoteHost, Property* remotePort, EpisodeInfo* episodeInfo){
    ERROR_CODE error;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

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

    if((error = util_getFileExtension(&episodeInfo->fileExtension,(uint_fast64_t*) &episodeInfo->fileExtensionLength, episodeInfo->path, episodeInfo->pathLength)) != ERROR_NO_ERROR){
        if(error == ERROR_INVALID_STRING){
            error = ERROR_INVALID_FILE_EXTENSION;
        }

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
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

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
    HERDER_CONSTRUCT_RELATIVE_FILE_PATH(&path, &pathLength, episodeInfo);

    const uint_fast64_t fileDstLength = (libraryDirectory->entry->length - 1) + pathLength;

    char* fileDst = alloca(sizeof(*fileDst) * (fileDstLength + 1));
    uint_fast64_t writeOffset = 0;

    strncpy(fileDst + writeOffset, (char*) libraryDirectory->buffer, libraryDirectory->entry->length - 1);
    writeOffset += libraryDirectory->entry->length - 1;

    strncpy(fileDst + writeOffset, path, pathLength);
    writeOffset += pathLength;
    
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

ERROR_CODE herder_pullShowInfo(Property* remoteHost, Property* remotePort, Show* show){
    ERROR_CODE error;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

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

                    // TODO: Remove memcpy into the stack. We can just point into the HTTP_PROCESSING_BUFFER since we malloc and memcpy in 'mediaLibrary_initEpisode' anyways. (jan - 2020.02.10)

                    // Episode_Name.
                    char* name = alloca(sizeof(*name) * (nameLength + 1));
                    memcpy(name, response.data + readOffset, nameLength + 1);
                    readOffset += nameLength + 1;

                    // File_ExtensionLength.
                    uint_fast16_t fileExtensionLength = util_byteArrayTo_uint16(response.data + readOffset);
                    readOffset += sizeof(uint16_t);

                    // File_Extension.
                    char* fileExtension = alloca(sizeof(*fileExtension) * (fileExtensionLength + 1));
                    memcpy(fileExtension, response.data + readOffset, fileExtensionLength + 1);
                    readOffset += fileExtensionLength + 1;

                    Episode* episode;
                    if((episode = malloc(sizeof(*episode))) == NULL){
                        error = ERROR_OUT_OF_MEMORY;

                        goto label_freeRequest;
                    }

                    if((medialibrary_initEpisode(episode, episodeNumber, name, nameLength, fileExtension, fileExtensionLength)) != ERROR_NO_ERROR){
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

ERROR_CODE herder_renameEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory, EpisodeInfo* info, const char* newEpisodeName, const uint_fast64_t newEpisodeNameLength){
    ERROR_CODE error;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint16(remotePort->buffer);

    void* httpProcessingBuffer;
    if((error = util_blockAlloc(&httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    int_fast32_t socketFD;
    if((error = http_openConnection(&socketFD, host, port)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    const char url[] = "/renameEpisode";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    // Show_Name.
    util_uint64ToByteArray(request.data + request.dataLength, info->nameLength);
    request.dataLength += sizeof(uint64_t);

    memcpy(request.data + request.dataLength, info->showName, info->nameLength + 1);
    request.dataLength += info->nameLength + 1;

    // Season_Number.
    util_uint16ToByteArray(request.data + request.dataLength, info->season);
    request.dataLength += sizeof(uint16_t);

    // Episode_Number.
    util_uint16ToByteArray(request.data + request.dataLength, info->episode);
    request.dataLength += sizeof(uint16_t);

    // New Episode_Name.
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

    char* path;
    uint_fast64_t pathLength;
    HERDER_CONSTRUCT_RELATIVE_FILE_PATH(&path, &pathLength, info);

    const uint_fast64_t fileDstLength = (libraryDirectory->entry->length - 1) + pathLength + 1;

    char* filePath = alloca(sizeof(*filePath) * (fileDstLength + 1));
    uint_fast64_t writeOffset = 0;

    strncpy(filePath + writeOffset, (char*) libraryDirectory->buffer, libraryDirectory->entry->length - 1);
    writeOffset += libraryDirectory->entry->length - 1;

    strncpy(filePath + writeOffset, path, pathLength);
    writeOffset += pathLength;
    
    filePath[writeOffset] = '\0';

    char* fileDirectory = alloca(sizeof(*fileDirectory) * fileDstLength + 1);
    if((error = util_getFileDirectory(fileDirectory, filePath, fileDstLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    char* oldFileName = util_getFileName(filePath, fileDstLength);

    // Build new fileName.
    const uint_fast64_t newFileNameLength = info->showNameLength + 2/*_*/ + (2 * UTIL_UINT16_STRING_LENGTH) + 3 /*'e'/'s'/'.'*/ + newEpisodeNameLength + info->fileExtensionLength;
    char* newFileName = alloca(sizeof(*newFileName) * (newFileNameLength + 1));
    
    snprintf(newFileName, newFileNameLength, "%s_s%02" PRIdFAST16 "e%02" PRIdFAST16 "_%s.%s", info->showName, info->season, info->episode, newEpisodeName, info->fileExtension);

    util_replaceAllChars(newFileName, ' ', '_');

    // Rename file on disk.
    if((error = util_renameFileRelative(fileDirectory, oldFileName, newFileName)) != ERROR_NO_ERROR){
        goto label_return;
    }

label_return:
    return ERROR(error);
}

ERROR_CODE herder_convertToMP3(Property* remoteHost, Property* remotePort, Property* libraryDirectory, const char* showName, const uint_fast64_t showNameLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    #define HERDER_STRING_CONSTANT_AUDIO "audio/"

    const uint_fast64_t audioStringLength = strlen(HERDER_STRING_CONSTANT_AUDIO);
    const uint_fast64_t audioDirectoryLength = (libraryDirectory->entry->length - 1) + audioStringLength;

    char* audioDirectory = alloca(sizeof(*audioDirectory) * (audioDirectoryLength + 1));
    memcpy(audioDirectory, libraryDirectory->buffer, libraryDirectory->entry->length - 1);
    strncpy(audioDirectory + libraryDirectory->entry->length - 1, HERDER_STRING_CONSTANT_AUDIO, audioStringLength + 1);

    #undef HERDER_STRING_CONSTANT_AUDIO

    if(!util_directoryExists(audioDirectory)){
        if((error = util_createDirectory(audioDirectory)) != ERROR_NO_ERROR){
            goto label_return;
        }

        UTIL_LOG_INFO_("Created directory:'%s'.", audioDirectory);
    }

    const uint_fast64_t showDirectoryLength = audioDirectoryLength + showNameLength + 1/*'/'*/;

    char* _showName = alloca(sizeof(*_showName) * showNameLength + 1);
    strncpy(_showName, showName, showNameLength + 1);
    util_replaceAllChars(_showName, ' ', '_');

    char* showDirectory = alloca(sizeof(*showDirectory) * (showDirectoryLength + 1));
    strncpy(showDirectory, audioDirectory, audioDirectoryLength);
    strncpy(showDirectory + audioDirectoryLength, _showName, showNameLength);
    showDirectory[audioDirectoryLength + showNameLength] = '/';
    showDirectory[audioDirectoryLength + showNameLength + 1] = '\0';

    if(!util_directoryExists(showDirectory)){
        if((error = util_createDirectory(showDirectory)) != ERROR_NO_ERROR){
            goto label_return;
        }

        UTIL_LOG_INFO_("Created directory:'%s'.", showDirectory);
    }

    Show show;
    if((error = medialibrary_initShow(&show, showName, showNameLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = herder_pullShowInfo(remoteHost, remotePort, &show)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    LinkedListIterator seasonIterator;
    linkedList_initIterator(&seasonIterator, &show.seasons);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&seasonIterator)){
        Season* season = LINKED_LIST_ITERATOR_NEXT(&seasonIterator);

        uint_fast64_t seasonDirectoryLength = showDirectoryLength + (showNameLength + 3/*' - '*/ + 7/*'Season_'*/ + UTIL_UINT16_STRING_LENGTH + 1/*'/'*/);

        char* seasonDirectory = alloca(sizeof(*seasonDirectory) * (seasonDirectoryLength + 1));

        seasonDirectoryLength = snprintf(seasonDirectory, seasonDirectoryLength + 1, "%s%s - Season_%02" PRIdFAST16 "/", showDirectory, _showName, season->number);

        if(!util_directoryExists(seasonDirectory)){
            if((error = util_createDirectory(seasonDirectory)) != ERROR_NO_ERROR){
                goto label_return;
            }

            UTIL_LOG_INFO_("Created directory:'%s'.", seasonDirectory);
        }

        LinkedListIterator episodeIterator;
        linkedList_initIterator(&episodeIterator, &season->episodes);

        while(LINKED_LIST_ITERATOR_HAS_NEXT(&episodeIterator)){
            Episode* episode = LINKED_LIST_ITERATOR_NEXT(&episodeIterator);

            uint_fast64_t episodePathLength = seasonDirectoryLength + showNameLength + episode->nameLength + (2 * UTIL_UINT16_STRING_LENGTH + 2/*'_s'*/ + 1/*'e'*/ + 1/*"_"*/ + 4/*'.mp3'*/);

            char* episodePath = alloca(sizeof(*episodePath) * (episodePathLength + 1));

            util_replaceAllChars(episode->name, ' ', '_');

            episodePathLength = snprintf(episodePath, episodePathLength + 1, "%s%s_s%02" PRIdFAST16 "e%02" 
            PRIdFAST16 "_%s%s", seasonDirectory, _showName, season->number, episode->number, episode->name, ".mp3");

            if(!util_fileExists(episodePath)){
                EpisodeInfo episodeInfo;
                mediaLibrary_fillEpisodeInfo(&episodeInfo, &show, season, episode);

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

                uint_fast64_t ffmpegCallStringLength = 10/*'ffmpeg -i '*/ + episodePathLength + 51/*'-q:a 2 -loglevel error -stats -af "volume=4dB" -vn '*/ + absoluteFilePathLength + 4/*'""'*/;

                char* ffmpegCallString = alloca(sizeof(*ffmpegCallString) * (ffmpegCallStringLength + 1));
                ffmpegCallStringLength = snprintf(ffmpegCallString, ffmpegCallStringLength + 1, "ffmpeg -i \"%s\" -q:a 2 -loglevel error -stats -af \"volume=4dB\" -vn \"%s\"", absoluteFilePath, episodePath);

                // const uint_fast64_t episodeOffset = util_findLast(relativePath, relativePathLength, '/');

                const int returnValue = system(ffmpegCallString);

                if(returnValue == -1){
                    UTIL_LOG_ERROR(strerror(error));
                }else{
                    if(returnValue != 0){
                        if(util_fileExists(episodePath)){
                            error = util_deleteFile(episodePath);
                        }
                        goto label_freeShow;
                    }
                }

                // https://trac.ffmpeg.org/wiki/Encode/MP3
                // -i Input.
                // -b:a 192k Desired bitrate. (Constant encoding)
                // -q:a 2 ~190kbit/s. -q:a == -qscale:a.
                // -af voulume=4dB Increase voulume.
                // -vn No video output.
                // -ar 44100 SamplingRate.
                // -ac 2 Numnber of audio chanels.
                // -loglevel error -stats
            }
        }
    }

label_freeShow:
    mediaLibrary_freeShow(&show);

label_return:
    return ERROR(error);
}