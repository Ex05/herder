// POSIX Version ¢2008
#define _XOPEN_SOURCE 700

// For mmap flags that are not in the POSIX-Standard.
#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <signal.h>

#include "util.c"
#include "linkedList.c"
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
    char* noWhiteSpaceShowName = alloca(sizeof(*noWhiteSpaceShowName) * ((episodeInfo)->showNameLength + 1)); \
    strncpy(noWhiteSpaceShowName, (episodeInfo)->showName, (episodeInfo)->showNameLength + 1); \
    util_replaceAllChars(noWhiteSpaceShowName, ' ', '_'); \
    *stringLength = ((episodeInfo)->showNameLength * 3) + ((episodeInfo)->nameLength) + (3/*" - "*/ + 7/*"Season_"*/ + 2/*"/"*/) + (UTIL_UINT16_STRING_LENGTH * 3); \
     \
    *(path) = alloca(sizeof(**(path)) * (*stringLength + 1)); \
    *stringLength = snprintf(*path, *stringLength, "%s/%s - Season_%02" PRIdFAST16 "/%s_s%02" PRIdFAST16 "e%02" PRIdFAST16 "_%s", noWhiteSpaceShowName, noWhiteSpaceShowName, (episodeInfo)->season, noWhiteSpaceShowName, (episodeInfo)->season, (episodeInfo)->episode, (episodeInfo)->name); \
     \
    util_replaceAllChars(*path + (*stringLength - ((episodeInfo)->showNameLength + (2 * UTIL_UINT16_STRING_LENGTH) + (episodeInfo)->nameLength + 4)), ' ', '_')

#define REMOTE_HOST_PROPERTIES_SET() ((propertyFile_propertySet(remoteHost, PROPERTY_REMOTE_HOST_NAME) == ERROR_NO_ERROR) && (propertyFile_propertySet(remotePort, PROPERTY_REMOTE_PORT_NAME) == ERROR_NO_ERROR))

typedef struct{
    char* path;
    char* fileName;
    uint64_t pathLength;
    uint64_t fileNameLength;
}DirectoryEntry;

local void herder_printHelp(void);

local ERROR_CODE herder_listShows(Property*, Property*);

local ERROR_CODE herder_removeShow(Property*, Property*, char*, const uint_fast64_t);

local ERROR_CODE herder_renameEpisode(Property*, Property*);

local ERROR_CODE herder_listAllShows(Property*, Property*);

local ERROR_CODE herder_pullShowList(LinkedList*, const char*, const uint_fast16_t);

local ERROR_CODE herder_addShow(Property*, Property*, char*, const uint_fast64_t);

local ERROR_CODE herder_printShowInfo(Property*, Property*, char*, const uint_fast64_t);

local ERROR_CODE herder_extractShowInfo(Property*, Property*, EpisodeInfo**, char*, const uint_fast64_t);

local ERROR_CODE herder_addEpisode(Property*, Property*, Property*, char*, const uint_fast64_t);

local ERROR_CODE herder_argumentSetImportDirectory(Argument*, PropertyFile*, Property**);

local ERROR_CODE herder_argumentSetRemoteHost(Argument*, PropertyFile*, Property**);

local ERROR_CODE herder_argumentSetRemoteHostPort(Argument*, PropertyFile*, Property**);

local ERROR_CODE herder_argumentSetLibraryDirectory(Argument*, PropertyFile*, Property**);

local ERROR_CODE herder_argumentImport(Argument*, Property*, Property*, Property*, Property*);

local ERROR_CODE herder_argumentKillDaemon(Argument*);

local ERROR_CODE herder_argumentShowInfo(Argument*, Property*, Property*);

local ERROR_CODE herder_argumentListShows(Argument*, Property*, Property*);

local ERROR_CODE herder_argumentListAll(Argument*, Property*, Property*);

local ERROR_CODE herder_argumentAddShow(Argument*, Property*, Property*);

local ERROR_CODE herder_argumentRemoveShow(Argument*, Property*, Property*);

local ERROR_CODE herder_arguemntRenameEpisode(Argument*, Property*, Property*);

local ERROR_CODE herder_walkDirectory(LinkedList*, const char*);

local ERROR_CODE herder_import(Property*, Property*, Property*, const char*);

