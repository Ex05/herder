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

#define CONSOLE_CLIENT_PROGRAM_NAME "herder"

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
    
    ARGUMENT_PARSER_ADD_ARGUMENT(Help, 3, "-?", "-h", "--help");
    ARGUMENT_PARSER_ADD_ARGUMENT(AddShow, 1, "--addShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveShow, 1, "--removeShow");
    ARGUMENT_PARSER_ADD_ARGUMENT(Add, 4, "-a", "-add" "--addFile", "--addEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import, 2, "-i", "--import");
    ARGUMENT_PARSER_ADD_ARGUMENT(RenameEpisode, 1, "--renameEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(RemoveEpisode, 1, "--removeEpisode");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListShows, 3, "-l", "--list", "--listShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ListAll, 2, "--listAll", "--listShows");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowInfo, 1, "--showInfo");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetImportDirectory, 1, "--setImportDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetLibraryDirectory, 1, "--setLibraryDirectory");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHost, 1, "--setRemoteHost");
    ARGUMENT_PARSER_ADD_ARGUMENT(SetRemoteHostPort, 1, "--setRemotePort");
    ARGUMENT_PARSER_ADD_ARGUMENT(ShowSettings, 1, "--showSettings");

    ERROR_CODE error;
    if((error = argumentParser_parse(&parser, argc, argv)) != ERROR_NO_ERROR){
        if(error == ERROR_NO_VALID_ARGUMENT){
            UTIL_LOG_CONSOLE(LOG_ERR, "No valid command line arguments.\n");    

            goto label_printHelp;
        }else{
            UTIL_LOG_CONSOLE(LOG_ERR, "Failed to parse command line arguments.");    
        }

		goto label_free;
    }
       if(argumentParser_contains(&parser, &argumentHelp)){ 
            goto label_printHelp;
    }

label_printHelp:
    consoleClient_printHelp();

label_free:
	argumentParser_free(&parser);

	closelog();

    return EXIT_SUCCESS;
}

inline void consoleClient_printHelp(void){
    UTIL_LOG_CONSOLE(LOG_INFO, "Usage: herder --[command]/-[alias] <arguments>.\n\n");

}
