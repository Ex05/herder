#ifndef UTIL_H
#define UTIL_H

// Use print MACROS for types like uint_fast64_t
#define __STDC_FORMAT_MACROS
 
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <endian.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>

/* void foo(char arg[__require 10])
    Require an array with atleast 10 elements.
*/
#define __require static
#define local static
#define localPersistent static

#define UTIL_FORMATTED_NUMBER_LENGTH 33

#define KB(x)   ((x) << 10)
#define MB(x)   ((x) << 20)
#define GB(x)   ((x) << 30)

#define UTIL_MAX_ERROR_MSG_LENGTH 256

#define UTIL_FLAG(name, size) uint8_t name : size

// Note: These redefenition are needed for the 'read from library file' macros in medialibnrary.c. And are an exception to the rule off the all caps macro style. (jan - 2019.05.17)
#define util_byteArrayTo_uint16_t util_byteArrayTo_uint16

#define util_byteArrayTo_uint32_t util_byteArrayTo_uint32

#define util_byteArrayTo_uint64_t util_byteArrayTo_uint64

typedef enum{
    ERROR_NO_ERROR = 0,
    ERROR_ERROR,
    ERROR_OUT_OF_MEMORY,
    ERROR_BUFFER_OVERFLOW,
    ERROR_TO_MANY_ELEMENTS,
    ERROR_FAILED_TO_UNMAP_MEMORY,
    ERROR_INVALID_REQUEST_URL,
    ERROR_PTHREAD_THREAD_CREATION_FAILED,
    ERROR_PTHREAD_MUTEX_INITIALISATION_FAILED,
    ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED,
    ERROR_UNIX_DOMAIN_SOCKET_INITIALISATION_FAILED,
    ERROR_FAILED_TO_BIND_SERVER_SOCKET,
    ERROR_FAILED_TO_LISTEN_ON_SERVER_SOCKET,
    ERROR_FAILED_TO_FORK_PROCESS,
    ERROR_FAILED_TO_CREATE_NEW_SESSION,
    ERROR_FAILED_TO_CHANGE_PROCESS_WORKING_DIRECTORY,
    ERROR_FAILED_TO_LOCK_FILE,
    ERROR_FAILED_TO_RETRIEVE_ADDRESS_INFORMATION,
    ERROR_FAILED_TO_CONNECT,
    ERROR_WRITE_ERROR,
    ERROR_READ_ERROR,
    ERROR_EMPTY_RESPONSE,
    ERROR_INSUFICIENT_MESSAGE_LENGTH,
    ERROR_INVALID_STATUS_CODE,
    ERROR_VERSION_MISSMATCH,
    ERROR_INVALID_HEADER_FIELD,
    ERROR_MAX_HEADER_FIELDS_REACHED,
    ERROR_INVALID_CONTENT_LENGTH,
    ERROR_MAX_MESSAGE_SIZE_EXCEEDED,
    ERROR_FAILED_TO_CLOSE_SOCKET,
    ERROR_FAILED_TO_APPEMD_SIGNAL_HANDLER,
    ERROR_HTTP_RESPONSE_SIZE_EXCEEDED,
    ERROR_HTTP_REQUEST_SIZE_EXCEEDED,
    ERROR_INCOMPLETE,
    ERROR_FAILED_TO_CREATE_FILE,
    ERROR_DUPLICATE_ENTRY,
    ERROR_PROPERTY_NOT_SET,
    ERROR_SERVER_ERROR,
    ERROR_UNAVAILABLE,
    ERROR_FAILED_TO_RETRIEV_FILE_INFO,
    ERROR_FAILED_TO_LOAD_FILE,
    ERROR_FAILED_TO_CLOSE_FILE,
    ERROR_MISSING_DIRECTORY,
    ERROR_REMOTE_PORT_PROPERTY_NO_SET,
    ERROR_REMOTE_HOST_PROPERTY_NO_SET,
    ERROR_INVALID_RETURN_CODE,
    ERROR_DISK_ERROR,
    ERROR_ENTRY_NOT_FOUND,
    ERROR_LIBRARY_DIRECTORY_NOT_SET,
    ERROR_FAILED_TO_CREATE_DIRECTORY,
    ERROR_FAILED_TO_OPEN_FILE,
    ERROR_M_PROTECT,
    ERROR_MEMORY_ALLOCATION_ERROR,
    ERROR_UNKNOWN_SHOW,
    ERROR_ALREADY_EXIST,
    ERROR_FAILED_TO_COPY_FILE,
    ERROR_FAILED_TO_DELETEFILE,
    ERROR_CONVERSION_ERROR,
    ERROR_FAILED_TO_ADD_PROPERTY,
    ERROR_FAILED_TO_REMOVE_PROPERTY,
    ERROR_INVALID_COMMAND_USAGE,
    ERROR_FAILED_TO_UPDATE_PROPERTY,
    ERROR_INVALID_STRING,
    ERROR_INVALID_VALUE,
    ERROR_FUNCTION_NOT_IMPLEMENTED,
    ERROR_FAILED_TO_OPEN_DIRECTORY,
    ERROR_NAME_MISSMATCH,
    ERROR_END_OF_FILE,
    ERROR_FAILED_TO_DELETE_DIRECTORY
}ERROR_CODE;

