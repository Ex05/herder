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

#define REMOTE_HOST_PROPERTIES_SET() ((propertyFile_propertySet(remoteHost, CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME) == ERROR_NO_ERROR) && (propertyFile_propertySet(remotePort, CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME) == ERROR_NO_ERROR))

local void consoleClient_printHelp(void);

// TODO:(jan) Make custom entry point for the client build, so we won't have to use 'main'.
#ifndef TEST_BUILD
	int main(const int argc, const char** argv){
#else
    int consoleClient_totalyNotMain(const int argc, const char** argv){
#endif
    openlog(CONSOLE_CLIENT_PROGRAM_NAME, LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_USER);

    ArgumentParser parser;
    argumentParser_init(&parser);
    
    ARGUMENT_PARSER_ADD_ARGUMENT(Help, 3, "-?", "-h", "--help"); //
    ARGUMENT_PARSER_ADD_ARGUMENT(AddShow, 1, "--addShow"); //
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveShow, 1, "--removeShow"); //
    ARGUMENT_PARSER_ADD_ARGUMENT(Add, 4, "-a", "-add", "--addFile", "--addEpisode"); //
    ARGUMENT_PARSER_ADD_ARGUMENT(Import, 2, "-i", "--import"); //
    ARGUMENT_PARSER_ADD_ARGUMENT(RenameEpisode, 1, "--renameEpisode"); //
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveEpisode, 1, "--removeEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListShows, 3, "-l", "--list", "--listShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListAll, 2, "--listAll", "--listAllShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowInfo, 2, "--showInfo", "--info"); //
    ARGUMENT_PARSER_ADD_ARGUMENT(SetImportDirectory, 1, "--setImportDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetLibraryDirectory, 1, "--setLibraryDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHost, 1, "--setRemoteHost");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHostPort, 1, "--setRemotePort");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowSettings, 1, "--showSettings"); //

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
    propertyFile_getProperty(&properties, &libraryDirectory, PROPERTY_LIBRARY_DIRECTORY_NAME);

    // --help.
    if(argumentParser_contains(&parser, &argumentHelp)){ 
        goto label_printHelp;
    }

    // --addShow.
    if(argumentParser_contains(&parser, &argumentAddShow)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentAddShow)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_SHOW);
        }
        else{
            if(REMOTE_HOST_PROPERTIES_SET()){
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
        }
        else{
             if(REMOTE_HOST_PROPERTIES_SET()){
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
        }
        else{
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
        }
        else{
            ERROR_CODE error;
            if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE((argumentImport))){
                // Import from path.
                if((error = herder_import(remoteHost, remotePort, libraryDirectory, argumentImport.value)) != ERROR_NO_ERROR){
                    // 
                }
            }else{
                if(PROPERTY_IS_SET(importDirectory)){
                    // Import from importDirectory_setting.
                    if((error = herder_import(remoteHost, remotePort, libraryDirectory, (char*) importDirectory->buffer)) != ERROR_NO_ERROR){
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
        }
        else{
            if(REMOTE_HOST_PROPERTIES_SET()){
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
        }
        else{
            UTIL_LOG_CONSOLE(LOG_INFO, "--removeEpisode");
        }
        
        goto label_freeProperties;
    }

    // --listShows.
    if(argumentParser_contains(&parser, &argumentListShows)){ 
        if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentListShows)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_SHOWS);
        }
        else{
            if(REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = herder_listShows(remoteHost, remotePort)) != ERROR_NO_ERROR){
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
        }
        else{
            UTIL_LOG_CONSOLE(LOG_INFO, "--listAll");
        }
        
        goto label_freeProperties;
    }

    // --showInfo.
    if(argumentParser_contains(&parser, &argumentShowInfo)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentShowInfo)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_INFO);
        }
        else{
             if(REMOTE_HOST_PROPERTIES_SET()){
                 ERROR_CODE error;
                if((error = herder_printShowInfo(remoteHost, remotePort, argumentShowInfo.value, argumentShowInfo.valueLength))  != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to print show info. '%s'", util_toErrorString(error));
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
        }
        else{
            UTIL_LOG_CONSOLE(LOG_INFO, "--setImportDirectory");
        }
        
        goto label_freeProperties;
    }

    // --setLibraryDirectory.
    if(argumentParser_contains(&parser, &argumentSetLibraryDirectory)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetLibraryDirectory)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_LIBRARY_DIRECTORY);
        }
        else{
            UTIL_LOG_CONSOLE(LOG_INFO, "--setLibraryDirectory");
        }
        
        goto label_freeProperties;
    }

    // --setRemoteHost.
    if(argumentParser_contains(&parser, &argumentSetRemoteHost)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetRemoteHost)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST);
        }
        else{
            UTIL_LOG_CONSOLE(LOG_INFO, "--setRemoteHost");
        }
        
        goto label_freeProperties;
    }

    // --setRemotePort.
    if(argumentParser_contains(&parser, &argumentSetRemoteHostPort)){ 
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentSetRemoteHostPort)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_PORT);
        }
        else{
            UTIL_LOG_CONSOLE(LOG_INFO, "--setRemotePort");
        }
        
        goto label_freeProperties;
    }

    // --showSettings.
    if(argumentParser_contains(&parser, &argumentShowSettings)){ 
        if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentShowSettings)){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_SETTINGS);
        }
        else{
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
