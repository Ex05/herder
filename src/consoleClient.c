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
#include "pls.c"

#define CONSOLE_CLIENT_PROGRAM_NAME "herder"

#define CONSOLE_CLIENT_USAGE_ARGUMENT_HELP "-?, -h, --help"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_SHOW "--addShow <name>."
#define CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_SHOW "--removeShow <name>."
#define CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_EPISODE "-a, --add, --addFile, --addEpisode <file>."
#define CONSOLE_CLIENT_USAGE_ARGUMENT_IMPORT "-i, --import optional:<path>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_BATCH_IMPORT "-b, --batch, --batchImport"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME "--rename"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME_EPISODE "--renameEpisode <old_name> <new_name>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE "--remove"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_EPISODE "--removeEpisode <name>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_SHOWS "-l, --list --listShows"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_ALL_SHOWS "--listAll, --listAllShows"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_INFO "--showInfo <name>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_IMPORT_DIRECTORY "--setImportDirectory <path>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_LIBRARY_DIRECTORY "--setLibraryDirectory <path>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST "--setRemoteHost <port>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_PORT "--setRemotePort <port>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_SETTINGS "--showSettings"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_PLAY "-p, --play <name>"
#define CONSOLE_CLIENT_USAGE_ARGUMENT_CONVERT "--mp3/convert :optional<all/showName>"

#define CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME "importDirectory"
#define CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME "remoteHost"
#define CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME "remotePort"
#define CONSOLE_CLIENT_PROPERTY_LIBRARY_DIRECTORY_NAME "libraryDirectory"

#define CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET() ((propertyFile_propertySet(remoteHost, CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME) == ERROR_NO_ERROR) && (propertyFile_propertySet(remotePort, CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME) == ERROR_NO_ERROR))

local void consoleClient_printHelp(void);

local ERROR_CODE consoleClient_listShows(Property*, Property*);

local ERROR_CODE consoleClient_import(Property*, Property*, Property*, const char*, const bool);

local ERROR_CODE consoleClient_listAllShows(Property*, Property*);

local ERROR_CODE consoleClient_printShowInfo(Property*, Property*, const char*, const uint_fast64_t);

local ERROR_CODE consoleClient_extractShowInfo(Property*, Property*, EpisodeInfo*, const bool);

local ERROR_CODE consoleClient_rename(Property*, Property*, Property*);

local ERROR_CODE consoleClient_renameEpisode(Property*, Property*, Property*, const char*, uint_fast64_t, const char*, const uint_fast64_t);

local ERROR_CODE consoleClient_remove(Property*, Property*, Property*);

local ERROR_CODE consoleClient_removeEpisode(Property*, Property*, Property*, const char*, uint_fast64_t);

local ERROR_CODE consoleClient_selectShow(Property*, Property*, Show**);

local ERROR_CODE consoleClient_selectSeason(Property*, Property*, Season**, Show*);

local ERROR_CODE consoleClient_selectEpisode(Episode**, Season*);

local ERROR_CODE consoleClient_selectYesNo(bool*);

local ERROR_CODE consoleClient_addEpisode(Property*, Property*, Property*, const char*, const uint_fast64_t);

local ERROR_CODE consoleClient_convert(Property*, Property*, Property*);