ERROR_CODE util_formatNumber(char*, uint_fast64_t*, const int_fast64_t);

const char* util_toErrorString(const ERROR_CODE);

#define ERROR_CALLBACK(functionName) void functionName(const int_fast32_t errorCode, const char* file, const int line, char* errorMessage)
typedef ERROR_CALLBACK(ErrorCallback);

typedef struct {
    int_fast32_t errorType;

    union{
        struct{
            UTIL_FLAG(supressNextError, 1);
            UTIL_FLAG(supressAllErrors, 1);
            UTIL_FLAG(supressNextErrorOfType, 1);
        };

        uint8_t flags;
    };
}ERROR_Supressor;

ERROR_Supressor global_errorSupressionStruct = {0};

#define __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(error) do{ \
    global_errorSupressionStruct.supressNextErrorOfType = true; \
    global_errorSupressionStruct.errorType = error; \
    } while(0)

#define __UTIL_SUPRESS_NEXT_ERROR__(void) do{ \
    global_errorSupressionStruct.supressNextError = true; \
}while(0)

#define __UTIL_DISABLE_ERROR_LOGGING__(void) do{ \
    global_errorSupressionStruct.supressAllErrors = true; \
}while(0)

#define __UTIL_ENABLE_ERROR_LOGGING__(void) do{ \
    global_errorSupressionStruct.supressAllErrors = false; \
}while(0)

#ifdef DEBUG
    static ERROR_CALLBACK(globbal_errorCallback){
        if(errorCode != ERROR_NO_ERROR && (
                !global_errorSupressionStruct.supressAllErrors &&
                !global_errorSupressionStruct.supressNextError && (
                    !global_errorSupressionStruct.supressNextErrorOfType &&
                    global_errorSupressionStruct.errorType != errorCode))){

            util_toErrorString(errorCode);
            syslog(LOG_ERR, "%s:%d %s [%" PRIdFAST32 "]%s%s.", file, line, util_toErrorString(errorCode), errorCode, errorMessage[0] == '\0' ? "" : " ", errorMessage);
        }

        global_errorSupressionStruct.supressNextErrorOfType = false;
        global_errorSupressionStruct.supressNextError = false;

        return;
    }

    static ErrorCallback* errorCallback_ = globbal_errorCallback;
    #define GLOBAL_ERROR_CALLBACK errorCallback_
#endif

local ERROR_CODE util_error(const ERROR_CODE, const char*, const int, char*, ...);

inline ERROR_CODE util_error(const ERROR_CODE error, const char* file, const int line, char* format, ...){
    #ifdef DEBUG
        char* msg = alloca(sizeof(*msg) * UTIL_MAX_ERROR_MSG_LENGTH);

        va_list args;
        va_start(args, format);

        vsnprintf(msg, UTIL_MAX_ERROR_MSG_LENGTH, format, args);

        va_end(args);

        GLOBAL_ERROR_CALLBACK(error, file, line, msg);
    #endif

    return error;
}

#define ERROR(error) util_error(error, __FILE__, __LINE__, "")

#define ERROR_(error, format, ...) util_error(error, __FILE__, __LINE__, format, __VA_ARGS__)

#define UTIL_LOG(level, formatString, ...) syslog(level, "%s:%d " formatString, __FILE__, __LINE__, __VA_ARGS__)
#define UTIL_LOG_CONSOLE_(level, formatString, ...) do{ \
    UTIL_LOG(level, formatString, __VA_ARGS__); \
    printf(formatString "\n", __VA_ARGS__); \
    } while(0)
#define UTIL_LOG_CONSOLE(level, formatString) do{ \
    UTIL_LOG(level, "%s", formatString); \
    printf("%s\n", formatString); \
    } while(0)
