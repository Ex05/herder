#ifndef SERVER_C
#define SERVER_C

// POSIX Version ¢2008
#define _XOPEN_SOURCE 700

// For mmap flags that are not in the POSIX-Standard.
#define _GNU_SOURCE

#include "server.h"

#include "que.c"
#include "threadPool.c"
#include "http.c"
#include "mediaLibrary.c"
#include "cache.c"
#include "propertyFile.c"
#include "argumentParser.c"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_DAEMON_NAME "herderServerDaemon"

#define SERVER_WEB_DIRECTORY "www/"

#define SERVER_USAGE_ARGUMENT_HELP "-?, -h, --help"
#define SERVER_USAGE_ARGUMENT_SET_SERVER_ROOT_DIRECTORY "--setServerRootDirectory <path>"
#define SERVER_USAGE_ARGUMENT_SET_SERVER_EXTERNAL_PORT "--setServerExternalPort <port>"
#define SERVER_USAGE_ARGUMENT_SHOW_SETTINGS "--showSettings <path>"

#define PROPERTY_FILE_NAME "server_settings"
#define SERVER_PROPERTY_SERVER_ROOT_DIRECTORY_NAME "serverRootDirectory"
#define PROPERTY_SERVER_EXTERNAL_PORT_NAME "serverExternalPort"

local void server_initClient(Client*, HerderServer*);

local ERROR_CODE server_initContext(Context*, const char*, const uint_fast64_t, ContextHandler*);

local void server_freeContext(Context*);

local ERROR_CODE server_getFile(HerderServer*, CacheObject**, char*, const uint_fast64_t);

local void server_daemonize(const char* workingDirectory);

local HerderServer server;
 
SERVER_CONTEXT_HANDLER(server_defaultContextHandler){
    uint_fast64_t fileLocationLength;
    char* fileLocation;
    if(request->urlLength == 1 && request->requestURL[0] == '/'){
        fileLocationLength = 11/*/index.html*/;
        fileLocation = alloca(sizeof(*fileLocation) * (11/*/index.html*/ + 1));

        memcpy(fileLocation, "/index.html", 11);
        fileLocation[fileLocationLength] = '\0';
    }else{
        fileLocationLength = request->urlLength;
        
        fileLocation = alloca(sizeof(*fileLocation) * (fileLocationLength + 1));
    memcpy(fileLocation, request->requestURL, fileLocationLength);
        fileLocation[fileLocationLength] = '\0';
    }

    CacheObject* cacheObject;
    ERROR_CODE error;
    if((error = server_getFile(server, &cacheObject, fileLocation, fileLocationLength)) != ERROR_NO_ERROR){
        if(error == ERROR_FAILED_TO_RETRIEV_FILE_INFO){
            error = server_constructErrorPage(server, request, response, _404_NOT_FOUND);

            return ERROR(error);
        }else{
            error = server_constructErrorPage(server, request, response, _500_INTERNAL_SERVER_ERROR);
            
            return ERROR(error);   
        }
    }

    response->cacheObject = cacheObject;
    response->dataLength += cacheObject->size;
    response->staticContent = true;

    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;

    response->contentType = http_getContentType(cacheObject->fileLocation + cacheObject->fileExtensionOffset, cacheObject->fileLocationLength - cacheObject->fileExtensionOffset);

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);
    
    return ERROR(ERROR_NO_ERROR);
}

local THREAD_POOL_RUNNABLE_(server_run, Job, job){
    Client* client = job->data;
    void* buffer = job->buffer;

    char ip[INET_ADDRSTRLEN];
        
    if(client->socket.sin_family == AF_INET){
        inet_ntop(AF_INET, &(client->socket.sin_addr), ip, INET_ADDRSTRLEN);    
    }else{
        memset(ip, 0, INET_ADDRSTRLEN);
    }

    HTTP_Request request;
    http_initRequest_(&request, buffer, HTTP_PROCESSING_BUFFER_SIZE, 0);

    if(http_receiveRequest(&request, client->sockFD) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to receive HTTP request.");

        goto label_failedToRecevieHTTP_Request;
    }  

    HTTP_Response response;
    http_initResponse(&response, buffer, HTTP_PROCESSING_BUFFER_SIZE);

    ContextHandler* contextHandler;
    // TODO:(jan) Check return value and send correct error page (ERROR_ENTRY_NOT_FOUND vs ERORR_INVALID_URL).
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if(server_getContext(client->server, &contextHandler, &request) != ERROR_NO_ERROR){
        server_constructErrorPage(client->server, &request, &response, _400_BAD_REQUEST);
    }

    ERROR_CODE error;
    if(contextHandler == NULL){
        server_constructErrorPage(client->server, &request, &response, _401_UNAUTHORIZED);
    }else{
        if((error = contextHandler(client->server, &request, &response)) != ERROR_NO_ERROR){
            UTIL_LOG_ERROR(util_toErrorString(error));
        }
    }

    if((error = http_sendResponse(&response, client->sockFD)) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR_("Failed to send HTTP response. %d", error);
    }  

    http_freeHTTP_Response(&response);

label_failedToRecevieHTTP_Request:
    close(client->sockFD);

    http_freeHTTP_Request(&request);

    free(client);

    return NULL;
}

