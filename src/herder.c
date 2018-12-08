// POSIX Version ¢2008
#define _XOPEN_SOURCE 700

// For mmap flags that are not in the POSIX-Standard.
#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <signal.h>

#include "util.c"
#include "http.c"
#include "argumentParser.c"
#include "propertyFile.c"
#include "mediaLibrary.c"

#define HERDER_PROGRAM_NAME "herder"

#define PROPERTY_IMPORT_DIRECTORY_NAME "importDirectory"
#define PROPERTY_REMOTE_HOST_NAME "remoteHost"
#define PROPERTY_REMOTE_PORT_NAME "remotePort"
#define PROPERTY_LIBRARY_DIRECTORY_NAME "libraryDirectory"

#define HTTP_PROCESSING_BUFFER_SIZE 8192 //16384

#define HERDER_CONSTRUCT_FILE_PATH(path, stringLength, episodeInfo) \
    *stringLength = ((episodeInfo)->showNameLength * 3) + ((episodeInfo)->nameLength) + (3/*" - "*/ + 7/*"Season_"*/ + 2/*"/"*/) + (UTIL_UINT16_STRING_LENGTH * 3); \
    \
    *(path) = alloca(sizeof(**(path)) * (*stringLength + 1)); \
    *stringLength = snprintf(*path, *stringLength, "%s/%s - Season_%02" PRIdFAST16 "/%s_s%02" PRIdFAST16 "e%02" PRIdFAST16 "_%s", (episodeInfo)->showName, (episodeInfo)->showName, (episodeInfo)->season, (episodeInfo)->showName, (episodeInfo)->season, (episodeInfo)->episode, (episodeInfo)->name); \
    \
    util_replaceAllChars(*path + (*stringLength - ((episodeInfo)->showNameLength + (2 * UTIL_UINT16_STRING_LENGTH) + (episodeInfo)->nameLength + 4)), ' ', '_')

local void herder_printHelp(void);

local ERROR_CODE herder_listShows(Property*, Property*);

local ERROR_CODE herder_pullShowList(ArrayList*, const char*, const uint_fast16_t);

local ERROR_CODE herder_addShow(Property*, Property*, char*, const uint_fast64_t);

local ERROR_CODE herder_pullShowInfo(Property*, Property*, char*, const uint_fast64_t);

local ERROR_CODE herder_extractShowInfo(Property*, Property*, EpisodeInfo**, char*, const uint_fast64_t);

local ERROR_CODE herder_addEpisode(Property*, Property*, Property*, char*, const uint_fast64_t);

