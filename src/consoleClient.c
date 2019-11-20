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
#include "herder.c"

#define CONSOLE_CLIENT_PROGRAM_NAME "herder"

#define CONSOLE_CLIENT_USAGE_ARGUMENT_HELP "-?, -h, --help"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_SHOW "--addShow <name>."
#define CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_SHOW "--removeShow <name>."
#define CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_EPISODE "-a, --add, --addFile, --addEpisode <file>."
#define CONSOLE_CLIENT_USAGE_ARGUMENT_IMPORT "-i, --import"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME_EPISODE "--renameEpisode <old_name> <new_name>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_EPISODE "--removeEpisode <name>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_SHOWS "-l, --list"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_ALL_SHOWS "--listAll, --listAllShows"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_INFO "--showInfo <name>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_IMPORT_DIRECTORY "--setImportDirectory <path>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_LIBRARY_DIRECTORY "--setLibraryDirectory <path>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST "--setRemoteHost <port>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_PORT "--setRemotePort <port>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_SETTINGS "--showSettings"

#define CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME "importDirectory"
#define CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME "remoteHost"
#define CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME "remotePort"
#define CONSOLE_CLIENT_PROPERTY_LIBRARY_DIRECTORY_NAME "libraryDirectory"

#define CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET() ((propertyFile_propertySet(remoteHost, CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME) == ERROR_NO_ERROR) && (propertyFile_propertySet(remotePort, CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME) == ERROR_NO_ERROR))

local void consoleClient_printHelp(void);

local ERROR_CODE consoleClient_listShows(Property*, Property*);

local ERROR_CODE consoleClient_import(Property*, Property*, Property*, const char*);

local ERROR_CODE consoleClient_listAllShows(Property*, Property*);

local ERROR_CODE consoleClient_printShowInfo(Property*, Property*, const char*, const uint_fast64_t);