local void server_deamonSignalHandler(int signal, siginfo_t* info, void* context){
	switch(signal){
        case SIGHUP:{
            UTIL_LOG_INFO("SIGHUP catched.");
             
            break;
        }case SIGINT:{
            UTIL_LOG_INFO("SIGINT - Stopping server.");

            server_stop(&server);

            break;
        }case SIGTERM:{
            UTIL_LOG_INFO("Terminating Daemon.");

            exit(EXIT_SUCCESS);

            break;
        }
	}
}

local SERVER_CONTEXT_HANDLER(server_pageAdd){
    if(request->type != REQUEST_TYPE_POST){
       server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }
    // TODO:(jan) Add error handling for to small/malformed requests.
    uint_fast64_t readOffset = 0;

     // ShowName.
    const uint_fast64_t showNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    char* showName = (char*) request->data + readOffset;
    readOffset += showNameLength;

    // EpisodeName.
    const uint_fast64_t episodeNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    char* episodeName = (char*) request->data + readOffset;
    readOffset += episodeNameLength;

    // Season.
    const uint_fast16_t seasonNumber = util_byteArrayTo_uint16(request->data + readOffset);
    readOffset += sizeof(uint16_t);

    // Episode.
    const uint_fast16_t episodeNumber = util_byteArrayTo_uint16(request->data + readOffset);
    readOffset += sizeof(uint16_t);

    // FileExtension.
    const uint_fast16_t fileExtensionLength = util_byteArrayTo_uint16(request->data + readOffset);
    readOffset += sizeof(uint16_t);

    char* fileExtension = (char*) request->data + readOffset;
    readOffset += fileExtensionLength;

    ERROR_CODE error;
    if(showNameLength == 0){
        error = ERROR_INVALID_CONTENT_LENGTH;

        goto label_return;
    }

    Show* show;
    if((error = medialibrary_getShow(&server->library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
        if((error = mediaLibrary_addShow(&server->library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
            goto label_return;
        }
    }

    Season* season;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if((error = medialibrary_getSeason(show, &season, seasonNumber)) != ERROR_NO_ERROR){
        if((error = mediaLibrary_addSeason(&server->library, &season, show, seasonNumber)) != ERROR_NO_ERROR){
            goto label_return;
        }
    }

    Episode* episode;
    if(mediaLibrary_getEpisode(season, &episode, episodeNumber) == ERROR_NO_ERROR){
         error = strncmp(episode->name,  episodeName, episode->nameLength + 1) ? ERROR_NAME_MISSMATCH : ERROR_DUPLICATE_ENTRY;
            goto label_return;
    }

    if((error = mediaLibrary_addEpisode(&server->library, &episode, show, season, episodeNumber, episodeName, episodeNameLength, fileExtension, fileExtensionLength, true)) != ERROR_NO_ERROR){
        goto label_return;
    }

label_return:
    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);
   
    util_uint64ToByteArray(response->data + response->dataLength, error);
    response->dataLength += sizeof(uint64_t);

    return ERROR(error);
}

local SERVER_CONTEXT_HANDLER(server_pageAddShow){
    if(request->type != REQUEST_TYPE_POST){
        server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }

    uint_fast64_t readOffset = 0;

    const uint_fast64_t showNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    char* showName = (char*) request->data + readOffset;
    readOffset += showNameLength;

    Show* show;
    ERROR_CODE error;
    error = mediaLibrary_addShow(&server->library, &show, showName, showNameLength);

    util_uint64ToByteArray(response->data + response->dataLength, error);
    response->dataLength += sizeof(uint64_t);
   
    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);

    return ERROR(ERROR_NO_ERROR);
}

local SERVER_CONTEXT_HANDLER(server_pageRemoveShow){
    if(request->type != REQUEST_TYPE_POST){
        server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }

    uint_fast64_t readOffset = 0;

    const uint_fast64_t showNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    char* showName = (char*) request->data + readOffset;
    readOffset += showNameLength;

    ERROR_CODE error;
    error = mediaLibrary_removeShow(&server->library, showName, showNameLength);
    
    util_uint64ToByteArray(response->data + response->dataLength, error);
    response->dataLength += sizeof(uint64_t);

    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);

    return ERROR(ERROR_NO_ERROR);
}

local SERVER_CONTEXT_HANDLER(server_pageRemoveEpisode){
    if(request->type != REQUEST_TYPE_POST){
        server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }
     // TODO:(jan) Add error handling for to small/malformed requests.
    uint_fast64_t readOffset = 0;

     // ShowName.
    const uint_fast64_t showNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    char* showName = (char*) request->data + readOffset;
    readOffset += showNameLength;

    // Season.
    const uint_fast16_t seasonNumber = util_byteArrayTo_uint16(request->data + readOffset);
    readOffset += sizeof(uint16_t);

    // Episode.
    const uint_fast16_t episodeNumber = util_byteArrayTo_uint16(request->data + readOffset);
    readOffset += sizeof(uint16_t);

    ERROR_CODE error;
    if(showNameLength == 0){
        error = ERROR_INVALID_CONTENT_LENGTH;

        goto label_return;
    }

    Show* show;
    if((error = medialibrary_getShow(&server->library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    Season* season;
    if((error = medialibrary_getSeason(show, &season, seasonNumber)) != ERROR_NO_ERROR){
        goto label_return;
    }

    Episode* episode;
    if(mediaLibrary_getEpisode(season, &episode, episodeNumber) != ERROR_NO_ERROR){
        goto label_return;
    }

    error = medialibrary_removeEpisode(&server->library, show, season, episode, true);
    
label_return:
    util_uint64ToByteArray(response->data + response->dataLength, error);
    response->dataLength += sizeof(uint64_t);

    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);

    return ERROR(ERROR_NO_ERROR);
}

local SERVER_CONTEXT_HANDLER(server_pageShows){
     if(request->type != REQUEST_TYPE_GET){
        server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }

    // NumShows.
    util_uint64ToByteArray(response->data + response->dataLength, server->library.shows.length);
    response->dataLength += sizeof(uint64_t);    

    LinkedListIterator it;
    linkedList_initIterator(&it, &server->library.shows);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

        util_uint64ToByteArray(response->data + response->dataLength, show->nameLength);
        response->dataLength += sizeof(uint64_t);

        memcpy(response->data + response->dataLength, show->name, show->nameLength);
        response->dataLength += show->nameLength;
    }

    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;
    response->contentType = HTTP_CONTENT_TYPE_TEXT_HTML;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);
   
    return ERROR(ERROR_NO_ERROR);
}

local SERVER_CONTEXT_HANDLER(server_pageShowInfo){
     if(request->type != REQUEST_TYPE_POST){
        server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }

    uint_fast64_t readOffset = 0;

    const uint_fast64_t nameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    const char* showName = (char*) (request->data + readOffset);
    readOffset += nameLength + 1;

    Show* show;    
    if(medialibrary_getShow(&server->library, &show, showName, nameLength) == ERROR_ENTRY_NOT_FOUND){
        util_uint64ToByteArray(response->data + response->dataLength, ERROR_ENTRY_NOT_FOUND);
        response->dataLength += sizeof(uint64_t);
    }else{
        util_uint64ToByteArray(response->data + response->dataLength, ERROR_NO_ERROR);
        response->dataLength += sizeof(uint64_t);

        // Num_Seasons.
        util_uint64ToByteArray(response->data + response->dataLength, show->seasons.length);
        response->dataLength += sizeof(uint64_t);

            if(show->seasons.length > 0){
                Season** sortedSeasons = alloca(sizeof(Season*) * show->seasons.length);
                mediaLibrary_sortSeasons(&sortedSeasons, &show->seasons);

                uint_fast64_t j;
                for(j = 0; j < show->seasons.length; j++){
                Season* season = sortedSeasons[j];

                // Season_Number.
                util_uint16ToByteArray(response->data + response->dataLength, season->number);
                response->dataLength += sizeof(uint16_t);

                // Num_Episodes.
                util_uint64ToByteArray(response->data + response->dataLength, season->episodes.length);
                response->dataLength += sizeof(uint64_t);

                Episode** sortedEpisodes = alloca(sizeof(Episode*) * season->episodes.length);
                mediaLibrary_sortEpisodes(&sortedEpisodes, &season->episodes);

                uint_fast64_t j;
                for(j = 0; j < season->episodes.length; j++){
                    Episode* episode = sortedEpisodes[j];
                
                    // Episode_Number.
                    util_uint16ToByteArray(response->data + response->dataLength, episode->number);
                    response->dataLength += sizeof(uint16_t);

                    // Episode_NameLength.
                    util_uint64ToByteArray(response->data + response->dataLength, episode->nameLength);
                    response->dataLength += sizeof(uint64_t);

                    // Episode_Name.
                    memcpy(response->data + response->dataLength, episode->name, episode->nameLength + 1);
                    response->dataLength += episode->nameLength + 1;

                    // File_ExtensionLength.
                    util_uint16ToByteArray(response->data + response->dataLength, episode->fileExtensionLength);
                    response->dataLength += sizeof(uint16_t);

                    // File_Extension.
                    memcpy(response->data + response->dataLength, episode->fileExtension, episode->fileExtensionLength + 1);
                    response->dataLength += episode->fileExtensionLength + 1;
                }
            }
        }
    }

    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;
    response->contentType = HTTP_CONTENT_TYPE_TEXT_HTML;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);
   
    return ERROR(ERROR_NO_ERROR);
}