local ERROR_CODE herder_pullShowInfo(Property*, Property*, Show*);

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
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveShow, 1, "--removeShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(RenameEpisode, 1, "--renameEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetImportDirectory, 1, "--setImportDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetLibraryDirectory, 1, "--setLibraryDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHost, 1, "--setRemoteHost");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHostPort, 1, "--setRemotePort");
    ARGUMENT_PARSER_ADD_ARGUMENT(KillDeamon, 1, "--killDeamon");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import, 2, "-i", "--import");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListShows, 2, "-l", "--list");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListAll, 2, "--listAll", "--listShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowInfo, 1, "--info");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowSettings, 1, "--showSettings");

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
        if(propertyFile_create(propertyFilePath, 8) != ERROR_NO_ERROR){
            goto label_free;
        }
    }

    PropertyFile properties;
    if(propertyFile_init(&properties, propertyFilePath) != ERROR_NO_ERROR){
        goto label_freeProperties;
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

    if(argumentParser_contains(&parser, &argumentSetImportDirectory)){
        noValidArgument = false;

        if((error = herder_argumentSetImportDirectory(&argumentSetImportDirectory, &properties, &importDirectory)) == ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'.", PROPERTY_IMPORT_DIRECTORY_NAME , (char*) importDirectory->buffer);
        }

        goto label_freeProperties;
    }

	if(argumentParser_contains(&parser, &argumentSetRemoteHost)){
        noValidArgument = false;

        if((error = herder_argumentSetRemoteHost(&argumentSetRemoteHost, &properties, &remoteHost)) == ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'.", PROPERTY_REMOTE_HOST_NAME, (char*) remoteHost->buffer);
        }

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentSetRemoteHostPort)){
        noValidArgument = false;

		if((error = herder_argumentSetRemoteHostPort(&argumentSetRemoteHostPort, &properties, &remotePort)) == ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%" PRIdFAST64 "'.", PROPERTY_REMOTE_PORT_NAME , util_byteArrayTo_uint64(remotePort->buffer));
        }

        goto label_freeProperties;
    }

     if(argumentParser_contains(&parser, &argumentSetLibraryDirectory)){
        noValidArgument = false;

        UTIL_LOG_CONSOLE(LOG_DEBUG, "--setLibraryDirectory.");

        if((error = herder_argumentSetLibraryDirectory(&argumentSetLibraryDirectory, &properties, &libraryDirectory)) == ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'.", PROPERTY_LIBRARY_DIRECTORY_NAME , (char*) libraryDirectory->buffer);
        }

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentShowSettings)){
        noValidArgument = false;

        UTIL_LOG_CONSOLE_(LOG_INFO, "ImportDirectory: %s.", PROPERTY_IS_SET(importDirectory) ? (char*) importDirectory->buffer : "NULL");
        UTIL_LOG_CONSOLE_(LOG_INFO, "RemoteHost: %s.", PROPERTY_IS_SET(remoteHost) ? (char*) remoteHost->buffer : "NULL");
        UTIL_LOG_CONSOLE_(LOG_INFO, "RemotePorrt: %" PRIuFAST64 ".", PROPERTY_IS_SET(remotePort) ? util_byteArrayTo_uint64(remotePort->buffer) : 0);
        UTIL_LOG_CONSOLE_(LOG_INFO, "LibraryDirectory: %s.", PROPERTY_IS_SET(libraryDirectory) ? (char*) libraryDirectory->buffer : "NULL");

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentImport)){
       noValidArgument = false;

        if((error = herder_argumentImport(&argumentImport, remoteHost, remotePort, libraryDirectory, importDirectory)) == ERROR_NO_ERROR){
            // TODO: Add handling of error or success case. (jan - 2018.12.18)
        }

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentKillDeamon)){
        noValidArgument = false;
   
        if((error = herder_argumentKillDaemon(&argumentKillDeamon)) == ERROR_NO_ERROR){
            // TODO: Add handling of error or success case. (jan - 2018.12.18)
        }

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentShowInfo)){
        noValidArgument = false;

        if((error = herder_argumentShowInfo(&argumentShowInfo, remoteHost, remotePort)) != ERROR_NO_ERROR){
            if(error == ERROR_UNKNOWN_SHOW){
                UTIL_LOG_CONSOLE_(LOG_INFO, "Unknown show '%s'.", argumentShowInfo.value);
            }else{
                if(error == ERROR_INVALID_COMMAND_USAGE){
                    UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --info <name>.");
                }else{
                     UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to fetch show info for show '%s'. [%s]", argumentShowInfo.value,util_toErrorString(error));
                    }
            }
        }

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentListShows)){
        noValidArgument = false;

        if((error = herder_argumentListShows(&argumentListShows, remoteHost, remotePort)) != ERROR_NO_ERROR){       
            if(error == ERROR_FAILED_TO_CONNECT){
                UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to list shows. [%s] (%s:%" PRIdFAST16 ")", util_toErrorString(error) , (char*) remoteHost->buffer, util_byteArrayTo_uint64(remotePort->buffer));
            }else{
                UTIL_LOG_CONSOLE_(LOG_ERR, "Unexpected error:'%s'.", util_toErrorString(error));
            }
        }
    }

    if(argumentParser_contains(&parser, &argumentListAll)){
        noValidArgument = false;

        if((error = herder_argumentListAll(&argumentListAll, remoteHost, remotePort)) != ERROR_NO_ERROR){       
            if(error == ERROR_FAILED_TO_CONNECT){
                UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to list all shows. [%s] (%s:%" PRIdFAST16 ")", util_toErrorString(error) , (char*) remoteHost->buffer, util_byteArrayTo_uint64(remotePort->buffer));
            }else{
                UTIL_LOG_CONSOLE_(LOG_ERR, "Unexpected error:'%s'.", util_toErrorString(error));
            }
        }
    }

    if(argumentParser_contains(&parser, &argumentAddShow)){
        noValidArgument = false;

        if((error = herder_argumentAddShow(&argumentAddShow, remoteHost, remotePort)) == ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_INFO, "Added '%s' to the library.", argumentAddShow.value);
        }

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentRemoveShow)){
        noValidArgument = false;

        if((error = herder_argumentRemoveShow(&argumentRemoveShow, remoteHost, remotePort)) == ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_INFO, "Removed '%s' from the library.", argumentRemoveShow.value);
        }else{
            UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to remove '%s' from the library. [%s]", argumentRemoveShow.value, util_toErrorString(error));
        }

        goto label_freeProperties;
    }

    if(argumentParser_contains(&parser, &argumentRenameEpisode)){
        noValidArgument = false;

        if((error = herder_arguemntRenameEpisode(&argumentRenameEpisode, remoteHost, remotePort)) == ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully renamed episode '%s'.", argumentRenameEpisode.value);
        }else{
            UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to rename epsiode. [%s]", util_toErrorString(error));
        }

        goto label_freeProperties;
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
                    }else{
                        UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully added '%s' to the library.", argumentAdd.value);
                    }
                }
            }
        }
    }

    if(noValidArgument){
        herder_printHelp();
    }