// TODO:(jan) Make custom entry point for the client build, so we won't have to use 'main'.
#ifndef TEST_BUILD
	int main(const int argc, const char** argv){
#else
    int consoleClient_totalyNotMain(const int argc, const char** argv){
#endif
    openlog(CONSOLE_CLIENT_PROGRAM_NAME, LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_USER);

    ArgumentParser parser;
    argumentParser_init(&parser);
    
    ARGUMENT_PARSER_ADD_ARGUMENT(Help, 3, "-?", "-h", "--help");
    ARGUMENT_PARSER_ADD_ARGUMENT(AddShow, 1, "--addShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveShow, 1, "--removeShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(Add, 4, "-a", "-add", "--addFile", "--addEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import, 2, "-i", "--import");
    ARGUMENT_PARSER_ADD_ARGUMENT(RenameEpisode, 1, "--renameEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveEpisode, 1, "--removeEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListShows, 3, "-l", "--list", "--listShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListAll, 2, "--listAll", "--listAllShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowInfo, 2, "--showInfo", "--info");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetImportDirectory, 1, "--setImportDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetLibraryDirectory, 1, "--setLibraryDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHost, 1, "--setRemoteHost");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHostPort, 1, "--setRemotePort");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowSettings, 1, "--showSettings");

    ERROR_CODE error;
    if((error = argumentParser_parse(&parser, argc, argv)) != ERROR_NO_ERROR){
        if(error == ERROR_NO_VALID_ARGUMENT){
            UTIL_LOG_CONSOLE(LOG_ERR, "No valid command line arguments.\nUse '" CONSOLE_CLIENT_USAGE_ARGUMENT_HELP "' to display a help message.");    

            goto label_free;
        }else{
            if(error == ERROR_DUPLICATE_ENTRY){
                UTIL_LOG_CONSOLE(LOG_ERR, "Duplicate argument.");
            }
        }

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
    propertyFile_getProperty(&properties, &importDirectory, CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);

    Property* remoteHost;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    propertyFile_getProperty(&properties, &remoteHost, CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME);

    Property* remotePort;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    propertyFile_getProperty(&properties, &remotePort, CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME);

    Property* libraryDirectory;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    propertyFile_getProperty(&properties, &libraryDirectory, CONSOLE_CLIENT_PROPERTY_LIBRARY_DIRECTORY_NAME);

    // --help.
    if(argumentParser_contains(&parser, &argumentHelp)){ 
        goto label_printHelp;
    }

    // --addShow.
    if(argumentParser_contains(&parser, &argumentAddShow)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentAddShow)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_SHOW);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = herder_addShow(remoteHost, remotePort, argumentAddShow.value, argumentAddShow.valueLength)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add show. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    }

    // --removeShow.
    if(argumentParser_contains(&parser, &argumentRemoveShow)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentRemoveShow)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_SHOW);
        }else{
             if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                 ERROR_CODE error;
                if((error = herder_removeShow(remoteHost, remotePort, argumentRemoveShow.value, argumentRemoveShow.valueLength)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to remove show. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    } 

    // --addEpisode.
    if(argumentParser_contains(&parser, &argumentAdd)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentAdd)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_EPISODE);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET() && (propertyFile_propertySet(libraryDirectory, CONSOLE_CLIENT_PROPERTY_LIBRARY_DIRECTORY_NAME) == ERROR_NO_ERROR)){
                if(!util_fileExists(argumentAdd.value) || util_isDirectory(argumentAdd.value)){
                    UTIL_LOG_CONSOLE_(LOG_INFO, "ERROR: '%s' is not a falid file.", argumentAdd.value);
                }else{
                    if((error = herder_addEpisode(remoteHost, remotePort, libraryDirectory, argumentAdd.value, argumentAdd.valueLength)) != ERROR_NO_ERROR){
                           UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add '%s' to library. [%s]", argumentAdd.value,  util_toErrorString(error));
                    }else{
                        UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully added '%s' to the library.", argumentAdd.value);
                    }
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }
        
        goto label_freeProperties;
    }

    // --import.
    if(argumentParser_contains(&parser, &argumentImport)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentImport)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_IMPORT);
        }else{
            ERROR_CODE error;
            if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((argumentImport))){
                // Import from path.
                if((error = consoleClient_import(remoteHost, remotePort, libraryDirectory, argumentImport.value)) != ERROR_NO_ERROR){
                    // 
                }
            }else{
                if(PROPERTY_IS_SET(importDirectory)){
                    // Import from importDirectory_setting.
                    if((error = consoleClient_import(remoteHost, remotePort, libraryDirectory, (char*) importDirectory->buffer)) != ERROR_NO_ERROR){
                        // 
                    }
                }else{
                    UTIL_LOG_CONSOLE(LOG_ERR, "Import directory not set, Use '--setImportDirectory <path>' to set an import directory, or specify the directory to import from when running '-i, --import optional:<path>'.");
                }
            }
        }
        
        goto label_freeProperties;
    }

    // --renameEpisode.
    if(argumentParser_contains(&parser, &argumentRenameEpisode)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentRenameEpisode)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME_EPISODE);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = herder_renameEpisode(remoteHost, remotePort)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to rename Episode. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }
        
        goto label_freeProperties;
    }

    // --removeEpisode.
    if(argumentParser_contains(&parser, &argumentRemoveEpisode)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentRemoveEpisode)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_EPISODE);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                UTIL_LOG_CONSOLE(LOG_CRIT, util_toErrorString(ERROR_FUNCTION_NOT_IMPLEMENTED));
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }
        
        goto label_freeProperties;
    }

    // --listShows.
    if(argumentParser_contains(&parser, &argumentListShows)){ 
        if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentListShows)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_SHOWS);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = consoleClient_listShows(remoteHost, remotePort)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to list shows. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }
        
        goto label_freeProperties;
    }

    // --listAll.
    if(argumentParser_contains(&parser, &argumentListAll)){ 
        if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentListAll)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_ALL_SHOWS);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = consoleClient_listAllShows(remoteHost, remotePort)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to list all shows. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }
        
        goto label_freeProperties;
    }

    // --showInfo.
    if(argumentParser_contains(&parser, &argumentShowInfo)){
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentShowInfo)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_INFO);
        }else{
             if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                 if((error = consoleClient_printShowInfo(remoteHost, remotePort, argumentShowInfo.value, argumentShowInfo.valueLength))){
                     UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));

                     goto label_freeProperties;
                 }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }
        
        goto label_freeProperties;
    }

    // --setImportDirectory.
    if(argumentParser_contains(&parser, &argumentSetImportDirectory)){
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetImportDirectory)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_IMPORT_DIRECTORY);
        }else{
            // Note: Make sure 'slashTerminated' is clamped to '0 - 1' so we can use it later to add/subtract depending on whether the string was slash termianted or not. (jan - 2018.10.20)
            const bool slashTerminated = (argumentSetImportDirectory.value[argumentSetImportDirectory.valueLength - 1] == '/') & 0x01;

            const uint_fast64_t importDirectoryLength = argumentSetImportDirectory.valueLength + !slashTerminated;

            char* importDirectoryString;
            if(slashTerminated){
                importDirectoryString = alloca(sizeof(*importDirectoryString) * (argumentSetImportDirectory.valueLength + 1));
                memmove(importDirectoryString, argumentSetImportDirectory.value, argumentSetImportDirectory.valueLength + 1);

            }else{
                importDirectoryString = alloca(sizeof(*importDirectoryString) * (importDirectoryLength + 1));
                memcpy(importDirectoryString, argumentSetImportDirectory.value, argumentSetImportDirectory.valueLength);
                importDirectoryString[importDirectoryLength - 1] = '/';
                importDirectoryString[importDirectoryLength] = '\0';
            } 

            if(PROPERTY_IS_NOT_SET(importDirectory)){
                if(propertyFile_addProperty(&properties, &importDirectory, CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME, importDirectoryLength + 1) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
                }
            }else{
                if(importDirectory->entry->length != importDirectoryLength + 1){
                    if(propertyFile_removeProperty(importDirectory) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
                    }

                    if(propertyFile_addProperty(&properties, &importDirectory, CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME, importDirectoryLength + 1) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
                    }
                }
            }

            if(propertyFile_setBuffer(importDirectory, (int8_t*) importDirectoryString) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
            }
        }
        
        goto label_freeProperties;
    }

    // --setLibraryDirectory.
    if(argumentParser_contains(&parser, &argumentSetLibraryDirectory)){
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetLibraryDirectory)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_LIBRARY_DIRECTORY);
        }else{
             // Note: Make sure 'slashTerminated' is clamped to '0 - 1' so we can use it later to add/subtract depending on wether the string was slash termianted or not. (jan - 2018.10.20)
            const bool slashTerminated = (argumentSetLibraryDirectory.value[argumentSetLibraryDirectory.valueLength - 1] == '/') & 0x01;

            const uint_fast64_t libraryDirectoryLength = argumentSetLibraryDirectory.valueLength + !slashTerminated;

            char* libraryDirectoryString;
            if(slashTerminated){
                libraryDirectoryString = alloca(sizeof(*libraryDirectoryString) * (argumentSetLibraryDirectory.valueLength + 1));
                memmove(libraryDirectoryString, argumentSetLibraryDirectory.value, argumentSetLibraryDirectory.valueLength + 1);
            }else{
                libraryDirectoryString = alloca(sizeof(*libraryDirectory) * (libraryDirectoryLength + 1));
                memcpy(libraryDirectoryString, argumentSetLibraryDirectory.value, argumentSetLibraryDirectory.valueLength);
                libraryDirectoryString[libraryDirectoryLength - 1] = '/';
                libraryDirectoryString[libraryDirectoryLength] = '\0';
            } 

            if(PROPERTY_IS_NOT_SET(libraryDirectory)){
                if(propertyFile_addProperty(&properties, &libraryDirectory, CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME, libraryDirectoryLength + 1) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
                }
            }else{
                if(libraryDirectory->entry->length != libraryDirectoryLength + 1){
                    if(propertyFile_removeProperty(libraryDirectory) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
                    }

                    if(propertyFile_addProperty(&properties, &libraryDirectory, CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME, libraryDirectoryLength + 1) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
                    }
                }
            }

            if(propertyFile_setBuffer(libraryDirectory, (int8_t*) libraryDirectoryString) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME);
            }
        }
        
        goto label_freeProperties;
    }

    // --setRemoteHost.
    if(argumentParser_contains(&parser, &argumentSetRemoteHost)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetRemoteHost)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST);
        }else{
            if(PROPERTY_IS_NOT_SET(remoteHost)){
                if(propertyFile_addProperty(&properties, &remoteHost, CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost.valueLength + 1) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME);
                }
            }else{
                if(remoteHost->entry->length != argumentSetRemoteHost.valueLength + 1){
                    if(propertyFile_removeProperty(remoteHost) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME);
                    }

                    if(propertyFile_addProperty(&properties, &remoteHost, CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost.valueLength + 1) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME);
                    }
                }
            }

            if(propertyFile_setBuffer(remoteHost, (int8_t*) argumentSetRemoteHost.value) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME);
            }
        }
        
        goto label_freeProperties;
    }

    // --setRemotePort.
    if(argumentParser_contains(&parser, &argumentSetRemoteHostPort)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetRemoteHostPort)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_PORT);
        }else{
            if(PROPERTY_IS_NOT_SET(remotePort)){
                if(propertyFile_addProperty(&properties, &remotePort, CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME, argumentSetRemoteHostPort.valueLength + 1) != ERROR_NO_ERROR){
                    return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME);
                }
            }else{
                if(remotePort->entry->length != argumentSetRemoteHostPort.valueLength + 1){
                    if(propertyFile_removeProperty(remoteHost) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_REMOVE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME);
                    }

                    if(propertyFile_addProperty(&properties, &remotePort, CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME, argumentSetRemoteHostPort.valueLength + 1) != ERROR_NO_ERROR){
                        return ERROR_(ERROR_FAILED_TO_ADD_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME);
                    }
                }
            }

            if(propertyFile_setBuffer(remotePort, (int8_t*) argumentSetRemoteHostPort.value) != ERROR_NO_ERROR){
                return ERROR_(ERROR_FAILED_TO_UPDATE_PROPERTY, "'%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME);
            }
        }
        
        goto label_freeProperties;
    }

    // --showSettings.
    if(argumentParser_contains(&parser, &argumentShowSettings)){ 
        if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentShowSettings)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_SETTINGS);
        }else{
            UTIL_LOG_CONSOLE_(LOG_INFO, "ImportDirectory: %s.", PROPERTY_IS_SET(importDirectory) ? (char*) importDirectory->buffer : "NULL");
            UTIL_LOG_CONSOLE_(LOG_INFO, "RemoteHost: %s.", PROPERTY_IS_SET(remoteHost) ? (char*) remoteHost->buffer : "NULL");
            UTIL_LOG_CONSOLE_(LOG_INFO, "RemotePorrt: %" PRIuFAST64 ".", PROPERTY_IS_SET(remotePort) ? util_byteArrayTo_uint64(remotePort->buffer) : 0);
            UTIL_LOG_CONSOLE_(LOG_INFO, "LibraryDirectory: %s.", PROPERTY_IS_SET(libraryDirectory) ? (char*) libraryDirectory->buffer : "NULL");
        }
        
        goto label_freeProperties;
    }