// TODO:(jan) Make custom entry point for the client build, so we won't have to use 'main'.
#ifndef TEST_BUILD
	int main(const int argc, const char** argv){
#else
    int herder_totalyNotMain(const int argc, const char** argv){
#endif
    ERROR_CODE error = ERROR_NO_ERROR;

	openlog(HERDER_PROGRAM_NAME, LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_USER);
	
    ArgumentParser parser;
    argumentParser_init(&parser);

	ARGUMENT_PARSER_ADD_ARGUMENT(Help, 3, "-?", "-h", "--help");
    ARGUMENT_PARSER_ADD_ARGUMENT(Add, 2, "-a", "--add");
    ARGUMENT_PARSER_ADD_ARGUMENT(AddShow, 1, "--addShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetImportDirectory, 1, "--setImportDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetLibraryDirectory, 1, "--setLibraryDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHost, 1, "--setRemoteHost");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHostPort, 1, "--setRemotePort");
    ARGUMENT_PARSER_ADD_ARGUMENT(KillDeamon, 1, "--killDeamon");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import, 2, "-i", "--import");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListShows, 3, "-l", "--list", "--listShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowInfo, 1, "--info");

	if(argumentParser_parse(&parser, argc, argv) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to parse command line arguments.");

		goto label_free;
    }

    bool noValidArgument = true;

   if(argc <= 1 || argumentParser_contains(&parser, &argumentHelp)){
        herder_printHelp();

        goto label_free;
    }

    const char* userHome = util_getHomeDirectory();
    const uint_fast64_t userHomeLength = strlen(userHome);

    const uint_fast64_t propertyFilePathLength = userHomeLength + 16/*/herder/settings*/;
    char* propertyFilePath = alloca(sizeof(*propertyFilePath) * (propertyFilePathLength + 1));
    strncpy(propertyFilePath, userHome, userHomeLength);
    strncpy(propertyFilePath + userHomeLength, "/herder/settings", 17);
    
    if(!util_fileExists(propertyFilePath)){
        if(propertyFile_create(8, propertyFilePath) != ERROR_NO_ERROR){
            goto label_free;
        }
    }

    PropertyFile properties;
    if(propertyFile_init(&properties, propertyFilePath) != ERROR_NO_ERROR){
        goto label_free;
    }

    Property* importDirectory;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    propertyFile_getProperty(&properties, &importDirectory, PROPERTY_IMPORT_DIRECTORY_NAME);

    Property* remoteHost;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    propertyFile_getProperty(&properties, &remoteHost, PROPERTY_REMOTE_HOST_NAME);

    Property* remotePort;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    propertyFile_getProperty(&properties, &remotePort, PROPERTY_REMOTE_PORT_NAME);

    Property* libraryDirectory;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    propertyFile_getProperty(&properties, &libraryDirectory, PROPERTY_LIBRARY_DIRECTORY_NAME);

    #define REMOTE_HOST_PROPERTIES_SET() ((propertyFile_propertySet(remoteHost, PROPERTY_REMOTE_HOST_NAME) == ERROR_NO_ERROR) && (propertyFile_propertySet(remotePort, PROPERTY_REMOTE_PORT_NAME) == ERROR_NO_ERROR))

	if(argumentParser_contains(&parser, &argumentSetImportDirectory)){
         noValidArgument = false;

         if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetImportDirectory)){
            if(PROPERTY_IS_NOT_SET(importDirectory)){
                propertyFile_addProperty(&properties, &importDirectory, PROPERTY_IMPORT_DIRECTORY_NAME, argumentSetImportDirectory.valueLength + 1);
            }else{
                if(importDirectory->entry->length != argumentSetImportDirectory.valueLength + 1){
                    propertyFile_removeProperty(importDirectory);

                    importDirectory = NULL;

                    propertyFile_addProperty(&properties, &importDirectory, PROPERTY_IMPORT_DIRECTORY_NAME, argumentSetImportDirectory.valueLength + 1);
                }
            }

            propertyFile_setBuffer(importDirectory, (int8_t*) argumentSetImportDirectory.value);

            printf("%s(\"%s\").\n", "setImportDirectory", (char*) importDirectory->buffer);
        }else{
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setImportDirectory <path>.");
        }
	}

	if(argumentParser_contains(&parser, &argumentSetRemoteHost)){
         noValidArgument = false;

         if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetRemoteHost)){
            if(PROPERTY_IS_NOT_SET(remoteHost)){
                propertyFile_addProperty(&properties, &remoteHost, PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost.valueLength + 1);
            }else{
                if(remoteHost->entry->length != argumentSetRemoteHost.valueLength + 1){
                    propertyFile_removeProperty(remoteHost);

                    remoteHost = NULL;

                    propertyFile_addProperty(&properties, &remoteHost, PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost.valueLength + 1);
                }
            }

            propertyFile_setBuffer(remoteHost, (int8_t*) argumentSetRemoteHost.value);

            printf("%s(\"%s\").\n", "setRemoteHost", (char*) remoteHost->buffer);
        }else{
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setRemoteHost <URL>.");
        }
    }

    if(argumentParser_contains(&parser, &argumentSetRemoteHostPort)){
        noValidArgument = false;

         if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetRemoteHostPort)){
            if(PROPERTY_IS_NOT_SET(remotePort)){
                propertyFile_addProperty(&properties, &remotePort, PROPERTY_REMOTE_PORT_NAME, sizeof(uint16_t));
            }

            errno = 0;

            char* endPtr;
            long port = strtol(argumentSetRemoteHostPort.value, &endPtr, 10/*Decimal*/);

            if((port == 0 && endPtr == argumentSetRemoteHostPort.value) || (errno == ERANGE && (port == LONG_MIN || port == LONG_MAX))){
                ERROR(ERROR_INVALID_STATUS_CODE);

                goto label_free;
            }

            int8_t* buffer = alloca(sizeof(uint16_t));
            util_uint16ToByteArray(buffer, port);

            propertyFile_setBuffer(remotePort, buffer);

            printf("%s(\"%ld\").\n", "setRemoteHostPort", port);
        }else{
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setRemotePort <port>.");
        }
    }

    if(argumentParser_contains(&parser, &argumentSetLibraryDirectory)){
        noValidArgument = false;

         if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetLibraryDirectory)){
            if(PROPERTY_IS_NOT_SET(libraryDirectory)){
                propertyFile_addProperty(&properties, &libraryDirectory, PROPERTY_LIBRARY_DIRECTORY_NAME, argumentSetLibraryDirectory.valueLength + 1);
            }else{
                if(libraryDirectory->entry->length != argumentSetLibraryDirectory.valueLength + 1){
                    propertyFile_removeProperty(libraryDirectory);

                    libraryDirectory = NULL;

                    propertyFile_addProperty(&properties, &libraryDirectory, PROPERTY_LIBRARY_DIRECTORY_NAME, argumentSetLibraryDirectory.valueLength + 1);
                }
            }

            propertyFile_setBuffer(libraryDirectory, (int8_t*) argumentSetLibraryDirectory.value);

            printf("%s(\"%s\").\n", "setLibraryDirectory", (char*) libraryDirectory->buffer);
        }else{
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setLibraryDirectory <path>.");
        }
    }

    if(argumentParser_contains(&parser, &argumentImport)){
        noValidArgument = false;

        if(REMOTE_HOST_PROPERTIES_SET()){
            // Import from predefined import directory.
            if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentImport)){
                // if(herder.importDirectory == NULL){
                //     UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --import optional:<path>.");
                // }else{
                //     if(herder_addFromDirectory(&herder, (char*) herder.importDirectory->buffer, herder.importDirectory->entry->length) != ERROR_NO_ERROR){
                //         UTIL_LOG_ERROR("Failed to addFromDirectory.");
                //     }
                // }

                printf("%s.\n", "import");
            }else{
                // if(herder_addFromDirectory(&herder, argumentImport.value, argumentImport.valueLength) != ERROR_NO_ERROR){
                //     UTIL_LOG_ERROR("Failed to addFromDirectory.");
                // }

                printf("%s(\"%s\").\n", "import", argumentImport.value);
            }
        }
    }

    if(argumentParser_contains(&parser, &argumentKillDeamon)){
        noValidArgument = false;

        if(REMOTE_HOST_PROPERTIES_SET()){
            UTIL_LOG_CONSOLE(LOG_CRIT, "Kill daemon.");

            UTIL_LOG_CRITICAL("Function not implemented.");
        }
    }

    if(argumentParser_contains(&parser, &argumentShowInfo)){
        noValidArgument = false;

        if(REMOTE_HOST_PROPERTIES_SET()){
            if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentShowInfo)){
                UTIL_LOG_CONSOLE(LOG_INFO, "Please provide a show name. Use '--help' to see the help menu.");
            }else{
                if(REMOTE_HOST_PROPERTIES_SET()){
                    error = herder_pullShowInfo(remoteHost, remotePort, argumentShowInfo.value, argumentShowInfo.valueLength);
                    
                    if(error == ERROR_UNKNOWN_SHOW){
                        UTIL_LOG_CONSOLE_(LOG_INFO, "Unknown show '%s'.", argumentShowInfo.value);
                    }else{
                        if(error != ERROR_NO_ERROR){
                            UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to fetch show info for show '%s'. [%s]", argumentShowInfo.value,util_toErrorString(error));
                        }
                    }
                }
            }
        }
    }

    if(argumentParser_contains(&parser, &argumentListShows)){
        noValidArgument = false;

        if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentListShows)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Use '--help' to see the help menu.");
        }else{
            if(REMOTE_HOST_PROPERTIES_SET()){
                if((error = herder_listShows(remoteHost, remotePort)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to fetch list of all shows. [%s]", util_toErrorString(error));
                }
            }
        }
    }

    if(argumentParser_contains(&parser, &argumentAddShow)){
        noValidArgument = false;

        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentAddShow)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Please provide a show name. Use '--help' to see the help menu.");
        }else{
            if(REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = herder_addShow(remoteHost, remotePort, argumentAddShow.value, argumentAddShow.valueLength)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add show '%s' to the library. [%s]", argumentAddShow.value, util_toErrorString(error));
                }else{
                    UTIL_LOG_CONSOLE_(LOG_INFO, "Added '%s' to the library.", argumentAddShow.value);
                }
            }
        }
    }

    if(argumentParser_contains(&parser, &argumentAdd)){
        noValidArgument = false;

        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentAdd)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Use '--help' to see the help menu.");
        }else{
            if(REMOTE_HOST_PROPERTIES_SET() && (propertyFile_propertySet(libraryDirectory, PROPERTY_LIBRARY_DIRECTORY_NAME) == ERROR_NO_ERROR)){
                if(!util_fileExists(argumentAdd.value) || util_isDirectory(argumentAdd.value)){
                    UTIL_LOG_CONSOLE_(LOG_INFO, "ERROR: '%s' is not a falid file.", argumentAdd.value);
                }else{
                    if((error = herder_addEpisode(remoteHost, remotePort, libraryDirectory, argumentAdd.value, argumentAdd.valueLength)) != ERROR_NO_ERROR){
                           UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add '%s' to library. [%s]", argumentAdd.value,  util_toErrorString(error));
                    }
                }
            }
        }
    }

    if(noValidArgument){
        herder_printHelp();
    }