label_freeProperties:
    propertyFile_free(&properties);

    if(PROPERTY_IS_SET(libraryDirectory)){
        propertyFile_freeProperty(libraryDirectory);

        free(libraryDirectory);
    }

     if(PROPERTY_IS_SET(remotePort)){
        propertyFile_freeProperty(remotePort);

        free(remotePort);
    }

     if(PROPERTY_IS_SET(remoteHost)){
        propertyFile_freeProperty(remoteHost);

        free(remoteHost);
    }

     if(PROPERTY_IS_SET(importDirectory)){
        propertyFile_freeProperty(importDirectory);

        free(importDirectory);
    }

label_free:
	argumentParser_free(&parser);

	closelog();

	return EXIT_SUCCESS;
}

inline void herder_printHelp(void){
    UTIL_LOG_CONSOLE(LOG_INFO, "Usage: herder --[command]/-[alias] <arguments>.\n\n");

    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", "-l, --list", "Fetches and prints a list of all shows in the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", "--info <name>", "Prints all Episodes of the given show.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", "-a, --add <file>", "Adds the given file to the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", "--addShow <name>", "Adds the given show to the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", "--removeShow <name>", "Removes the given show and all its seasons/episodes from the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t%s", "-i, --import optional:<path>", "Imports all files from the specified import path to the library (See '--setImportDirectory').");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t%s", "--setImportDirectory <path>", "Sets the 'import directory' to the given path.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t%s", "--setLibraryDirectory <path>", "Sets the 'library directory' to the given path.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", "--setRemoteHost <URL>", "Sets the 'remote host' address to the given URL.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", "--setRemotePort <port>", "Sets the 'remote host' port to the given port.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", "--killDeamon", "Kills the deamon process running the 'herder' server.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", "--restartDeamon", "Restarts the deamon process running the 'herder' server.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", "--showSettings", "Prints all available settings and their value.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", "-?, -h, -help, --help", "Displays this help.");
}

inline ERROR_CODE herder_listAllShows(Property* remoteHostProperty, Property* remoteHostPortProperty){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHostProperty->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remoteHostPortProperty->buffer);

    LinkedList shows = {0};
    if((error = herder_pullShowList(&shows, host, port)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    if(shows.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO, "Library is empty.");

        goto label_freeShowList;
    }

    LinkedListIterator it;
    linkedList_initIterator(&it, &shows);

    uint_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        char* show = LINKED_LIST_ITERATOR_NEXT(&it);
        
        if((error = herder_printShowInfo(remoteHostProperty, remoteHostPortProperty, show, strlen(show))) != ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
        }

        i++;

        free(show);
    }

label_freeShowList:
    linkedList_free(&shows);

    return ERROR(error);
}

inline ERROR_CODE herder_listShows(Property* remoteHostProperty, Property* remoteHostPortProperty){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHostProperty->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remoteHostPortProperty->buffer);

    LinkedList shows;
    if((error = herder_pullShowList(&shows, host, port)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    if(shows.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO, "Library is empty.");

        goto label_freeShowList;
    }

    LinkedListIterator it;
    linkedList_initIterator(&it, &shows);

    uint_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        char* show = LINKED_LIST_ITERATOR_NEXT(&it);
        
        UTIL_LOG_CONSOLE_(LOG_INFO, "%02" PRIuFAST64 ":'%s'.", i, show);
        i++;

        free(show);
    }