label_printHelp:
    consoleClient_printHelp();

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

inline void consoleClient_printHelp(void){
    UTIL_LOG_CONSOLE(LOG_INFO, "Usage: herder --[command]/-[alias] <arguments>.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_IMPORT_DIRECTORY, "Sets the 'import directory' to the given path.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_LIBRARY_DIRECTORY, "Sets the 'library directory' to the given path.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST, "Sets the 'remote host' address to the given URL.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_PORT, "Sets the 'remote host' port to the given port.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_EPISODE, "Adds the given file to the library.");
}

inline ERROR_CODE consoleClient_listShows(Property* remoteHostProperty, Property* remoteHostPortProperty){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHostProperty->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remoteHostPortProperty->buffer);

    LinkedList shows;
    if((error = herder_pullShowList(&shows, host, port)) != ERROR_NO_ERROR){
        if(error != ERROR_FAILED_TO_CONNECT){
            goto label_freeShowList;
        }else{
            goto label_return;
        }
    }

    if(shows.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO, "Library is empty.");

        goto label_freeShowList;
    }

    LinkedListIterator it;
    linkedList_initIterator(&it, &shows);

    uint_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);
        
        UTIL_LOG_CONSOLE_(LOG_INFO, "%02" PRIuFAST64 ":'%s'.", i, show->name);
        i++;

        mediaLibrary_freeShow(show);

        free(show);
    }