label_free:
	argumentParser_free(&parser);

	closelog();

#undef REMOTE_HOST_PROPERTIES_SET

	return EXIT_SUCCESS;
}

inline void herder_printHelp(void){
    printf("Usage: herder --[command]/-[alias] <arguments>.\n\n");

    printf("\t%s\t\t\t%s\n", "-l, --list", "Fetches and prints a list of all shows in the library.");
    printf("\t%s\t\t\t%s\n", "--info <name>", "Prints all Episodes of the given show.");
    printf("\t%s\t\t%s\n", "-a, --add <file>", "Adds the given file to the library.");
    printf("\t%s\t\t%s\n", "--addShow <name>", "Adds the given show to the library.");
    printf("\t%s\t%s\n", "-i, --import optional:<path>", "Imports all files from the specified import path to the library (See '--setImportDirectory').");
    printf("\t%s\t%s\n", "--setImportDirectory <path>", "Sets the 'import directory' to the given path.");
    printf("\t%s\t%s\n", "--setLibraryDirectory <path>", "Sets the 'library directory' to the given path.");
    printf("\t%s\t\t%s\n", "--setRemoteHost <URL>", "Sets the 'remote host' address to the given URL.");
    printf("\t%s\t%s\n", "--setRemotePort <port>", "Sets the 'remote host' port to the given port.");
    printf("\t%s\t\t\t%s\n", "--killDeamon", "Kills the deamon process running the 'herder' server.");
    printf("\t%s\t\t\t%s\n", "--restartDeamon", "Restarts the deamon process running the 'herder' server.");
    printf("\t%s\t\t%s\n", "-?, -h, -help, --help", "Displays this help.");
}