label_freeShowList:
    linkedList_free(&shows);

    return ERROR(error);
}

ERROR_CODE herder_pullShowList(LinkedList* shows, const char* host, const uint_fast16_t port){
    ERROR_CODE error = ERROR_NO_ERROR;
    
    // Note: We have to zero out shows here to not free garbage values in the ArrayList if we fail to connect, as the initialisation of shows requires the number of shows to be send from the server. (jan - 2019.04.04)
    // memset(shows, 0, sizeof(*shows));

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

        char* name = malloc(sizeof(*name) * (nameLength + 1));

        if(name == NULL){
            error = ERROR_OUT_OF_MEMORY;

            goto label_freeRequest;
        }

        memcpy(name, response.data + readOffset, nameLength);
        name[nameLength] = '\0';

        readOffset += nameLength;

        linkedList_add(shows, name);
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

ERROR_CODE herder_removeShow(Property* remoteHost, Property* remotePort, char* showName, const uint_fast64_t showNameLength){
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
    const uint_fast64_t returnCode = util_byteArrayTo_uint64(response.data);
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

local ERROR_CODE herder_printShowInfo(Property* remoteHost, Property* remotePort, char* showName, const uint_fast64_t showNameLength){
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

    const char url[] = "/showInfo";

    HTTP_Request request;
    if((error = http_initRequest(&request, url, strlen(url), httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_POST)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    HTTP_ADD_HEADER_FIELD(request, Host, host);

    util_uint64ToByteArray(request.data + request.dataLength, showNameLength);
    request.dataLength += sizeof(uint64_t);   

    memcpy(request.data + request.dataLength, showName, showNameLength + 1);
    request.dataLength += showNameLength + 1;

    HTTP_Response response;
    http_initResponse(&response, httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE);
    if((error = http_sendRequest(&request, &response, socketFD)) != ERROR_NO_ERROR){
        goto label_freeResponse;
    }

    http_closeConnection(socketFD);

    UTIL_LOG_CONSOLE_(LOG_INFO, "%s:", showName);

    if(response.statusCode == _200_OK){
        uint_fast64_t readOffset = 0;

        // Num_Seasons.
        uint_fast64_t numSeasons = util_byteArrayTo_uint64(response.data + readOffset);
        readOffset += sizeof(uint64_t);

        if(numSeasons != 0){                
            for( ; numSeasons > 0; numSeasons--){
                // Season_Number.
                const uint_fast16_t season = util_byteArrayTo_uint16(response.data + readOffset);
                readOffset += sizeof(uint16_t);

                UTIL_LOG_CONSOLE_(LOG_INFO, "\tSeason: %02" PRIuFAST16 ".", season);

                // Num_Episodes.
                uint_fast64_t numEpisodes = util_byteArrayTo_uint64(response.data + readOffset);
                readOffset += sizeof(uint64_t);

                for( ; numEpisodes > 0; numEpisodes--){
                    // Episode_Number.
                    const uint_fast16_t episode = util_byteArrayTo_uint16(response.data + readOffset);
                    readOffset += sizeof(uint16_t);

                    // Episode_NameLength.
                    uint_fast64_t nameLength = util_byteArrayTo_uint64(response.data + readOffset);
                    readOffset += sizeof(uint64_t);

                    // Episode_Name.
                    char* name = alloca(sizeof(*name) * (nameLength + 1));
                    memcpy(name, response.data + readOffset, nameLength + 1);
                    readOffset += nameLength + 1;

                    UTIL_LOG_CONSOLE_(LOG_INFO, "\t\t  -> %02" PRIuFAST16 ": '%s'.", episode, name);
                }
            }
        }else{
            UTIL_LOG_CONSOLE(LOG_INFO, "\tShow is empty.");
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

ERROR_CODE herder_extractShowInfo(Property* remoteHost, Property* remotePort, EpisodeInfo** episodeInfo, char* filePath, const uint_fast64_t filePathLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHost->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remotePort->buffer);

    void* httpProcessingBuffer;
    char* showName = NULL;
    uint_fast64_t showNameLength = 0;
label_extractShowInfo:
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

    if((error = util_getFileExtension(&((*episodeInfo)->fileExtension), filePath, filePathLength)) != ERROR_NO_ERROR){
        goto label_unMap;
    }

    (*episodeInfo)->fileExtensionLength = strlen((*episodeInfo)->fileExtension);

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
                if((*episodeInfo)->showName == NULL){
                    return  ERROR(ERROR_OUT_OF_MEMORY);
                }

                strncpy((*episodeInfo)->showName, (char*) (response.data + readOffset), (*episodeInfo)->showNameLength);
                (*episodeInfo)->showName[(*episodeInfo)->showNameLength] = '\0';

                readOffset += (*episodeInfo)->showNameLength;
            }else{
                (*episodeInfo)->showName = NULL;
            }            

            (*episodeInfo)->nameLength = util_byteArrayTo_uint64(response.data + readOffset);
            readOffset += sizeof(uint64_t);

            if((*episodeInfo)->nameLength != 0){
                (*episodeInfo)->name = malloc(sizeof(*(*episodeInfo)->name) * ((*episodeInfo)->nameLength + 1));

                 if((*episodeInfo)->name == NULL){
                    return  ERROR(ERROR_OUT_OF_MEMORY);
                }

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

    if(showName != NULL && showNameLength != 0){
        free((*episodeInfo)->showName);

        (*episodeInfo)->showName = showName;
        (*episodeInfo)->showNameLength = showNameLength;
    }

    // TODO: Extract this to its own function . (jan - 2018.12.07)
    if((*episodeInfo)->showName == NULL){
        UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to extract show name from file: '%s'.", fileName);

        UTIL_LOG_CONSOLE(LOG_INFO, "Please enter the show name...");

        int_fast64_t userInputLength;
        if((error = util_readUserInput(&showName, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        showNameLength = userInputLength;

        if((error = herder_addShow(remoteHost, remotePort, showName, userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }
        
        http_freeHTTP_Request(&request);
        http_freeHTTP_Response(&response);

        if(util_unMap(httpProcessingBuffer, HTTP_PROCESSING_BUFFER_SIZE) != ERROR_NO_ERROR){
            UTIL_LOG_ERROR(util_toErrorString(ERROR_FAILED_TO_UNMAP_MEMORY));
        }

        mediaLibrary_freeEpisodeInfo(*episodeInfo);

        goto label_extractShowInfo;
    }

    if((*episodeInfo)->season == 0){
        UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to extract season number from file: '%s'.", fileName);

        UTIL_LOG_CONSOLE(LOG_INFO, "Please enter the season number...");

        char* seasonNumber;
        int_fast64_t userInputLength;
        if((error = util_readUserInput(&seasonNumber, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        if((error = util_stringToInt(seasonNumber, &(*episodeInfo)->season)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }
    }

    if((*episodeInfo)->episode == 0){
        UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to extract episode number from file: '%s'.", fileName);

        UTIL_LOG_CONSOLE(LOG_INFO, "Please enter the episode number...");

        char* eoisodeNumber;
        int_fast64_t userInputLength;
        if((error = util_readUserInput(&eoisodeNumber, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }


        if((error = util_stringToInt(eoisodeNumber, &(*episodeInfo)->episode)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }
    }

    if((*episodeInfo)->name == NULL){
        UTIL_LOG_CONSOLE_(LOG_INFO, "Failed to extract episode name from file: '%s'.", fileName);

        UTIL_LOG_CONSOLE(LOG_INFO, "Please enter the episode name...");

        int_fast64_t userInputLength;
        if((error = util_readUserInput(&(*episodeInfo)->name, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        (*episodeInfo)->nameLength = userInputLength;
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Are these values correct? Yes/No.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\tShow:'%s'.", (*episodeInfo)->showName);
    UTIL_LOG_CONSOLE_(LOG_INFO, "\tSeason:'%" PRIdFAST16 "'.", (*episodeInfo)->season);
    UTIL_LOG_CONSOLE_(LOG_INFO, "\tEpisode:'%" PRIdFAST16 "'.", (*episodeInfo)->episode);
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t\t'%s'.", (*episodeInfo)->name);

    int_fast64_t userInputLength;
    char* userInput;    
label_invalidUserInput:    
    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_freeRequest;
    }

    util_toLowerChase(userInput);

    if(userInputLength != 0 && strncmp("no", userInput, userInputLength) == 0){
        // Show name.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Show name:'%s'. Press <Enter> to accept.", (*episodeInfo)->showName);

        char* showName;
        if((error = util_readUserInput(&showName, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        if(userInputLength != 0){
            free((*episodeInfo)->showName);

            (*episodeInfo)->showName = showName;
            (*episodeInfo)->showNameLength = userInputLength;
        }else{
            free(showName);
        }

    label_seasonNumber:
        // Season number.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Season:'%" PRIiFAST16 "'. Press <Enter> to accept.", (*episodeInfo)->season);

        char* season;
        if((error = util_readUserInput(&season, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        if(userInputLength != 0){
            if((error = util_stringToInt(season, &(*episodeInfo)->season)) != ERROR_NO_ERROR){
                free(season);

                UTIL_LOG_CONSOLE_(LOG_ERR, "%s.", util_toErrorString(error));

                goto label_seasonNumber;
            }
        }

        free(season);

        label_episodeNumber:
        // Episode number.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Episode:'%" PRIiFAST16 "'. Press <Enter> to accept.", (*episodeInfo)->episode);

        char* episode;
        if((error = util_readUserInput(&episode, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        if(userInputLength != 0){
            if((error = util_stringToInt(episode, &(*episodeInfo)->episode)) != ERROR_NO_ERROR){
                free(episode);

                UTIL_LOG_CONSOLE_(LOG_ERR, "%s.", util_toErrorString(error));

                goto label_episodeNumber;
            }
        }

        free(episode);

        // Episode name.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Episode name:'%s'. Press <Enter> to accept.", (*episodeInfo)->name);

        char* episodeName;
        if((error = util_readUserInput(&episodeName, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeRequest;
        }

        if(userInputLength != 0){
            free((*episodeInfo)->name);

            (*episodeInfo)->name = episodeName;
            (*episodeInfo)->nameLength = userInputLength;
        }
    }else{
        if(userInputLength != 0 || strncmp("yes", userInput, userInputLength) != 0){
            UTIL_LOG_CONSOLE_(LOG_INFO, "'%s' is not a valid answer, please type Yes/No.", userInput);

            free(userInput);

            goto label_invalidUserInput;
        }
    }

    free(userInput);
    
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

inline ERROR_CODE herder_argumentSetImportDirectory(Argument* argumentSetImportDirectory, PropertyFile* propertyFile, Property** importDirectory){
    // Note: Make sure 'slashTerminated' is clamped to '0 - 1' so we can use it later to add/subtract depending on wether the string was slash termianted or not. (jan - 2018.10.20)
    const bool slashTerminated = (argumentSetImportDirectory->value[argumentSetImportDirectory->valueLength - 1] == '/') & 0x01;

    const uint_fast64_t importDirectoryLength = argumentSetImportDirectory->valueLength + !slashTerminated;

    char* importDirectoryString;
    if(slashTerminated){
        importDirectoryString = argumentSetImportDirectory->value;
    }else{
        importDirectoryString = alloca(sizeof(*importDirectoryString) * (importDirectoryLength + 1));
        memcpy(importDirectoryString, argumentSetImportDirectory->value, argumentSetImportDirectory->valueLength);
        importDirectoryString[importDirectoryLength - 1] = '/';
        importDirectoryString[importDirectoryLength] = '\0';
    }

    if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentSetImportDirectory))){
        if(PROPERTY_IS_NOT_SET(*importDirectory)){
            if(propertyFile_addProperty(propertyFile, importDirectory, PROPERTY_IMPORT_DIRECTORY_NAME, importDirectoryLength + 1) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_IMPORT_DIRECTORY_NAME);
            }
        }else{
            if((*importDirectory)->entry->length != importDirectoryLength + 1){
                if(propertyFile_removeProperty(*importDirectory) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", PROPERTY_IMPORT_DIRECTORY_NAME);
                }

                if(propertyFile_addProperty(propertyFile, importDirectory, PROPERTY_IMPORT_DIRECTORY_NAME, importDirectoryLength + 1) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_IMPORT_DIRECTORY_NAME);
                }
            }
        }

        if(propertyFile_setBuffer(*importDirectory, (int8_t*) importDirectoryString) != ERROR_NO_ERROR){
            return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", PROPERTY_IMPORT_DIRECTORY_NAME);
        }
    }else{
        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setImportDirectory <path>.");

        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE herder_argumentSetRemoteHost(Argument* argumentSetRemoteHost, PropertyFile* propertyFile, Property** remoteHost){
    if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentSetRemoteHost))){
        if(PROPERTY_IS_NOT_SET(*remoteHost)){
            if(propertyFile_addProperty(propertyFile, remoteHost, PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost->valueLength + 1) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_REMOTE_HOST_NAME);
            }
        }else{
            if((*remoteHost)->entry->length != argumentSetRemoteHost->valueLength + 1){
                if(propertyFile_removeProperty(*remoteHost) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", PROPERTY_REMOTE_HOST_NAME);
                }

                if(propertyFile_addProperty(propertyFile, remoteHost, PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost->valueLength + 1) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_REMOTE_HOST_NAME);
                }
            }
        }

        if(propertyFile_setBuffer(*remoteHost, (int8_t*) argumentSetRemoteHost->value) != ERROR_NO_ERROR){
            return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", PROPERTY_REMOTE_HOST_NAME);
        }
    }else{
        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setRemoteHost <URL>.");

        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }

    return ERROR(ERROR_NO_ERROR);    
}

inline ERROR_CODE herder_argumentSetRemoteHostPort(Argument* argumentSetRemoteHostPort, PropertyFile* propertyFile, Property** remotePort){
    if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentSetRemoteHostPort))){
        if(PROPERTY_IS_NOT_SET(*remotePort)){
            if(propertyFile_addProperty(propertyFile, remotePort, PROPERTY_REMOTE_PORT_NAME, sizeof(uint64_t)) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_REMOTE_PORT_NAME);
            }
        }else{
            if((*remotePort)->entry->length != sizeof(uint64_t)){
                if(propertyFile_removeProperty(*remotePort) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", PROPERTY_REMOTE_PORT_NAME);
                }

                if(propertyFile_addProperty(propertyFile, remotePort, PROPERTY_REMOTE_PORT_NAME, sizeof(uint64_t)) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_REMOTE_PORT_NAME);
                }
            }
        }

        int64_t port;
        ERROR_CODE error;
        if((error = util_stringToInt(argumentSetRemoteHostPort->value, &port)) != ERROR_NO_ERROR){
            return ERROR(error);
        }else{
            if(port <= 0 || port > UINT16_MAX){
                return ERROR_(ERROR_INVALID_VALUE, "Port value must be in range of 0-%" PRIu16 ".", UINT16_MAX);
            }
        }

        int8_t* buffer = alloca(sizeof(uint64_t));
        util_uint64ToByteArray(buffer, port);

        if(propertyFile_setBuffer(*remotePort, buffer) != ERROR_NO_ERROR){
            return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", PROPERTY_REMOTE_PORT_NAME);
        }
    }else{
        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setRemotePort <port>.");

        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }

   return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE herder_argumentSetLibraryDirectory(Argument* argumentSetLibraryDirectory, PropertyFile* propertyFile, Property** libraryDirectory){
    // Note: Make sure 'slashTerminated' is clamped to '0 - 1' so we can use it later to add/subtract depending on wether the string was slash termianted or not. (jan - 2018.10.20)
    const bool slashTerminated = (argumentSetLibraryDirectory->value[argumentSetLibraryDirectory->valueLength - 1] == '/') & 0x01;

    const uint_fast64_t libraryDirectoryLength = argumentSetLibraryDirectory->valueLength + !slashTerminated;

    char* libraryDirectoryString;
    if(slashTerminated){
        libraryDirectoryString = argumentSetLibraryDirectory->value;
    }else{
        libraryDirectoryString = alloca(sizeof(*libraryDirectoryString) * (libraryDirectoryLength + 1));
        memcpy(libraryDirectoryString, argumentSetLibraryDirectory->value, argumentSetLibraryDirectory->valueLength);
        libraryDirectoryString[libraryDirectoryLength - 1] = '/';
        libraryDirectoryString[libraryDirectoryLength] = '\0';
    }

    if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentSetLibraryDirectory))){
        if(PROPERTY_IS_NOT_SET(*libraryDirectory)){
            if(propertyFile_addProperty(propertyFile, libraryDirectory, PROPERTY_LIBRARY_DIRECTORY_NAME, libraryDirectoryLength + 1) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_LIBRARY_DIRECTORY_NAME);
            }
        }else{
            if((*libraryDirectory)->entry->length != libraryDirectoryLength + 1){
                if(propertyFile_removeProperty(*libraryDirectory) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", PROPERTY_LIBRARY_DIRECTORY_NAME);
                }

                if(propertyFile_addProperty(propertyFile, libraryDirectory, PROPERTY_LIBRARY_DIRECTORY_NAME, libraryDirectoryLength + 1) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", PROPERTY_LIBRARY_DIRECTORY_NAME);
                }
            }
        }

        if(propertyFile_setBuffer(*libraryDirectory, (int8_t*) libraryDirectoryString) != ERROR_NO_ERROR){
            return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", PROPERTY_LIBRARY_DIRECTORY_NAME);
        }
    }else{
        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --setImportDirectory <path>.");

        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }

    return ERROR(ERROR_NO_ERROR);
}

// -i, --import optional:<path>
inline ERROR_CODE herder_argumentImport(Argument* argumentImport, Property* remoteHost, Property* remotePort, Property* libraryDirectory, Property* importDirectory){
    if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentImport))){
        // Import from path.
        return ERROR(herder_import(remoteHost, remotePort, libraryDirectory, argumentImport->value));
    }else{
        if(PROPERTY_IS_SET(importDirectory)){
            // Import from importDirectory_setting.
            return ERROR(herder_import(remoteHost, remotePort, libraryDirectory, (char*) importDirectory->buffer));
        }else{
            UTIL_LOG_CONSOLE(LOG_ERR, "Import directory not set, Use '--setImportDirectory <path>' to set an import directory, or specify the directory to import from when running '-i, --import optional:<path>'.");

            return ERROR_(ERROR_PROPERTY_NOT_SET, "%s", PROPERTY_IMPORT_DIRECTORY_NAME);
        }
    }
}

inline ERROR_CODE herder_argumentKillDaemon(Argument* argumentImport){
    return ERROR(ERROR_FUNCTION_NOT_IMPLEMENTED);
}

inline ERROR_CODE herder_argumentShowInfo(Argument* argumentShowInfo, Property* remoteHost, Property* remotePort){
    if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentShowInfo))){
        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }else{
        if(REMOTE_HOST_PROPERTIES_SET()){
            return ERROR(herder_printShowInfo(remoteHost, remotePort, argumentShowInfo->value, argumentShowInfo->valueLength));
        }else{
            return ERROR(ERROR_PROPERTY_NOT_SET);
        }
    }
}
  
inline ERROR_CODE herder_argumentListShows(Argument* argumentListShows, Property* remoteHost, Property* remotePort){
     if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentListShows))){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage -l, --list");

            return ERROR(ERROR_INVALID_COMMAND_USAGE);
        }else{
            if(REMOTE_HOST_PROPERTIES_SET()){
                return ERROR(herder_listShows(remoteHost, remotePort));
            }else{
                return ERROR(ERROR_PROPERTY_NOT_SET);
            }
        }
}

inline ERROR_CODE herder_argumentListAll(Argument* argumentListShows, Property* remoteHost, Property* remotePort){
     if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentListShows))){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --listAll, --listShows.");

            return ERROR(ERROR_INVALID_COMMAND_USAGE);
        }else{
            if(REMOTE_HOST_PROPERTIES_SET()){
                return ERROR(herder_listAllShows(remoteHost, remotePort));
            }else{
                return ERROR(ERROR_PROPERTY_NOT_SET);
            }
        }
}

inline ERROR_CODE herder_argumentAddShow(Argument* argumentAddShow, Property* remoteHost, Property* remotePort){
    if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentAddShow))){
        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage -a, --add <file>.");

        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }else{
        if(REMOTE_HOST_PROPERTIES_SET()){
            return ERROR(herder_addShow(remoteHost, remotePort, argumentAddShow->value, argumentAddShow->valueLength));
        }else{
            return ERROR(ERROR_PROPERTY_NOT_SET);
        }
    }
}

inline ERROR_CODE herder_argumentRemoveShow(Argument* argumentRemoveShow, Property* remoteHost, Property* remotePort){
    if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentRemoveShow))){
        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --removeShow <show>.");

        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }else{
        if(REMOTE_HOST_PROPERTIES_SET()){
            return ERROR(herder_removeShow(remoteHost, remotePort, argumentRemoveShow->value, argumentRemoveShow->valueLength));
        }else{
            return ERROR(ERROR_PROPERTY_NOT_SET);
        }
    }
}