label_freeShowList:
    linkedList_free(&shows);

label_return:
    return ERROR(error);
}

ERROR_CODE consoleClient_import(Property* remoteHost, Property* remotePort, Property* libraryDirectory, const char* directory){
    ERROR_CODE error;

     if(!CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET() || PROPERTY_IS_NOT_SET(libraryDirectory)){
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

inline ERROR_CODE consoleClient_listAllShows(Property* remoteHostProperty, Property* remoteHostPortProperty){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* host = (char*) remoteHostProperty->buffer;
    uint_fast16_t port = util_byteArrayTo_uint64(remoteHostPortProperty->buffer);

    LinkedList shows = {0};
    if((error = herder_pullShowList(&shows, host, port)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    if(shows.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO,"Library is empty.");

        goto label_freeShowList;
    }

    LinkedListIterator it;
    linkedList_initIterator(&it, &shows);

    uint_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);
        
        if((error = consoleClient_printShowInfo(remoteHostProperty, remoteHostPortProperty, show->name, show->nameLength)) != ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
        }

        i++;

        mediaLibrary_freeShow(show);
        free(show);
    }

label_freeShowList:
    linkedList_free(&shows);

    return ERROR(error);
}

inline ERROR_CODE consoleClient_printShowInfo(Property* remoteHost, Property* remotePort, const char* showName, const uint_fast64_t showNameLength){
    ERROR_CODE error;

    Show show;
    if((error = medialibrary_initShow(&show, showName, showNameLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = herder_pullShowInfo(remoteHost, remotePort, &show))  != ERROR_NO_ERROR){
        UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to print show info. '%s'", util_toErrorString(error));

        goto label_freeShow;
    }

    UTIL_LOG_CONSOLE_(LOG_INFO, "%s:", show.name);

    LinkedListIterator seasonIterator;
    linkedList_initIterator(&seasonIterator, &show.seasons);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&seasonIterator)){
        Season* season = LINKED_LIST_ITERATOR_NEXT(&seasonIterator);

        UTIL_LOG_CONSOLE_(LOG_INFO, "\tSeason: %02" PRIuFAST16 ".", season->number);

        LinkedListIterator episodeIterator;
        linkedList_initIterator(&episodeIterator, &season->episodes);

        while(LINKED_LIST_ITERATOR_HAS_NEXT(&episodeIterator)){
            Episode* episode = LINKED_LIST_ITERATOR_NEXT(&episodeIterator);

            UTIL_LOG_CONSOLE_(LOG_INFO, "\t\t  -> %02" PRIuFAST16 ": '%s'.", episode->number, episode->name);
        }
    }

label_freeShow:
    mediaLibrary_freeShow(&show);

label_return:
    return ERROR(error);
}