local SERVER_CONTEXT_HANDLER(server_pageRenameEpisode){
     if(request->type != REQUEST_TYPE_POST){
        server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }

    ERROR_CODE error;
    
    uint_fast64_t readOffset = 0;

    // Show_name.
    const uint_fast64_t showNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    const char* showName = (char*) (request->data + readOffset);
    readOffset += showNameLength + 1;

    Show* show;
    if((error = medialibrary_getShow(&server->library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
        goto label_onError;
    }

    // Season_Number.
    uint16_t seasonNumber = util_byteArrayTo_uint16(request->data + readOffset);
    readOffset += sizeof(uint16_t);

    Season* season;
    if((error = medialibrary_getSeason(show, &season, seasonNumber)) != ERROR_NO_ERROR){
        goto label_onError;
    }

    // Episode_Number.
    uint16_t episodeNumber = util_byteArrayTo_uint16(request->data + readOffset);
    readOffset += sizeof(uint16_t);   

    Episode* episode;
    if((error = mediaLibrary_getEpisode(season, &episode, episodeNumber)) != ERROR_NO_ERROR){
        goto label_onError;
    }

    // New Episode_Name.
    const uint_fast64_t newEpisodeNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    char* newEpisodeName = (char*) (request->data + readOffset);
    readOffset += newEpisodeNameLength + 1;

    if((error = mediaLibrary_renameEpisode(&server->library, show, season, episode, newEpisodeName, newEpisodeNameLength)) != ERROR_NO_ERROR){
        goto label_onError;
    }

 label_onError:
    util_uint64ToByteArray(response->data + response->dataLength, error);
    response->dataLength += sizeof(uint64_t);

    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;
    response->contentType = HTTP_CONTENT_TYPE_TEXT_HTML;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);
   
    return ERROR(ERROR_NO_ERROR);
}

local SERVER_CONTEXT_HANDLER(server_pageExtractShowInfo){
     if(request->type != REQUEST_TYPE_POST){
        server_constructErrorPage(server, request, response, _405_METHOD_NOT_ALLOWED);

        return ERROR(ERROR_NO_ERROR);
    }

    ERROR_CODE error = ERROR_NO_ERROR;

    uint_fast64_t readOffset = 0;

    const uint_fast64_t fileNameLength = util_byteArrayTo_uint64(request->data + readOffset);
    readOffset += sizeof(uint64_t);

    char* fileName = (char*) (request->data + readOffset);
    readOffset += fileNameLength;

    EpisodeInfo info;
    if((error = mediaLibrary_initEpisodeInfo(&info)) != ERROR_NO_ERROR){
        goto label_onError;
    }else{
        error = mediaLibrary_extractEpisodeInfo(&info, &server->library.shows, fileName, fileNameLength);

        if(error != ERROR_NO_ERROR && error != ERROR_INCOMPLETE){
            mediaLibrary_freeEpisodeInfo(&info);
            
            goto label_onError;
        }else{
            util_uint64ToByteArray(response->data + response->dataLength, error);
            response->dataLength += sizeof(uint64_t);

            util_uint64ToByteArray(response->data + response->dataLength, info.showNameLength);
            response->dataLength += sizeof(uint64_t);

            memcpy(response->data + response->dataLength, info.showName, info.showNameLength);
            response->dataLength += info.showNameLength;

            util_uint64ToByteArray(response->data + response->dataLength, info.nameLength);
            response->dataLength += sizeof(uint64_t);

            memcpy(response->data + response->dataLength, info.name, info.nameLength);
            response->dataLength += info.nameLength;

            util_uint16ToByteArray(response->data + response->dataLength, info.season);
            response->dataLength += sizeof(uint16_t);

            util_uint16ToByteArray(response->data + response->dataLength, info.episode);
            response->dataLength += sizeof(uint16_t);

            goto label_return;
        }
    }

label_onError:
    util_uint64ToByteArray(response->data + response->dataLength, error);
    response->dataLength += sizeof(uint64_t);

label_return:
    mediaLibrary_freeEpisodeInfo(&info);

    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;
    response->contentType = HTTP_CONTENT_TYPE_TEXT_HTML;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);
   
    return ERROR(ERROR_NO_ERROR);
}