// TODO:(jan) Make custom entry point for the client build, so we won't have to use 'main'.
#ifndef TEST_BUILD
	int main(const int argc, const char** argv){
#else
    int consoleClient_totalyNotMain(const int argc, const char** argv){
#endif
    ERROR_CODE error;
    openlog(CONSOLE_CLIENT_PROGRAM_NAME, LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_USER);

    ArgumentParser parser;
    if((error = argumentParser_init(&parser)) != ERROR_NO_ERROR){
        goto label_free;
    }

    ARGUMENT_PARSER_ADD_ARGUMENT(Help, 3, "-?", "-h", "--help");
    ARGUMENT_PARSER_ADD_ARGUMENT(AddShow, 1, "--addShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveShow, 1, "--removeShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(Add, 4, "-a", "--add", "--addFile", "--addEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import, 2, "-i", "--import");
    ARGUMENT_PARSER_ADD_ARGUMENT(BatchImport, 3, "-b", "--batch", "--batchImport");
    ARGUMENT_PARSER_ADD_ARGUMENT(Rename, 1, "--rename");
    ARGUMENT_PARSER_ADD_ARGUMENT(RenameEpisode, 1, "--renameEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(Remove, 1, "--remove");
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveEpisode, 1, "--removeEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListShows, 3, "-l", "--list", "--listShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListAll, 2, "--listAll", "--listAllShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowInfo, 2, "--showInfo", "--info");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetImportDirectory, 1, "--setImportDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetLibraryDirectory, 1, "--setLibraryDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHost, 1, "--setRemoteHost");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHostPort, 1, "--setRemotePort");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowSettings, 1, "--showSettings");
    ARGUMENT_PARSER_ADD_ARGUMENT(Play, 1, "--play");
    ARGUMENT_PARSER_ADD_ARGUMENT(Convert, 2, "--mp3", "--convert");

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
        if(argumentAddShow.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_SHOW);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = herder_addShow(remoteHost, remotePort, argumentAddShow.values[0], argumentAddShow.valueLengths[0])) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add show '%s' to the library. '%s'", argumentAddShow.values[0],  util_toErrorString(error));
                }else{
                    UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully added show '%s' to the library.", argumentAddShow.values[0]);
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    }

    // --removeShow.
    if(argumentParser_contains(&parser, &argumentRemoveShow)){ 
        if(argumentRemoveShow.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_SHOW);
        }else{
             if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                 ERROR_CODE error;
                if((error = herder_removeShow(remoteHost, remotePort, argumentRemoveShow.values[0], argumentRemoveShow.valueLengths[0])) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to remove show. '%s'", util_toErrorString(error));
                }else{
                    UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully removed show '%s' from the library.", argumentRemoveShow.values[0]);
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    } 

    // --addEpisode.
    if(argumentParser_contains(&parser, &argumentAdd)){ 
        if(argumentAdd.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_EPISODE);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET() && (propertyFile_propertySet(libraryDirectory, CONSOLE_CLIENT_PROPERTY_LIBRARY_DIRECTORY_NAME) == ERROR_NO_ERROR)){
                if(!util_fileExists(argumentAdd.values[0]) || util_isDirectory(argumentAdd.values[0])){
                    UTIL_LOG_CONSOLE_(LOG_INFO, "ERROR: '%s' is not a falid file.", argumentAdd.values[0]);
                }else{
                    if((error = consoleClient_addEpisode(remoteHost, remotePort, libraryDirectory, argumentAdd.values[0], argumentAdd.valueLengths[0])) != ERROR_NO_ERROR){
                        UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add file'%s' to library. '%s'", argumentAdd.values[0], util_toErrorString(error));
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
        ERROR_CODE error;
        if(argumentImport.numValues == 1){
            // Import from path.
            if((error = consoleClient_import(remoteHost, remotePort, libraryDirectory, argumentImport.values[0], false)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE_(LOG_ERR, "ERROR: Failed to import from directory: '%s'. [%s]", argumentImport.values[0], util_toErrorString(error));
            }
        }else{
            if(argumentImport.numValues == 0){
                if(PROPERTY_IS_SET(importDirectory)){
                    // Import from importDirectory_setting.
                    if((error = consoleClient_import(remoteHost, remotePort, libraryDirectory, (char*) importDirectory->buffer, false)) != ERROR_NO_ERROR){
                        if(error != ERROR_DUPLICATE_ENTRY){
                            UTIL_LOG_CONSOLE_(LOG_ERR, "ERROR: Failed to import from directory: '%s'. [%s]", (char*) importDirectory->buffer, util_toErrorString(error));
                        }
                    }
                }else{
                    UTIL_LOG_CONSOLE(LOG_ERR, "Import directory not set, Use '--setImportDirectory <path>' to set an import directory, or specify the directory to import from when running '-i, --import optional:<path>'.");
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_IMPORT);
            }
        }

        goto label_freeProperties;
    }

    // --batch.
    if(argumentParser_contains(&parser, &argumentBatchImport)){ 
        ERROR_CODE error;
        if(argumentBatchImport.numValues == 1){
            // Import from path.
            if((error = consoleClient_import(remoteHost, remotePort, libraryDirectory, argumentBatchImport.values[0], true)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE_(LOG_ERR, "ERROR: Failed to import from directory: '%s'. [%s]", argumentBatchImport.values[0], util_toErrorString(error));
            }
        }else{
            if(argumentBatchImport.numValues == 0){
                if(PROPERTY_IS_SET(importDirectory)){
                    // Import from importDirectory_setting.
                    if((error = consoleClient_import(remoteHost, remotePort, libraryDirectory, (char*) importDirectory->buffer, true)) != ERROR_NO_ERROR){
                        if(error != ERROR_DUPLICATE_ENTRY){
                            UTIL_LOG_CONSOLE_(LOG_ERR, "ERROR: Failed to import from directory: '%s'. [%s]", (char*) importDirectory->buffer, util_toErrorString(error));
                        }
                    }
                }else{
                    UTIL_LOG_CONSOLE(LOG_ERR, "Import directory not set, Use '--setImportDirectory <path>' to set an import directory, or specify the directory to import from when running '-i, --import optional:<path>'.");
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_BATCH_IMPORT);
            }
        }

        goto label_freeProperties;
    }

    // --rename.
    if(argumentParser_contains(&parser, &argumentRename)){ 
        if(argumentRename.numValues != 0){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = consoleClient_rename(remoteHost, remotePort, libraryDirectory)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to rename Episode. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    }

    // --renameEpisode.
    if(argumentParser_contains(&parser, &argumentRenameEpisode)){
        if(argumentRenameEpisode.numValues != 2){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME_EPISODE);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = consoleClient_renameEpisode(remoteHost, remotePort, libraryDirectory, argumentRenameEpisode.values[0], argumentRenameEpisode.valueLengths[0], argumentRenameEpisode.values[1], argumentRenameEpisode.valueLengths[1])) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to rename Episode. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    }

    // --remove.
    if(argumentParser_contains(&parser, &argumentRemove)){ 
        if(argumentRename.numValues != 0){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = consoleClient_remove(remoteHost, remotePort, libraryDirectory)) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to remove Episode. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    }

    // --removeEpisode.
    if(argumentParser_contains(&parser, &argumentRemoveEpisode)){ 
        if(argumentRenameEpisode.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_EPISODE);
        }else{
            if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                ERROR_CODE error;
                if((error = consoleClient_removeEpisode(remoteHost, remotePort, libraryDirectory, argumentRemoveEpisode.values[0], argumentRemoveEpisode.valueLengths[0])) != ERROR_NO_ERROR){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to remove Episode. '%s'", util_toErrorString(error));
                }
            }else{
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_PROPERTY_NOT_SET));
            }
        }

        goto label_freeProperties;
    }

    // --listShows.
    if(argumentParser_contains(&parser, &argumentListShows)){
        if(argumentListShows.numValues != 0){
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
        if(argumentListAll.numValues != 0){
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
        if(argumentShowInfo.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_INFO);
        }else{
             if(CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET()){
                 if((error = consoleClient_printShowInfo(remoteHost, remotePort, argumentShowInfo.values[0], argumentShowInfo.valueLengths[0]))){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to print show info for '%s': '%s'.", argumentShowInfo.values[0], util_toErrorString(error));

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
        if(argumentSetImportDirectory.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_IMPORT_DIRECTORY);
        }else{
            if((error = propertyFile_createAndSetDirectoryProperty(&properties, &importDirectory, CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME, argumentSetImportDirectory.values[0], argumentSetImportDirectory.valueLengths[0])) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
            }else{
                UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'", CONSOLE_CLIENT_PROPERTY_IMPORT_DIRECTORY_NAME, (char*) importDirectory->buffer);
            }
        }

        goto label_freeProperties;
    }

    // --setLibraryDirectory.
    if(argumentParser_contains(&parser, &argumentSetLibraryDirectory)){
        if(argumentSetLibraryDirectory.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_LIBRARY_DIRECTORY);
        }else{
            if((error = propertyFile_createAndSetDirectoryProperty(&properties, &libraryDirectory, CONSOLE_CLIENT_PROPERTY_LIBRARY_DIRECTORY_NAME, argumentSetLibraryDirectory.values[0], argumentSetLibraryDirectory.valueLengths[0])) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
            }else{
                UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'", CONSOLE_CLIENT_PROPERTY_LIBRARY_DIRECTORY_NAME, argumentSetLibraryDirectory.values[0]);
            }
        }

        goto label_freeProperties;
    }

    // --setRemoteHost.
    if(argumentParser_contains(&parser, &argumentSetRemoteHost)){ 
        if(argumentSetRemoteHost.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST);
        }else{
            if((error = propertyFile_createAndSetStringProperty(&properties, &libraryDirectory, CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost.values[0], argumentSetRemoteHost.valueLengths[0])) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
            }else{
                UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_HOST_NAME, argumentSetRemoteHost.values[0]);
            }
        }

        goto label_freeProperties;
    }

    // --setRemotePort.
    if(argumentParser_contains(&parser, &argumentSetRemoteHostPort)){ 
       if(argumentSetRemoteHostPort.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST);
        }else{
            int64_t port;
            ERROR_CODE error;
            if((error = util_stringToInt(argumentSetRemoteHostPort.values[0], &port)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE_(LOG_ERR, "Port value must be in range of 0-%" PRIu16 ". '%s'.", UINT16_MAX, util_toErrorString(ERROR_INVALID_VALUE));

                goto label_freeProperties;
            }else{
                if(port <= 0 || port > UINT16_MAX){
                    UTIL_LOG_CONSOLE_(LOG_ERR, "Port value must be in range of 0-%" PRIu16 ". '%s'.", UINT16_MAX, util_toErrorString(ERROR_INVALID_VALUE));

                    goto label_freeProperties;
                }
            }

            if((error = propertyFile_createAndSetUINT16Property(&properties, &libraryDirectory, CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME, port)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error));
            }else{
                UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully set '%s' to '%s'", CONSOLE_CLIENT_PROPERTY_REMOTE_PORT_NAME, argumentSetRemoteHostPort.values[0]);
            }
        }

        goto label_freeProperties;
    }

    // --play.
    if(argumentParser_contains(&parser, &argumentPlay)){ 
        if(argumentPlay.numValues != 1){
            UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_PLAY);
        }else{
            Show show;
            if((error = medialibrary_initShow(&show, argumentPlay.values[0], argumentPlay.valueLengths[0])) != ERROR_NO_ERROR){
                goto label_freeProperties;
            }

            if((error = herder_pullShowInfo(remoteHost, remotePort, &show)) != ERROR_NO_ERROR){
                goto label_freeProperties;
            }

            #define PLAYLIST_FILE_NAME "/tmp/herder_playList.pls"

            if((error = pls_savePlayList(libraryDirectory, PLAYLIST_FILE_NAME, &show)) != ERROR_NO_ERROR){
                mediaLibrary_freeShow(&show);

                goto label_freeProperties;
            }

            mediaLibrary_freeShow(&show);

            system("vlc \"" PLAYLIST_FILE_NAME "\" --play-and-exit");

            #undef PLAYLIST_FILE_NAME
        }

        goto label_freeProperties;
    }

    // --convert.
    if(argumentParser_contains(&parser, &argumentConvert)){ 
        if(argumentConvert.numValues == 0){
            consoleClient_convert(remoteHost, remotePort, libraryDirectory);
        }else{
            if(argumentConvert.numValues == 1){
                LinkedList shows;
                if((error = herder_pullShowList(&shows, remoteHost, remotePort)) != ERROR_NO_ERROR){
                    goto label_freeProperties;
                }

                #define CONSOLE_CLIENT_STRING_CONSTANT_ALL "all"
                
                if(strncmp(CONSOLE_CLIENT_STRING_CONSTANT_ALL, argumentConvert.values[0], strlen(CONSOLE_CLIENT_STRING_CONSTANT_ALL) + 1) == 0){
                    LinkedListIterator it;
                    linkedList_initIterator(&it, &shows);

                    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
                        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

                        if(strncmp(argumentConvert.values[0], show->name, argumentConvert.valueLengths[0] + 1) == 0){
                            if((error = herder_convertToMP3(remoteHost, remotePort, libraryDirectory, show->name, show->nameLength)) != ERROR_NO_ERROR){
                                goto label_freeShowList;
                            }
                        }
                    }
                }else{
                    LinkedListIterator it;
                    linkedList_initIterator(&it, &shows);

                    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
                        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

                        if(strncmp(argumentConvert.values[0], show->name, argumentConvert.valueLengths[0] + 1) == 0){
                            if((error = herder_convertToMP3(remoteHost, remotePort, libraryDirectory, show->name, show->nameLength)) != ERROR_NO_ERROR){
                                goto label_freeShowList;
                            }
                        }
                    }
                }
                #undef CONSOLE_CLIENT_STRING_CONSTANT_ALL

                LinkedListIterator it;

            label_freeShowList:
                linkedList_initIterator(&it, &shows);

                while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
                    Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

                    mediaLibrary_freeShow(show);

                    free(show);
                }

                linkedList_free(&shows);
            }else{
                UTIL_LOG_CONSOLE(LOG_INFO, "Invalid command. Usage: " CONSOLE_CLIENT_USAGE_ARGUMENT_CONVERT);
            }
        }
        goto label_freeProperties;
    }

    // --showSettings.
    if(argumentParser_contains(&parser, &argumentShowSettings)){ 
        if(argumentShowSettings.numValues != 0){
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

    propertyFile_free(&properties);

label_free:
	argumentParser_free(&parser);

	closelog();

    return EXIT_SUCCESS;
}

inline void consoleClient_printHelp(void){
    UTIL_LOG_CONSOLE(LOG_INFO, "Usage: herder --[command]/-[alias] <arguments>.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_SHOW, "Adds a new empty show with the given name to the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_SHOW, "Removes the show with the given name from the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_ADD_EPISODE, "Adds the given file to the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_IMPORT, "Imports all files from the given directory, if no directory is specified the import directory set via '--setImportDirectory' will be used.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_BATCH_IMPORT, "Like '--import' but only imports episodes which don't require any user input to be add to the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME, "Interactivly renames an episode.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_RENAME_EPISODE, "Renames episode <old_name> to <new_name>.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE, "Interactivly remove an episode from the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_REMOVE_EPISODE, "Remove episode <name> from the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_SHOWS, "Lists all shows currently in the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_LIST_ALL_SHOWS, "Lists all episodes of all shows currently in the library.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_INFO, "Lists all episodes of the show <name>.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_IMPORT_DIRECTORY, "Sets the 'import directory' to the given path.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_LIBRARY_DIRECTORY, "Sets the 'library directory' to the given path.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_HOST, "Sets the 'remote host' address to the given URL.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SET_REMOTE_PORT, "Sets the 'remote host' port to the given port.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_SHOW_SETTINGS, "Shows a quick overview of all the user settings.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_PLAY, "Shows a quick overview of all the user settings.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t%s\t\t\t%s", CONSOLE_CLIENT_USAGE_ARGUMENT_CONVERT, "Convert episodes to audio only versions. No Arguments opens an interactive episode selection, 'all' to convert all episodes in the library which have not been converted and 'showName' to convert all episodes of a show.");
}

inline ERROR_CODE consoleClient_listShows(Property* remoteHost, Property* remotePort){
    ERROR_CODE error = ERROR_NO_ERROR;

    LinkedList shows;
    if((error = herder_pullShowList(&shows, remoteHost, remotePort)) != ERROR_NO_ERROR){
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

ERROR_CODE consoleClient_import(Property* remoteHost, Property* remotePort, Property* libraryDirectory, const char* directory, const bool batchImport){
    ERROR_CODE error;

     if(!CONSOLE_CLIENT_REMOTE_HOST_PROPERTIES_SET() || PROPERTY_IS_NOT_SET(libraryDirectory)){
        return ERROR(ERROR_PROPERTY_NOT_SET);
     }

    LinkedList infos;
    if((error = linkedList_init(&infos)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = util_walkDirectory(&infos, directory, UTIL_FILES_ONLY)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if(LINKED_LIST_IS_EMPTY(&infos)){
        UTIL_LOG_CONSOLE_(LOG_INFO, "No files recorgnised for import in '%s'.", directory);

        goto label_freeFiles;
    }

    LinkedList fileInfos;
    if((error = linkedList_init(&fileInfos)) != ERROR_NO_ERROR){
        goto label_freeFiles;
    }

    LinkedListIterator fileIterator;
    linkedList_initIterator(&fileIterator, &infos);
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&fileIterator)){
        char* path = LINKED_LIST_ITERATOR_NEXT(&fileIterator);

        const uint_fast64_t pathLength = strlen(path);

        EpisodeInfo* info = alloca(sizeof(*info));
        mediaLibrary_initEpisodeInfo(info);
        info->path = path;
        info->pathLength = pathLength;

        info->fileName = util_getFileName(info->path, info->pathLength);
        info->fileNameLength = strlen(info->fileName);

        UTIL_LOG_CONSOLE_(LOG_INFO, "\"%s\"", info->fileName);
          
        if((error = consoleClient_extractShowInfo(remoteHost, remotePort, info, batchImport)) != ERROR_NO_ERROR){
            UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to extract show info from file: '%s'. [%s]", info->fileName, util_toErrorString(error));

            goto label_freeFiles;
        }

        linkedList_add(&fileInfos, info);
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Importing:...");

    const uint_fast64_t entries = infos.length;
    uint_fast64_t i = 0;

    LinkedListIterator episodeInfoIterator;
    linkedList_initIterator(&episodeInfoIterator, &fileInfos);
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&episodeInfoIterator)){
        EpisodeInfo* info = LINKED_LIST_ITERATOR_NEXT(&episodeInfoIterator);

        if((error = herder_addEpisode(remoteHost, remotePort, libraryDirectory, info)) != ERROR_NO_ERROR){
            if(error != ERROR_DUPLICATE_ENTRY){
                UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add Episode: '%s'. [%s]", info->fileName, util_toErrorString(error));
            }

            goto label_freeFiles;
        }else{
            i++;

            UTIL_LOG_CONSOLE_(LOG_INFO, " %" PRIdFAST64 "/%" PRIdFAST64 " Successfully added '%s'. to library.", i, entries, info->fileName);
        }

        mediaLibrary_freeEpisodeInfo(info);
    }

    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_FAILED_TO_DELETE_DIRECTORY);
    if((error = util_deleteDirectory(directory, true, true)) != ERROR_NO_ERROR){
        goto label_freeFiles;
    }

    linkedList_free(&fileInfos);

label_freeFiles:
    linkedList_free(&infos);

label_return:
    return ERROR(error);
}

inline ERROR_CODE consoleClient_listAllShows(Property* remoteHost, Property* remotePort){
    ERROR_CODE error = ERROR_NO_ERROR;

    LinkedList shows = {0};
    if((error = herder_pullShowList(&shows, remoteHost, remotePort)) != ERROR_NO_ERROR){
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

        if((error = consoleClient_printShowInfo(remoteHost, remotePort, show->name, show->nameLength)) != ERROR_NO_ERROR){
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

    if((error = herder_pullShowInfo(remoteHost, remotePort, &show)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    UTIL_LOG_CONSOLE_(LOG_INFO, "%s:", show.name);

    Season** seasons = alloca(sizeof(*seasons) * show.seasons.length);
    if((error = mediaLibrary_sortSeasons(&seasons, &show.seasons)) != ERROR_NO_ERROR){
        goto label_freeShow;
    }

    if(show.seasons.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO, "\tEmpty.");
    }

    register uint_fast64_t i;
    for(i = 0; i < show.seasons.length; i++){
        Season* season = seasons[i];

        Episode** episodes = alloca(sizeof(*episodes) * season->episodes.length);
        if((error = mediaLibrary_sortEpisodes(&episodes, &season->episodes)) != ERROR_NO_ERROR){
            goto label_freeShow;
        }

        UTIL_LOG_CONSOLE_(LOG_INFO, "\tSeason: %02" PRIuFAST16 ".", season->number);

        register uint_fast64_t j;
        for(j = 0; j < season->episodes.length; j++){
            Episode* episode = episodes[j];

            UTIL_LOG_CONSOLE_(LOG_INFO, "\t\t  -> %02" PRIuFAST16 ": '%s'.", episode->number, episode->name);
        }
    }

label_freeShow:
    mediaLibrary_freeShow(&show);

label_return:
    return ERROR(error);
}

ERROR_CODE consoleClient_extractShowInfo(Property* remoteHost, Property* remotePort, EpisodeInfo* episodeInfo, const bool batchImport){
    ERROR_CODE error;

label_extractShowInfo:
    if((error = herder_extractShowInfo(remoteHost, remotePort, episodeInfo)) != ERROR_NO_ERROR){
        if(error != ERROR_INCOMPLETE){
            goto label_return;
        }
    }

    int_fast64_t userInputLength;
    char* userInput;
    if(episodeInfo->showName == NULL){
        UTIL_LOG_CONSOLE(LOG_INFO, "Failed to extract the show name based on library entries.\nPlease enter the show name.");

        if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeUserInput;
        }

        if((error = herder_addShow(remoteHost, remotePort, userInput, userInputLength)) != ERROR_NO_ERROR){
            goto label_freeUserInput;
        }

        free(episodeInfo->name);
        free(userInput);

        goto label_extractShowInfo;
    }

    if(batchImport){
        goto label_return;
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Are these values correct? Yes/No.");
    UTIL_LOG_CONSOLE_(LOG_INFO, "\tShow:'%s'.", episodeInfo->showName);
    UTIL_LOG_CONSOLE_(LOG_INFO, "\tSeason:'%" PRIdFAST16 "'.", episodeInfo->season);
    UTIL_LOG_CONSOLE_(LOG_INFO, "\tEpisode:'%" PRIdFAST16 "'.", episodeInfo->episode);
    UTIL_LOG_CONSOLE_(LOG_INFO, "\t\t'%s'.", episodeInfo->name);

label_readUserInput:
    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_freeUserInput;
    }

    util_toLowerChase(userInput);

    if(userInputLength != 0 && strncmp("no", userInput, userInputLength) == 0){
        // Show name.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Show name:'%s'. Press <Enter> to accept.", episodeInfo->showName);

        char* showName;
        if((error = util_readUserInput(&showName, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeUserInput;
        }

        if(userInputLength != 0){
            free(episodeInfo->showName);

            episodeInfo->showName = showName;
            episodeInfo->showNameLength = userInputLength;
        }else{
            free(showName);
        }

    label_seasonNumber:
        // Season number.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Season:'%" PRIiFAST16 "'. Press <Enter> to accept.", episodeInfo->season);

        char* season;
        if((error = util_readUserInput(&season, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeUserInput;
        }

        if(userInputLength != 0){
            if((error = util_stringToInt(season, (int64_t*) &episodeInfo->season)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE_(LOG_ERR, "%s.", util_toErrorString(error));

                free(season);

                goto label_seasonNumber;
            }
        }

        free(season);

    label_episodeNumber:
        // Episode number.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Episode:'%" PRIiFAST16 "'. Press <Enter> to accept.", episodeInfo->episode);

        char* episode;
        if((error = util_readUserInput(&episode, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeUserInput;
        }

        if(userInputLength != 0){
            if((error = util_stringToInt(episode, (int64_t*) &episodeInfo->episode)) != ERROR_NO_ERROR){
                UTIL_LOG_CONSOLE_(LOG_ERR, "%s.", util_toErrorString(error));

                free(episode);

                goto label_episodeNumber;
            }
        }

        free(episode);

        // Episode name.
        UTIL_LOG_CONSOLE_(LOG_INFO, "Episode name:'%s'. Press <Enter> to accept.", episodeInfo->name);

        char* episodeName;
        if((error = util_readUserInput(&episodeName, &userInputLength)) != ERROR_NO_ERROR){
            goto label_freeUserInput;
        }

        if(userInputLength != 0){
            free(episodeInfo->name);

            episodeInfo->name = episodeName;
            episodeInfo->nameLength = userInputLength;
        }
    }else{
        if(userInputLength != 0 || strncmp("yes", userInput, userInputLength) != 0){
            UTIL_LOG_CONSOLE_(LOG_INFO, "'%s' is not a valid answer, please type Yes/No.", userInput);

            free(userInput);

            goto label_readUserInput;
        }
    }

label_freeUserInput:
    free(userInput);

label_return:
    return ERROR(error);
}

ERROR_CODE consoleClient_rename(Property* remoteHost, Property* remotePort, Property* libraryDirectory){
    ERROR_CODE error = ERROR_NO_ERROR;

    Show* selectedShow = NULL;
    if((error = consoleClient_selectShow(remoteHost, remotePort, &selectedShow))){
        goto label_return;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Selected show:'%s'.", selectedShow->name);

    Season* selectedSeason = NULL;
    if((error = consoleClient_selectSeason(remoteHost, remotePort, &selectedSeason, selectedShow))){
        goto label_return;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Selected season:'%" PRIdFAST16 "'.", selectedSeason->number);

    Episode* selectedEpisode = NULL;
    if((error = consoleClient_selectEpisode(&selectedEpisode, selectedSeason) != ERROR_NO_ERROR)){
        goto label_return;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Selected:[Episode:%" PRIdFAST16 " '%s'].", selectedEpisode->number, selectedEpisode->name);

    UTIL_LOG_CONSOLE(LOG_INFO, "Please enter a new episode name.");

    char* newEpisodeName;
    int_fast64_t newEpisodeNameLength;

    if((error = util_readUserInput(&newEpisodeName, &newEpisodeNameLength)) != ERROR_NO_ERROR){
        goto label_freeNewName;
    }

label_yesNo:
    UTIL_LOG_CONSOLE_(LOG_INFO, "Rename '%s', s%02" PRIdFAST16 "e%02" PRIdFAST16 " - '%s' to '%s'? Yes/No.", selectedShow->name, selectedSeason->number, selectedEpisode->number, selectedEpisode->name, newEpisodeName);

    bool renameEpisode;
    if((error = consoleClient_selectYesNo(&renameEpisode) != ERROR_NO_ERROR)){
        goto label_yesNo;
    }

    if(!renameEpisode){
        goto label_freeNewName;
    }

    EpisodeInfo info;
    if((error = mediaLibrary_initEpisodeInfo(&info)) != ERROR_NO_ERROR){
        goto label_freeNewName;
    }

    mediaLibrary_fillEpisodeInfo(&info ,selectedShow, selectedSeason, selectedEpisode);

    if((error = herder_renameEpisode(remoteHost, remotePort, libraryDirectory, &info, newEpisodeName, newEpisodeNameLength)) != ERROR_NO_ERROR){
        UTIL_LOG_CONSOLE_(LOG_ERR, "Server side error, failed to rename episode. [%s]", util_toErrorString(error));

        goto label_freeNewName;
    }

label_freeNewName:
    free(newEpisodeName);

    mediaLibrary_freeShow(selectedShow);

    free(selectedShow);

label_return:
    return ERROR(error);
}

local ERROR_CODE consoleClient_renameEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory, const char* oldName, const uint_fast64_t oldNameLength, const char* newName, const uint_fast64_t newNameLength){
    ERROR_CODE error;

    LinkedList shows;
    if((error = linkedList_init(&shows)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = herder_pullShowList(&shows, remoteHost, remotePort)) != ERROR_NO_ERROR){
        goto label_freeShows;
    }

    EpisodeInfo info;
    if((error = mediaLibrary_initEpisodeInfo(&info)) != ERROR_NO_ERROR){
        goto label_freeShows;
    }

    char* fileName = alloca(sizeof(*fileName) * (oldNameLength + 1));
    memcpy(fileName, oldName, oldNameLength + 1);

    if((error = mediaLibrary_extractEpisodeInfo(&info, &shows, fileName, oldNameLength)) != ERROR_NO_ERROR){
        goto label_freeShows;
    }

    if((error = herder_renameEpisode(remoteHost, remotePort, libraryDirectory, &info, newName, newNameLength)) != ERROR_NO_ERROR){
        UTIL_LOG_CONSOLE_(LOG_ERR, "Server side error, failed to rename episode. [%s]", util_toErrorString(error));

        goto label_freeShows;
    }

    LinkedListIterator it;
label_freeShows:
    linkedList_initIterator(&it, &shows);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

        mediaLibrary_freeShow(show);
        free(show);
    }

    linkedList_free(&shows);

    free(info.showName);
    free(info.name);

label_return:
    return ERROR(error);
}

ERROR_CODE consoleClient_remove(Property* remoteHost, Property* remotePort, Property* libraryDirectory){
    ERROR_CODE error = ERROR_NO_ERROR;

    Show* selectedShow = NULL;
    if((error = consoleClient_selectShow(remoteHost, remotePort, &selectedShow))){
        goto label_return;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Selected show:'%s'.", selectedShow->name);

    Season* selectedSeason = NULL;
    if((error = consoleClient_selectSeason(remoteHost, remotePort, &selectedSeason, selectedShow))){
        goto label_return;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Selected season:'%" PRIdFAST16 "'.", selectedSeason->number);

    Episode* selectedEpisode = NULL;
    if((error = consoleClient_selectEpisode(&selectedEpisode, selectedSeason) != ERROR_NO_ERROR)){
        goto label_return;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Selected:[Episode:%" PRIdFAST16 " '%s'].", selectedEpisode->number, selectedEpisode->name);

label_yesNo:
    UTIL_LOG_CONSOLE_(LOG_INFO, "Remove '%s', s%02" PRIdFAST16 "e%02" PRIdFAST16 " - '%s' from the library? Yes/No.", selectedShow->name, selectedSeason->number, selectedEpisode->number, selectedEpisode->name);

    bool renameEpisode;
    if((error = consoleClient_selectYesNo(&renameEpisode) != ERROR_NO_ERROR)){
        goto label_yesNo;
    }

    if(!renameEpisode){
        goto label_freeNewName;
    }

    EpisodeInfo info;
    mediaLibrary_initEpisodeInfo(&info);
    mediaLibrary_fillEpisodeInfo(&info, selectedShow, selectedSeason, selectedEpisode);

    // Send remove packet.
    if((error = herder_removeEpisode(remoteHost, remotePort, libraryDirectory, &info)) != ERROR_NO_ERROR){
        goto label_freeNewName;
    }

label_freeNewName:
    mediaLibrary_freeShow(selectedShow);

    free(selectedShow);

label_return:
    return ERROR(error);
}

local ERROR_CODE consoleClient_removeEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory, const char* episodeName, const uint_fast64_t episodeNameLength){
    ERROR_CODE error;

    LinkedList shows;
    if((error = linkedList_init(&shows)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if((error = herder_pullShowList(&shows, remoteHost, remotePort)) != ERROR_NO_ERROR){
        goto label_freeShows;
    }

    EpisodeInfo info;
    if((error = mediaLibrary_initEpisodeInfo(&info)) != ERROR_NO_ERROR){
        goto label_freeShows;
    }

    char* fileName = alloca(sizeof(*fileName) * (episodeNameLength + 1));
    memcpy(fileName, episodeName, episodeNameLength + 1);

    if((error = mediaLibrary_extractEpisodeInfo(&info, &shows, fileName, episodeNameLength)) != ERROR_NO_ERROR){
        goto label_freeShows;
    }

    // Send remove packet.
    if((error = herder_removeEpisode(remoteHost, remotePort, libraryDirectory, &info)) != ERROR_NO_ERROR){
        goto label_freeShows;
    }

    LinkedListIterator it;
label_freeShows:
    linkedList_initIterator(&it, &shows);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

        mediaLibrary_freeShow(show);
        free(show);
    }

    linkedList_free(&shows);

    free(info.showName);
    free(info.name);

label_return:
    return ERROR(error);
}

local ERROR_CODE consoleClient_selectShow(Property* remoteHost, Property* remotePort, Show** selection){
    ERROR_CODE error;

    // Pull show list.
    LinkedList shows;
    if((error = herder_pullShowList(&shows, remoteHost, remotePort)) != ERROR_NO_ERROR){
        if(error != ERROR_FAILED_TO_CONNECT){
            goto label_freeShowList;
        }else{
            goto label_return;
        }
    }

    if(shows.length == 0){
        UTIL_LOG_CONSOLE(LOG_INFO, "Library is empty.");

        error = ERROR_EMPTY_RESPONSE;

        goto label_freeShowList;
    }

    // Print shows.
    LinkedListIterator it;
    linkedList_initIterator(&it, &shows);

    int_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

        UTIL_LOG_CONSOLE_(LOG_INFO, "%02" PRIuFAST64 ":'%s'.", i, show->name);
        i++;
    }

    // Select show.
    char* userInput;
    int_fast64_t userInputLength;

label_readUserInput:
    UTIL_LOG_CONSOLE(LOG_INFO, "\nPlease select a show.");

    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    char* showSelection = NULL;

    // Differentiate between selection via show name or via position in list.
    // TODO: Add a check to see if there are shows with names like '1', '2', etc. (jan - 2020.02.08)
    int_fast64_t numberSelection;
    if(util_stringToInt(userInput, &numberSelection) != ERROR_NO_ERROR){
        showSelection = userInput;
    }

    if(numberSelection + 1 > (int_fast64_t) shows.length || numberSelection < 0){
        UTIL_LOG_CONSOLE(LOG_ERR, "Invalid selection.");

        free(userInput);

        goto label_readUserInput;
    }else{
        linkedList_initIterator(&it, &shows);

        i = 0;
        while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
            Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

            if(numberSelection == i++ && showSelection == NULL){
                *selection = show;

                break;
            }

            if(strncmp(userInput, show->name, userInputLength + 1) == 0){
                *selection = show;

                break;
            }
        }
    }

    if(*selection == NULL){
        free(userInput);

        UTIL_LOG_CONSOLE(LOG_ERR, "Invalid selection.");

        goto label_readUserInput;
    }

    LinkedListIterator showIterator;
label_freeShowList:
    linkedList_initIterator(&showIterator, &shows);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&showIterator)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&showIterator);

        if(show != *selection){
            mediaLibrary_freeShow(show);

            free(show);
        }
    }

    linkedList_free(&shows);

    free(userInput);

label_return:
    return ERROR(error);
}

local ERROR_CODE consoleClient_selectSeason(Property* remoteHost, Property* remotePort, Season** season, Show* show){
    ERROR_CODE error;

    if((error = herder_pullShowInfo(remoteHost, remotePort, show)) != ERROR_NO_ERROR){
        goto label_return;
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Seasons:");

    Season** seasons = alloca(sizeof(*seasons) * show->seasons.length);
    mediaLibrary_sortSeasons(&seasons, &show->seasons);

    uint_fast64_t j;
    for(j = 0; j < show->seasons.length; j++){
        UTIL_LOG_CONSOLE_(LOG_INFO, "\t%" PRIuFAST16 ".", seasons[j]->number);
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Please select a season.");

    char* userInput;
    int_fast64_t userInputLength;
    int_fast64_t selection;

label_selectSeason:
    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if(util_stringToInt(userInput, &selection) != ERROR_NO_ERROR){
        free(userInput);

        goto label_selectSeason;
    }

    if(selection <= 0 || (uint_fast16_t) selection > UINT_FAST16_MAX){
        goto label_invalidUserInput;
    }

    uint_fast64_t i;
    for(i = 0; i < show->seasons.length; i++){
        if(seasons[i]->number == (uint_fast16_t) selection){
            *season = seasons[i];

            goto label_return;
        }
    }

label_invalidUserInput:
    free(userInput);

    UTIL_LOG_CONSOLE(LOG_INFO, "Invalid selection.");

    goto label_selectSeason;

label_return:
    free(userInput);

    return ERROR(error);
}

local ERROR_CODE consoleClient_selectEpisode(Episode** episode, Season* season){
    ERROR_CODE error;

    UTIL_LOG_CONSOLE(LOG_INFO, "Episodes:");

    Episode** episodes = alloca(sizeof(*episodes) * season->episodes.length);
    mediaLibrary_sortEpisodes(&episodes, &season->episodes);

    uint_fast64_t j;
    for(j = 0; j < season->episodes.length; j++){
         UTIL_LOG_CONSOLE_(LOG_INFO, "\t%" PRIuFAST16 ":'%s'", episodes[j]->number, episodes[j]->name);
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Please select an episode.");

    char* userInput;
    int_fast64_t userInputLength;
    int_fast64_t selection;

label_selectSeason:
    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    if(util_stringToInt(userInput, &selection) != ERROR_NO_ERROR){
        free(userInput);

        goto label_selectSeason;
    }

    if(selection <= 0 || (uint_fast16_t) selection > UINT_FAST16_MAX){
        goto label_invalidUserInput;
    }

    uint_fast64_t i;
    for(i = 0; i < season->episodes.length; i++){
        if(episodes[i]->number == (uint_fast16_t) selection){
            *episode = episodes[i];

            goto label_return;
        }
    }

label_invalidUserInput:
    free(userInput);

    UTIL_LOG_CONSOLE(LOG_INFO, "Invalid selection.");

    goto label_selectSeason;

label_return:
    free(userInput);

    return ERROR(error);
}

local ERROR_CODE consoleClient_selectYesNo(bool* selection){
    ERROR_CODE error;

    char* userInput;
    int_fast64_t userInputLength;

    if((error = util_readUserInput(&userInput, &userInputLength)) != ERROR_NO_ERROR){
        goto label_freeUserInput;
    }

    util_toLowerChase(userInput);

    if(strncmp(userInput, "no", 3) == 0 || strncmp(userInput, "n", 2) == 0){
        *selection = false;
    }else{
        if(userInputLength != 0 && strncmp(userInput, "yes", 4) != 0 && strncmp(userInput, "y", 2) != 0){
            UTIL_LOG_CONSOLE(LOG_DEBUG, "Invalid selection, enter Yes/No to continue.");

            error = ERROR_INVALID_VALUE;
        }else{
            *selection = true;
        }
    }

label_freeUserInput:
    free(userInput);

    return ERROR(error);
}

ERROR_CODE consoleClient_addEpisode(Property* remoteHost, Property* remotePort, Property* libraryDirectory, const char* path, const uint_fast64_t pathLength){
    ERROR_CODE error;

    EpisodeInfo info;
    if((error = mediaLibrary_initEpisodeInfo(&info)) != ERROR_NO_ERROR){
        goto label_freeInfo;
    }

    LinkedList shows;
    if((error = linkedList_init(&shows)) != ERROR_NO_ERROR){
        goto label_freeInfo;
    }

    if((error = herder_pullShowList(&shows, remoteHost, remotePort)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    info.path = alloca(sizeof(*info.path) * (pathLength + 1));
    memcpy(info.path, path, pathLength + 1);

    info.pathLength = pathLength;

    char* fileName = util_getFileName(info.path, info.pathLength);
    const uint_fast64_t fileNameLength = strlen(fileName);

    if((error = util_getFileExtension(&info.fileExtension, (uint_fast64_t*) &info.fileExtensionLength, fileName, fileNameLength)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    if((error = consoleClient_extractShowInfo(remoteHost, remotePort, &info, false)) != ERROR_NO_ERROR){
        goto label_freeShowList;
    }

    UTIL_LOG_CONSOLE(LOG_INFO, "Importing:...");

    if((error = herder_addEpisode(remoteHost, remotePort, libraryDirectory, &info)) != ERROR_NO_ERROR){
        UTIL_LOG_CONSOLE_(LOG_ERR, "Failed to add '%s' to library. [%s]", path,  util_toErrorString(error));
    }else{
        UTIL_LOG_CONSOLE_(LOG_INFO, "Successfully added '%s' to the library.", path);
    }

    // Free show list.
    LinkedListIterator it;
label_freeShowList:
    linkedList_initIterator(&it, &shows);

    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        Show* show = LINKED_LIST_ITERATOR_NEXT(&it);

        mediaLibrary_freeShow(show);
        free(show);
    }

    linkedList_free(&shows);

label_freeInfo:
    free(info.showName);
    free(info.name);

    return ERROR(error);
}

ERROR_CODE consoleClient_convert(Property* remoteHost, Property* remotePort, Property* libraryDirectory){
    ERROR_CODE error = ERROR_NO_ERROR;

    Show* selectedShow = NULL;
    if((error = consoleClient_selectShow(remoteHost, remotePort, &selectedShow))){
        goto label_return;
    }

    UTIL_LOG_CONSOLE_(LOG_DEBUG, "Selected show:'%s'.", selectedShow->name);

label_yesNo:
    UTIL_LOG_CONSOLE_(LOG_INFO, "Convert '%s' to audio/mp3? Yes/No.", selectedShow->name);

    bool renameEpisode;
    if((error = consoleClient_selectYesNo(&renameEpisode) != ERROR_NO_ERROR)){
        goto label_yesNo;
    }

    if(!renameEpisode){
        goto label_freeNewName;
    }

    if((error = herder_convertToMP3(remoteHost, remotePort, libraryDirectory, selectedShow->name, selectedShow->nameLength)) != ERROR_NO_ERROR){
        goto label_freeNewName;
    }

label_freeNewName:
    mediaLibrary_freeShow(selectedShow);

    free(selectedShow);

label_return:
    return ERROR(error);
}