#define UTIL_LOG_NOTICE_(formatString, ...) UTIL_LOG(LOG_NOTICE, formatString, __VA_ARGS__)
#define UTIL_LOG_NOTICE(formatString) UTIL_LOG(LOG_NOTICE, "%s", formatString)
#define UTIL_LOG_INFO_(formatString, ...) UTIL_LOG(LOG_INFO, formatString, __VA_ARGS__)
#define UTIL_LOG_INFO(formatString) UTIL_LOG(LOG_INFO, "%s", formatString)
#define UTIL_LOG_ALERT_(formatString, ...) UTIL_LOG(LOG_ALERT, formatString, __VA_ARGS__)
#define UTIL_LOG_ALERT(formatString) UTIL_LOG(LOG_ALERT, "%s", formatString)
#define UTIL_LOG_CRITICAL_(formatString, ...) UTIL_LOG(LOG_CRIT, formatString, __VA_ARGS__)
#define UTIL_LOG_CRITICAL(formatString) UTIL_LOG(LOG_CRIT, "%s", formatString)
#define UTIL_LOG_WARNING_(formatString, ...) UTIL_LOG(LOG_WARNING, formatString, __VA_ARGS__)
#define UTIL_LOG_WARNING(formatString) UTIL_LOG(LOG_WARNING, "%s", formatString)
#define UTIL_LOG_DEBUG_(formatString, ...) UTIL_LOG(LOG_DEBUG, formatString, __VA_ARGS__)
#define UTIL_LOG_DEBUG(formatString) UTIL_LOG(LOG_DEBUG, "%s", formatString)
#define UTIL_LOG_ERROR_(formatString, ...) UTIL_LOG(LOG_ERR, formatString, __VA_ARGS__)
#define UTIL_LOG_ERROR(formatString) UTIL_LOG(LOG_ERR, "%s", formatString)

#define UTIL_ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define UTIL_UINT16_STRING_LENGTH 5

int_fast32_t util_fileExists(const char*);

ERROR_CODE util_blockAlloc(void**, const uint_fast64_t);

ERROR_CODE util_unMap(void*, const uint_fast64_t);

void util_concatenate(char*, const uint_fast64_t, const char*, const uint_fast64_t, const char*, const uint_fast64_t);

uint_fast16_t util_byteArrayTo_uint16(const int8_t*);

uint_fast32_t util_byteArrayTo_uint32(const int8_t*);

uint_fast64_t util_byteArrayTo_uint64(const int8_t*);

int8_t* util_uint16ToByteArray(int8_t*, const uint_fast16_t);

int8_t* util_uint32ToByteArray(int8_t*, const uint_fast32_t);

int8_t* util_uint64ToByteArray(int8_t*, const uint_fast64_t);

void util_toLowerChase(char*);

void util_stringCopy(char*, char*, uint_fast64_t);

int_fast64_t util_findFirst(const char*, const uint_fast64_t, const char);

int_fast64_t util_findFirst_s(const char*, const uint_fast64_t, const char*, const uint_fast64_t);

int_fast64_t util_findLast(const char*, const uint_fast64_t, const char);

ERROR_CODE util_fileCopy(const char*, const char*);

uint_fast32_t util_getFileSystemBlockSize(const char*);

ERROR_CODE util_deleteFile(const char*);

void util_replaceAllChars(char*, const char, const char);

void util_append(char*, const uint_fast64_t, char*, const uint_fast64_t);

int util_hashString(const char*, uint_fast64_t);

ERROR_CODE util_createDirectory(const char*);

ERROR_CODE util_createAllDirectories(const char*, const uint_fast64_t);

bool util_isDirectory(const char*);

bool util_startsWith(const char*, const char);

int_fast32_t util_getNumAvailableProcessorCores(void);

ERROR_CODE util_getBaseDirectory(char**, uint_fast64_t*, char*, uint_fast64_t);

ERROR_CODE util_replace(char*, const uint_fast64_t, uint_fast64_t*, const char*,  const uint_fast64_t, const char*, const uint_fast64_t);

char* util_trim(char*, const uint_fast64_t);

void util_printBuffer(void*, uint_fast64_t);

char* util_getHomeDirectory(void);

char* util_getFileName(char*, const uint_fast64_t);

ERROR_CODE util_readUserInput(char**, int_fast64_t*);

ERROR_CODE util_stringToInt(const char*, int64_t*);

ERROR_CODE util_getCurrentWorkingDirectory(char*, const uint_fast64_t);

ERROR_CODE util_getFileExtension(char**, char*, const uint_fast64_t);

#undef UTIL_MAX_ERROR_MSG_LENGTH

#endif