inline ERROR_CODE herder_arguemntRenameEpisode(Argument* argumentRenameEpisode, Property* remoteHost, Property* remotePort){
    if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((*argumentRenameEpisode))){
        UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage --renameEpisode.");

        return ERROR(ERROR_INVALID_COMMAND_USAGE);
    }else{
        if(REMOTE_HOST_PROPERTIES_SET()){
            return ERROR(herder_renameEpisode(remoteHost, remotePort));
        }else{
            return ERROR(ERROR_PROPERTY_NOT_SET);
        }
    }
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

ERROR_CODE herder_import(Property* remoteHost, Property* remotePort, Property* libraryDirectory, const char* directory){
    ERROR_CODE error;

     if(!REMOTE_HOST_PROPERTIES_SET() || PROPERTY_IS_NOT_SET(libraryDirectory)){
        return ERROR(ERROR_PROPERTY_NOT_SET);
     }

    LinkedList files;
    if((error = linkedList_init(&files)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = herder_walkDirectory(&files, directory)) != ERROR_NO_ERROR){
        goto label_return;
    }

    LinkedListIterator it;
    linkedList_initIterator(&it, &files);

    if(LINKED_LIST_IS_EMPTY(&files)){
        UTIL_LOG_CONSOLE_(LOG_INFO, "No files recorgnised for import in '%s'.", directory);
    }

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        DirectoryEntry* entry = LINKED_LIST_ITERATOR_NEXT(&it);

        UTIL_LOG_CONSOLE_(LOG_INFO, "Adding '%s'.", entry->path);

        if((error = herder_addEpisode(remoteHost, remotePort, libraryDirectory, entry->path, entry->pathLength)) != ERROR_NO_ERROR){
            free(entry->path);
            free(entry);

            goto label_freeFiles;
        }else{
            UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully added '%s'. to library.%s", entry->path, LINKED_LIST_ITERATOR_HAS_NEXT(&it) ? "\n" : "");
        }

        free(entry->path);
        free(entry);
    }

    // TODO: Only delete directories inside import the import directory. (jan - 2019.05.28)
    if((error = util_deleteDirectory(directory, true, true)) != ERROR_NO_ERROR){
        goto label_freeFiles;
    }
    
label_freeFiles:
    linkedList_free(&files);

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

#undef HERDER_PROGRAM_NAME

#undef REMOTE_HOST_PROPERTIES_SET

#undef PROPERTY_IMPORT_DIRECTORY_NAME
#undef PROPERTY_REMOTE_HOST_NAME
#undef PROPERTY_REMOTE_PORT_NAME

#undef HTTP_PROCESSING_BUFFER_SIZE