inline ERROR_CODE herder_listShows(Property* remoteHostProperty, Property* remoteHostPortProperty){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHostProperty->buffer;
    uint_fast16_t port = util_byteArrayTo_uint16(remoteHostPortProperty->buffer);

    ArrayList shows;
    if((error = herder_pullShowList(&shows, host, port)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    if(shows.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO, "Library is empty.");

        goto label_freeShowList;
    }

    ArrayListIterator it;
    arrayList_initIterator(&it, &shows);

    uint_fast64_t i = 0;
    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        char* show = ARRAY_LIST_ITERATOR_NEXT(&it);
        
        UTIL_LOG_CONSOLE_(LOG_INFO, "%02" PRIuFAST64 ":'%s'.", i, show);
        i++;

        free(show);
    }

label_freeShowList:
    arrayList_free(&shows);

    return ERROR(error);
}

ERROR_CODE herder_pullShowList(ArrayList* shows, const char* host, const uint_fast16_t port){
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

    if((error = arrayList_initFixedSizeList(shows, numShows)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    uint_fast64_t i;
    for(i = 0; i < numShows; i++){
        const uint_fast64_t nameLength = util_byteArrayTo_uint64(response.data + readOffset);
        readOffset += sizeof(uint64_t);

        char* name = malloc(sizeof(*name) * (nameLength + 1));

        if(name == NULL){
            error = ERROR_OUT_OF_MEMORY;

            goto label_freeRequest;
        }

        memcpy(name, response.data + readOffset, nameLength);
        name[nameLength] = '\0';

        readOffset += nameLength;

        arrayList_add(shows, name);
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

ERROR_CODE herder_addShow(Property* remoteHost, Property* remotePort, char* showName, const uint_fast64_t showNameLength){
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

local ERROR_CODE herder_pullShowInfo(Property* remoteHost, Property* remotePort, char* showName, const uint_fast64_t showNameLength){
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

    const char url[] = "/showInfo";

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

    printf("%s:\n", showName);

    if(response.statusCode == _200_OK){
        uint_fast64_t readOffset = 0;

        uint_fast64_t numSeasons = util_byteArrayTo_uint64(response.data + readOffset);
        readOffset += sizeof(uint64_t);

        if(numSeasons != 0){                
            for( ; numSeasons > 0; numSeasons--){
                const uint_fast16_t season = util_byteArrayTo_uint16(response.data + readOffset);
                readOffset += sizeof(uint16_t);

                printf("\tSeason: %02" PRIuFAST16 ".\n", season);

                uint_fast64_t numEpisodes = util_byteArrayTo_uint64(response.data + readOffset);
                readOffset += sizeof(uint64_t);

                for( ; numEpisodes > 0; numEpisodes--){
                    const uint_fast16_t episode = util_byteArrayTo_uint16(response.data + readOffset);
                    readOffset += sizeof(uint16_t);

                    uint_fast64_t nameLength = util_byteArrayTo_uint64(response.data + readOffset);
                    readOffset += sizeof(uint64_t);

                    char* name = alloca(sizeof(*name) * (nameLength));
                    memcpy(name, response.data + readOffset, nameLength);
                    readOffset += nameLength;

                    printf("\t\t  -> %02" PRIuFAST16 ": '%s'.\n", episode, name);
                }
            }
        }else{
            UTIL_LOG_CONSOLE(LOG_INFO, "Show is empty.");
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

ERROR_CODE herder_addEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory,  char* filePath, const uint_fast64_t filePathLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    EpisodeInfo* episodeInfo = alloca(sizeof(*episodeInfo));
    if((error = mediaLibrary_initEpisodeInfo(episodeInfo)) != ERROR_NO_ERROR){
        goto label_freeEpisodeInfo;
    }
    
    if((error = herder_extractShowInfo(remoteHost, remotePort, &episodeInfo, filePath, filePathLength)) != ERROR_NO_ERROR){
        goto label_freeEpisodeInfo;
    }

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

    // TODO: Inform server that something went wrong and stuff should be removed from the library in case of failed file copy. (Jan - 2018.11.28)

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

ERROR_CODE herder_extractShowInfo(Property* remoteHost, Property* remotePort, EpisodeInfo** episodeInfo, char* filePath, const uint_fast64_t filePathLength){
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

    const uint_fast64_t fileNameOffset = util_findLast(filePath, filePathLength, '/') + 1;

    char* fileName = filePath + fileNameOffset;
    const uint_fast64_t fileNameLength = filePathLength - fileNameOffset;

    const char url[] = "/extractShowInfo";

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
            (*episodeInfo)->showNameLength = util_byteArrayTo_uint64(response.data + readOffset);
            readOffset += sizeof(uint64_t);

            if((*episodeInfo)->showNameLength != 0){
                (*episodeInfo)->showName = malloc(sizeof(*(*episodeInfo)->showName) * ((*episodeInfo)->showNameLength) + 1);
                strncpy((*episodeInfo)->showName, (char*) (response.data + readOffset), (*episodeInfo)->showNameLength);
                readOffset += (*episodeInfo)->showNameLength;
            }else{
                (*episodeInfo)->showName = NULL;
            }            

            (*episodeInfo)->nameLength = util_byteArrayTo_uint64(response.data + readOffset);
            readOffset += sizeof(uint64_t);

            if((*episodeInfo)->nameLength != 0){
                (*episodeInfo)->name = malloc(sizeof(*(*episodeInfo)->name) * ((*episodeInfo)->nameLength + 1));
                strncpy((*episodeInfo)->name, (char*) (response.data + readOffset), (*episodeInfo)->nameLength);
                readOffset += (*episodeInfo)->nameLength;
            }else{
                (*episodeInfo)->name = NULL;
            }

            (*episodeInfo)->season = util_byteArrayTo_uint16(response.data + readOffset);
            readOffset += sizeof(uint16_t);

            (*episodeInfo)->episode = util_byteArrayTo_uint16(response.data + readOffset);
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

    // TODO: Extract this to its own function . (Jan - 2018.12.07)
    if((*episodeInfo)->showName == NULL){
        UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to extract show name from file: '%s'.", fileName);

        UTIL_LOG_CONSOLE(LOG_INFO, "Please enter the show name...");

       (*episodeInfo)->showName = util_readUserInput();
    }

     if((*episodeInfo)->name == NULL){
        UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to extract episode name from file: '%s'.", fileName);

        UTIL_LOG_CONSOLE(LOG_INFO, "Please enter the episode name...");

        (*episodeInfo)->name = util_readUserInput();
    }
    
    return ERROR(error);
}

#undef HERDER_PROGRAM_NAME

#undef PROPERTY_IMPORT_DIRECTORY_NAME
#undef PROPERTY_REMOTE_HOST_NAME
#undef PROPERTY_REMOTE_PORT_NAME

#undef HTTP_PROCESSING_BUFFER_SIZE