// TODO:(jan) Make custom entry point for the server build, so we won't have to use 'main'.
#ifndef TEST_BUILD
	int main(const int argc, const char** argv){
#else
    int server_totalyNotMain(const int argc, const char** argv){
#endif
        ERROR_CODE error;
        openlog(SERVER_DAEMON_NAME, LOG_PID | LOG_NOWAIT, LOG_DAEMON);

        ArgumentParser parser;
        if((error = argumentParser_init(&parser)) != ERROR_NO_ERROR){
            goto label_exit;
        }
    
        ARGUMENT_PARSER_ADD_ARGUMENT(Help, 3, "-?", "-h", "--help");
        ARGUMENT_PARSER_ADD_ARGUMENT(SetServerRootDirectory, 1, "--setServerRootDirectory");
        ARGUMENT_PARSER_ADD_ARGUMENT(SetServerExternalPort, 1, "--setServerExternalPort");
        ARGUMENT_PARSER_ADD_ARGUMENT(ShowSettings, 1, "--showSettings");

        __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_NO_VALID_ARGUMENT);
        if((error = argumentParser_parse(&parser, argc, argv)) != ERROR_NO_ERROR){
            if(error == ERROR_DUPLICATE_ENTRY){
                UTIL_LOG_CONSOLE(LOG_ERR, "Duplicate argument.");

                goto label_exit;
            }
        }

        const char* userHome = util_getHomeDirectory();
        const uint_fast64_t userHomeLength = strlen(userHome);

        const uint_fast64_t serverWorkingDirectoryLength = userHomeLength + 15/*/herder/daemon/*/ ;
        char* serverWorkingDirectory = alloca(sizeof(*serverWorkingDirectory) * (serverWorkingDirectoryLength + 1));
        strncpy(serverWorkingDirectory, userHome, userHomeLength);
        strncpy(serverWorkingDirectory + userHomeLength, "/herder/daemon/", 16);

		if(!util_fileExists(serverWorkingDirectory)){
			UTIL_LOG_CONSOLE_(LOG_ERR, "WorkingDirectory '%s' %s.", serverWorkingDirectory, strerror(errno));

			goto label_exit;
		}

        const uint_fast64_t proeprtyFileNameLength = strlen(PROPERTY_FILE_NAME);
        const uint_fast64_t propertyFilePathLength = serverWorkingDirectoryLength + proeprtyFileNameLength;

        char* propertyFilePath = alloca(sizeof(*propertyFilePath) * (propertyFilePathLength + 1));
        strncpy(propertyFilePath, serverWorkingDirectory, serverWorkingDirectoryLength);
        strncpy(propertyFilePath + serverWorkingDirectoryLength, PROPERTY_FILE_NAME, proeprtyFileNameLength + 1);

        if(!util_fileExists(propertyFilePath)){
            propertyFile_create(propertyFilePath, 8);
        }

        PropertyFile properties;
        propertyFile_init(&properties, propertyFilePath);

        Property* serverRootDirectory;
        propertyFile_getProperty(&properties, &serverRootDirectory, SERVER_PROPERTY_SERVER_ROOT_DIRECTORY_NAME);

        Property* serverExternalPort;
        propertyFile_getProperty(&properties, &serverExternalPort, PROPERTY_SERVER_EXTERNAL_PORT_NAME);

        if(argumentParser_contains(&parser, &argumentHelp)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Usage: herder --[command]/-[alias] <arguments>.\n");

            UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t%s", SERVER_USAGE_ARGUMENT_SET_SERVER_ROOT_DIRECTORY,  "Sets the 'server root directory' to the given path.");
            UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t%s", SERVER_USAGE_ARGUMENT_SET_SERVER_EXTERNAL_PORT, "Sets the external port to the given port.");
            UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", SERVER_USAGE_ARGUMENT_SHOW_SETTINGS, "Shows a quick overview of all the user settings.");

            UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", SERVER_USAGE_ARGUMENT_HELP, "Displays this help.");

            goto label_exit;
        }

        // --setServerRootDirectory
        if(argumentParser_contains(&parser, &argumentSetServerRootDirectory)){
           if(argumentSetServerRootDirectory.numValues != 1){
                UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " SERVER_USAGE_ARGUMENT_SET_SERVER_ROOT_DIRECTORY);
            }else{
                if((error = propertyFile_createAndSetDirectoryProperty(&properties, serverRootDirectory, SERVER_PROPERTY_SERVER_ROOT_DIRECTORY_NAME, argumentSetServerRootDirectory.values[0], argumentSetServerRootDirectory.valueLengths[0])) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
                }else{
                    UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'", SERVER_PROPERTY_SERVER_ROOT_DIRECTORY_NAME, argumentSetServerRootDirectory.values[0]);
                }

                goto label_free;
            }
        }

        // --setServerExternalPort
        if(argumentParser_contains(&parser, &argumentSetServerExternalPort)){
	        if(argumentSetServerExternalPort.numValues != 1){
                UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " PROPERTY_SERVER_EXTERNAL_PORT_NAME);
            }else{
                int64_t port;
                ERROR_CODE error;
                if((error = util_stringToInt(argumentSetServerExternalPort.values[0], &port)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Port value must be in range of 0-%" PRIu16 ". '%s'.", UINT16_MAX, util_toErrorString(ERROR_INVALID_VALUE));

                    goto label_free;
                }else{
                    if(port <= 0 || port > UINT16_MAX){
                        UTIL_LOG_CONSOLE_(LOG_ERR, "Port value must be in range of 0-%" PRIu16 ". '%s'.", UINT16_MAX, util_toErrorString(ERROR_INVALID_VALUE));

                        goto label_free;
                    }
                }

                if((error = propertyFile_createAndSetUINT16Property(&properties, serverExternalPort, PROPERTY_SERVER_EXTERNAL_PORT_NAME, port)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
                }else{
                    UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'", PROPERTY_SERVER_EXTERNAL_PORT_NAME, argumentSetServerExternalPort.values[0]);
                }

                goto label_free;
            }
        }

        // --showSettings
        if(argumentParser_contains(&parser, &argumentShowSettings)){
  		    if(argumentShowSettings.numValues != 0){
                UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --showSettings.");
            }else{
                UTIL_LOG_CONSOLE_(LOG_INFO, "ServerRootDirectory: '%s'.", PROPERTY_IS_SET(serverRootDirectory) ? (char*) serverRootDirectory->buffer : "NULL");

                UTIL_LOG_CONSOLE_(LOG_INFO, "ServerExternalPort: '%" PRIuFAST16 "'.", PROPERTY_IS_SET(serverExternalPort) ? util_byteArrayTo_uint16(serverExternalPort->buffer) : 0);                
		    }
            
            goto label_exit;
        }

        argumentParser_free(&parser);

        if(PROPERTY_IS_NOT_SET(serverRootDirectory)){
            UTIL_LOG_CONSOLE_(LOG_ERR, "Error: Failed to find server settings entry for '%s'.\n\tUse --setServerRootDirectory <path> to set the http server root directory.", SERVER_PROPERTY_SERVER_ROOT_DIRECTORY_NAME);

            goto label_exit;
        }

        if(PROPERTY_IS_NOT_SET(serverExternalPort)){
            UTIL_LOG_CONSOLE_(LOG_ERR, "Error: Failed to find server settings entry for '%s'.\n\tUse --setServerExternalPort <port> to set the external http server port.", PROPERTY_SERVER_EXTERNAL_PORT_NAME);

            goto label_exit;
        }

	    // server_daemonize(serverWorkingDirectory);

        const uint_fast64_t serverDaemonNameLength = strlen(SERVER_DAEMON_NAME);
        const uint_fast64_t serverDaemonLockFileNameLength = serverWorkingDirectoryLength + serverDaemonNameLength;

        char* serverDaemonLockFileName = alloca(sizeof(*serverDaemonLockFileName) * (serverDaemonLockFileNameLength +5 /*'.lock'*/ + 1));
        strncpy(serverDaemonLockFileName, serverWorkingDirectory, serverWorkingDirectoryLength);
        strncpy(serverDaemonLockFileName + serverWorkingDirectoryLength, SERVER_DAEMON_NAME ".lock", serverDaemonNameLength + 5 + 1);

		int lockFile;
		lockFile = open(serverDaemonLockFileName, O_RDWR | O_CREAT, 0640);

        if(lockFile == -1){
            UTIL_LOG_ERROR_("Failed to open file: '%s'. %s.", serverDaemonLockFileName, strerror(errno));

            goto label_exit;
        }

		if(lockf(lockFile, F_TLOCK, 0)  == -1){
			UTIL_LOG_INFO("Server already running.");
		}else{
            const uint_fast16_t port = util_byteArrayTo_uint16((int8_t*) serverExternalPort->buffer);

			if((error = server_init(&server, (char*) serverRootDirectory->buffer, serverRootDirectory->entry->length - 1, port)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));

				goto label_free;
			}

			// HTML/Static pages
			server_addContext(&server, "/", server_defaultContextHandler);
			server_addContext(&server, "/img", server_defaultContextHandler);
			server_addContext(&server, "/css", server_defaultContextHandler);
			server_addContext(&server, "/js", server_defaultContextHandler);

			// Herder media library rest api.
            server_addContext(&server, "/add", server_pageAdd);
            server_addContext(&server, "/addShow", server_pageAddShow);
            server_addContext(&server, "/removeShow", server_pageRemoveShow);
			server_addContext(&server, "/shows", server_pageShows);
            server_addContext(&server, "/extractShowInfo", server_pageExtractShowInfo);
            server_addContext(&server, "/showInfo", server_pageShowInfo);
            server_addContext(&server, "/renameEpisode", server_pageRenameEpisode);
            server_addContext(&server, "/removeEpisode", server_pageRemoveEpisode);

			UTIL_LOG_INFO_("Starting server on port '%" PRIuFAST16 "'.", port);

			if((error = server_start(&server)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE_(LOG_ERR, "%s", util_toErrorString(error));
            }

            if(lockf(lockFile, F_ULOCK, 0)  == -1){
			    UTIL_LOG_ERROR_("Failed to unlock lock file [%d] (%s)", errno, strerror(errno));
		    }
		}

	label_free:
		close(lockFile);

        server_free(&server);

        util_deleteFile(serverDaemonLockFileName);

        propertyFile_free(&properties);

        if(PROPERTY_IS_SET(serverRootDirectory)){
            propertyFile_freeProperty(serverRootDirectory);

            free(serverRootDirectory);
        }

        if(PROPERTY_IS_SET(serverExternalPort)){
            propertyFile_freeProperty(serverExternalPort);

            free(serverExternalPort);
        }

    label_exit:
        closelog();

		return 0;
	}

ERROR_CODE server_init(HerderServer* server, const char* rootDirectory, const uint_fast64_t rootDirectoryLength, const uint_fast16_t port){
    memset(server, 0, sizeof(*server));

    server->port = port;

    ERROR_CODE error;
    // TODO:(jan) Choose a sane number of threads here, or refactor threadPool to not need numWokers as an argument.
    const int_fast32_t numAvailableCores = util_getNumAvailableProcessorCores();
    const int_fast32_t numThreads = numAvailableCores * 12;
        
    if((error = threadPool_init(&server->threadPool, numThreads)) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if((server->sockFD = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        return ERROR(ERROR_UNIX_DOMAIN_SOCKET_INITIALISATION_FAILED);
    }

    server->sockAddr.sin_family = AF_INET;
    server->sockAddr.sin_addr.s_addr = INADDR_ANY;
    server->sockAddr.sin_port = htons(port);

    linkedList_init(&server->contexts);

    if((error = mediaLibrary_init(&server->library, rootDirectory, rootDirectoryLength)) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    cache_init(&server->cache, numThreads, 32 * 1024 * 1024);

    const uint_fast64_t serverWebDirectoryLength = strlen(SERVER_WEB_DIRECTORY);

    server->rootDirectoryLength = rootDirectoryLength + serverWebDirectoryLength;

    server->rootDirectory = malloc(sizeof(*server->rootDirectory) * (server->rootDirectoryLength + 1));
    if(server->rootDirectory == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    strncpy(server->rootDirectory, rootDirectory, rootDirectoryLength);
    strncpy(server->rootDirectory + rootDirectoryLength, SERVER_WEB_DIRECTORY, serverWebDirectoryLength);
    server->rootDirectory[server->rootDirectoryLength] = '\0';
    
    if(!util_fileExists(server->rootDirectory)){
        if((error = util_createDirectory(server->rootDirectory) != ERROR_NO_ERROR)){
            return ERROR(error);
        }
    }

    if(bind(server->sockFD, (struct sockaddr*) &server->sockAddr, sizeof(server->sockAddr)) == -1){
        return ERROR(ERROR_FAILED_TO_BIND_SERVER_SOCKET);
    }

    server->alive = true;

    // TODO:(jan) Decide on a socket backlog value, or implement epool for fun and giggles.
    if(listen(server->sockFD, SOMAXCONN) == -1){
        return ERROR(ERROR_FAILED_TO_LISTEN_ON_SERVER_SOCKET);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline void server_initClient(Client* client, HerderServer* server){
    memset(client, 0, sizeof(*client));

    client->server = server;
}

inline ERROR_CODE server_getFile(HerderServer* server, CacheObject** cacheObject, char* symbolicFileLocation, const uint_fast64_t symbolicFileLocationLength){
    *cacheObject = NULL;
    ERROR_CODE error;
    if((error = cache_get(&server->cache, cacheObject, symbolicFileLocation, symbolicFileLocationLength)) != ERROR_NO_ERROR){
        return ERROR(error);
    }

    if(*cacheObject == NULL){
        const uint_fast64_t fileLocationLength = server->rootDirectoryLength + symbolicFileLocationLength - 1;
        
        char* fileLocation = malloc(sizeof(*fileLocation) * fileLocationLength + 1);
        if(fileLocation == NULL){
            return ERROR(ERROR_OUT_OF_MEMORY);
        }

        memcpy(fileLocation, server->rootDirectory, server->rootDirectoryLength);
        memcpy(fileLocation + server->rootDirectoryLength, symbolicFileLocation + 1, symbolicFileLocationLength - 1);
        fileLocation[fileLocationLength] = '\0';

        error = cache_load(&server->cache, cacheObject, fileLocation, fileLocationLength, symbolicFileLocation, symbolicFileLocationLength);
   }

    return ERROR_(error, "File:'%s'", symbolicFileLocation);
}

inline ERROR_CODE server_initContext(Context* context, const char* location, const uint_fast64_t locationLength, ContextHandler* contextHandler){
    context->locationLength = locationLength;
    context->location = malloc(sizeof(*location) * (locationLength + 1));
    if(context->location == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    strncpy(context->location, location, locationLength + 1);

    context->contextHandler = contextHandler;

    return ERROR(ERROR_NO_ERROR);
}

inline void server_freeContext(Context* context){
    free(context->location);
}

ERROR_CODE server_start(HerderServer* server){
    while(server->alive){
        Client* client = malloc(sizeof(*client));
        if(client == NULL){
            return ERROR(ERROR_OUT_OF_MEMORY);
        }

        server_initClient(client, server);
        
        socklen_t clientSocketSize = sizeof(client->socket);
        if((client->sockFD = accept(server->sockFD, (struct sockaddr*) &client->socket, &clientSocketSize)) == -1){
            UTIL_LOG_INFO_("Failed to accept incoming connection. '%s'.", strerror(errno));
            
            free(client);

            continue;
        }

        // Note: We cast server_run to Runnable* here to be able to use the THREAD_POOL_RUNNABLE_ macro in the defenition of server_run, this allows us to have type information inside the function. (jan - 2019.03.07)
        threadPool_run(&server->threadPool, (Runnable*) server_run, client);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline void server_free(HerderServer* server){
    close(server->sockFD);

    threadPool_free(&server->threadPool);

    LinkedListIterator it;
    linkedList_initIterator(&it, &server->contexts);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Context* context = LINKED_LIST_ITERATOR_NEXT(&it);

        server_freeContext(context);

        free(context);
    }

    linkedList_free(&server->contexts);

    free(server->rootDirectory);

    cache_free(&server->cache);

    mediaLibrary_free(&server->library);
}

inline ERROR_CODE server_stop(HerderServer* server){
    server->alive = false;

    shutdown(server->sockFD, SHUT_RDWR);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE server_addContext(HerderServer* server, const char* location, ContextHandler* contextHandler){
    Context* context = malloc(sizeof(*context));
    if(context == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    server_initContext(context, location, strlen(location), contextHandler);

    linkedList_add(&server->contexts, context);

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE server_getContext(HerderServer* server, ContextHandler** contextHandler, HTTP_Request* request){
    char* baseDirectory;
    uint_fast64_t baseDirectoryLength;
    if(util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, request->requestURL, request->urlLength) != ERROR_NO_ERROR){
        return ERROR_(ERROR_INVALID_REQUEST_URL, "Failed to find baseDirectory URL:'%s'.",request->requestURL);
    }
    
    LinkedListIterator it;
    linkedList_initIterator(&it, &server->contexts);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Context* context = LINKED_LIST_ITERATOR_NEXT(&it);

        // UTIL_LOG_CONSOLE_(LOG_DEBUG, "'%s' - [%" PRIuFAST64 "] :'%s' + '%c'", baseDirectory, baseDirectoryLength, context->location, context->location[context->locationLength]);

        if(strncmp(baseDirectory, context->location, context->locationLength + 1) == 0){
            *contextHandler = context->contextHandler;

            return ERROR(ERROR_NO_ERROR);
        }
    }

    *contextHandler = NULL;

    return ERROR(ERROR_ENTRY_NOT_FOUND);
}

ERROR_CODE server_constructErrorPage(HerderServer* server,HTTP_Request* request,  HTTP_Response* response, HTTP_StatusCode statusCode){
    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = statusCode;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);

    const char errorPage[] = "<!DOCTYPE html><html> <head> <meta charset=\"utf-8\"> <title>$errorCode</title> </head> <body> <h1>$errorCode</h1> <p>$errorMessage</p> <hr> <address>'Herder' HTTP-Web Server at <a href=\"http://$address\">$address</a> Port $port</address> </body></html>";
    uint_fast64_t pageLength = strlen(errorPage);

    memcpy(response->data, errorPage, pageLength + 1);

    // ErrorCode.
    const char errorCodeSearchString[] = "$errorCode";
    const uint_fast64_t errorCodeSearchStringLength = strlen(errorCodeSearchString);

    char errorString[UTIL_FORMATTED_NUMBER_LENGTH];
    int_fast64_t formatedNumberLength = snprintf(errorString, UTIL_FORMATTED_NUMBER_LENGTH, "%" PRIdFAST16 "", http_getStatusCode(statusCode));

    if(util_replace((char*) response->data, response->bufferSize, &pageLength, errorCodeSearchString, errorCodeSearchStringLength, errorString, formatedNumberLength) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to replace '$erroCode'.");        
    }

    // ErrorMessage.
    const char errorMessageSearchString[] = "$errorMessage";

    const char* errorMessage = http_getStatusMsg(statusCode);

    if(util_replace((char*) response->data, response->bufferSize, &pageLength, errorMessageSearchString, strlen(errorMessageSearchString), errorMessage, strlen(errorMessage)) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to replace '$errorMessage'.");        
    }

    // Port.
    const char portSeasrchString[] = "$port";
    
    char port[UTIL_FORMATTED_NUMBER_LENGTH];
    int_fast64_t portLength = snprintf(port, UTIL_FORMATTED_NUMBER_LENGTH, "%" PRIdFAST32 "", server->port);
    
    if(util_replace((char*) response->data, response->bufferSize, &pageLength, portSeasrchString, strlen(portSeasrchString), port, portLength) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to replace '$errorMessage'.");        
    }

    // Url.
    const char urlSeasrchString[] = "$url";
    
    uint_fast64_t urlLength = request->urlLength;
    char* url = alloca(sizeof(*url) * (urlLength + 1));
    memcpy(url, request->requestURL, urlLength);
    url[urlLength] = '\0';

    if(util_replace((char*) response->data, response->bufferSize, &pageLength, urlSeasrchString, strlen(urlSeasrchString), url, request->urlLength) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to replace '$errorMessage'.");        
    }

    // Address.    
    const HTTP_HeaderField* headerFieldServer = http_getHeaderField(request, "Host");

    if(headerFieldServer != NULL){
        const char adddressSeatchString[] = "$address";

        if(util_replace((char*) response->data, response->bufferSize, &pageLength, adddressSeatchString, strlen(adddressSeatchString), headerFieldServer->value, headerFieldServer->valueLength) != ERROR_NO_ERROR){
                UTIL_LOG_ERROR("Failed to replace '$address'.");        
        }
    }

    response->dataLength = pageLength;

    return ERROR(ERROR_NO_ERROR);
}

inline void server_daemonize(const char* workingDirectory){
    // Ignore child signals, so we dont end up with a defunct zombie child process.
    signal(SIGCHLD, SIG_IGN);

    pid_t pid = fork();

    if(pid < 0){
        exit(EXIT_FAILURE);
    }else if(pid > 0){
        exit(EXIT_SUCCESS);
    }

    if(setsid() < 0){
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if(pid < 0){
        exit(EXIT_FAILURE);
    }else if(pid > 0){
        exit(EXIT_SUCCESS);
    }

    umask(S_IWGRP | S_IWOTH /*022*/);

    if(chdir(workingDirectory) != 0){
        UTIL_LOG_ERROR("Failed to change working directory."); 
    }

    int_fast32_t fd;
    for (fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--){
        close(fd);
    }

    openlog(SERVER_DAEMON_NAME, LOG_PID | LOG_NOWAIT, LOG_DAEMON);

    // Ignore TTY signals.
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

    struct sigaction action = {0};

     // Block every signal while the handler runs.
    sigfillset(&action.sa_mask);

    action.sa_sigaction = &server_deamonSignalHandler;
    action.sa_flags = SA_SIGINFO;

    if(sigaction(SIGINT, &action, NULL) != 0) {
		UTIL_LOG_WARNING("Failed to append signal handler. [SIGINT]"); 
    }

    if(sigaction(SIGHUP, &action, NULL) != 0) {
		UTIL_LOG_WARNING("Failed to append signal handler. [SIGHUP]"); 
	}

    if(sigaction(SIGTERM, &action, NULL) != 0) {
		UTIL_LOG_WARNING("Failed to append signal handler. [SIGTERM]"); 
	}
}

#undef SERVER_WEB_DIRECTORY

#endif