#ifndef UTIL_C
#define UTIL_C

#include "util.h"

/**
 * Converts a given number <b>value</b> into a easy readable formated number string.
 *
 * Numbers <= to 999 will be padded with 0.<br>
 * And every 3 digits an underscore('_') will be inserted.
 * @param s The buffer in which the formated number will be put.
 * @param bufferSize The size of the buffer.
 * @param value The value to be converted.
 * @return Returns <b>ERROR_NO_ERROR</b> on success. And <b>ERROR_BUFFER_OVERFLOW</b> if the provided buffer was not large enough.
*/
inline ERROR_CODE util_formatNumber(char* s, uint_fast64_t* bufferSize, const int_fast64_t value){
	int_fast64_t length = snprintf(s, *bufferSize, value > 999 ? "%" PRIdFAST64 : "%03" PRIdFAST64, value);

    if(length >= (int_fast64_t) *bufferSize){
        UTIL_LOG_CONSOLE(LOG_ERR, "Buffer Overflow.");

        return ERROR(ERROR_BUFFER_OVERFLOW);
    }

#define SKIP_VALUE 3
    *bufferSize = length;

    if(value < 1000){
        return ERROR(ERROR_NO_ERROR);
    }

    uint_fast8_t skip = SKIP_VALUE;
    int_fast64_t i;
    for(i = length; i >= 0; i--){
        if(length + 1 < (int_fast64_t) *bufferSize){
            return ERROR(ERROR_BUFFER_OVERFLOW);
        }

        if(--skip == 0 && i > 0){
            int_fast64_t j;
            for(j = length; j >= i - 1; j--){
                s[j + 1] = s[j];
            }

            s[j + 1] = '_';
            length++;

            skip = SKIP_VALUE;
        }
    }
#undef SKIP_VALUE

    return ERROR(ERROR_NO_ERROR);
}

local const char* UTIL_ERROR_CODE_MESSAGE_MAPPING_ARRAY[] = {
    "ERROR_NO_ERROR",
    "ERROR_ERROR",
    "ERROR_OUT_OF_MEMORY",
    "ERROR_BUFFER_OVERFLOW",
    "ERROR_TO_MANY_ELEMENTS",
    "ERROR_FAILED_TO_UNMAP_MEMORY",
    "ERROR_INVALID_REQUEST_URL",
    "ERROR_PTHREAD_THREAD_CREATION_FAILED",
    "ERROR_PTHREAD_MUTEX_INITIALISATION_FAILED",
    "ERROR_PTHREAD_SEMAPHOR_INITIALISATION_FAILED",
    "ERROR_UNIX_DOMAIN_SOCKET_INITIALISATION_FAILED",
    "ERROR_FAILED_TO_BIND_SERVER_SOCKET",
    "ERROR_FAILED_TO_LISTEN_ON_SERVER_SOCKET",
    "ERROR_FAILED_TO_FORK_PROCESS",
    "ERROR_FAILED_TO_CREATE_NEW_SESSION",
    "ERROR_FAILED_TO_CHANGE_PROCESS_WORKING_DIRECTORY",
    "ERROR_FAILED_TO_LOCK_FILE",
    "ERROR_FAILED_TO_RETRIEVE_ADDRESS_INFORMATION",
    "ERROR_FAILED_TO_CONNECT",
    "ERROR_WRITE_ERROR",
    "ERROR_READ_ERROR",
    "ERROR_EMPTY_RESPONSE",
    "ERROR_INSUFICIENT_MESSAGE_LENGTH",
    "ERROR_INVALID_STATUS_CODE",
    "ERROR_VERSION_MISSMATCH",
    "ERROR_INVALID_HEADER_FIELD",
    "ERROR_MAX_HEADER_FIELDS_REACHED",
    "ERROR_INVALID_CONTENT_LENGTH",
    "ERROR_MAX_MESSAGE_SIZE_EXCEEDED",
    "ERROR_FAILED_TO_CLOSE_SOCKET",
    "ERROR_FAILED_TO_APPEMD_SIGNAL_HANDLER",
    "ERROR_HTTP_RESPONSE_SIZE_EXCEEDED",
    "ERROR_HTTP_REQUEST_SIZE_EXCEEDED",
    "ERROR_INCOMPLETE",
    "ERROR_FAILED_TO_CREATE_FILE",
    "ERROR_DUPLICATE_ENTRY",
    "ERROR_PROPERTY_NOT_SET",
    "ERROR_SERVER_ERROR",
    "ERROR_UNAVAILABLE",
    "ERROR_FAILED_TO_RETRIEV_FILE_INFO",
    "ERROR_FAILED_TO_LOAD_FILE",
    "ERROR_FAILED_TO_CLOSE_FILE",
    "ERROR_MISSING_DIRECTORY",
    "ERROR_REMOTE_PORT_PROPERTY_NO_SET",
    "ERROR_REMOTE_HOST_PROPERTY_NO_SET",
    "ERROR_INVALID_RETURN_CODE",
    "ERROR_DISK_ERROR",
    "ERROR_ENTRY_NOT_FOUND",
    "ERROR_LIBRARY_DIRECTORY_NOT_SET",
    "ERROR_FAILED_TO_CREATE_DIRECTORY",
    "ERROR_FAILED_TO_OPEN_FILE",
    "ERROR_M_PROTECT",
    "ERROR_MEMORY_ALLOCATION_ERROR",
    "ERROR_UNKNOWN_SHOW",
    "ERROR_ALREADY_EXIST",
    "ERROR_FAILED_TO_COPY_FILE",
    "ERROR_FAILED_TO_DELETEFILE",
    "ERROR_CONVERSION_ERROR",
    "ERROR_FAILED_TO_ADD_PROPERTY",
    "ERROR_FAILED_TO_REMOVE_PROPERTY",
    "ERROR_INVALID_COMMAND_USAGE",
    "ERROR_FAILED_TO_UPDATE_PROPERTY",
    "ERROR_INVALID_STRING",
    "ERROR_INVALID_VALUE",
    "ERROR_FUNCTION_NOT_IMPLEMENTED"
};

inline const char* util_toErrorString(const ERROR_CODE errorCode){
    return UTIL_ERROR_CODE_MESSAGE_MAPPING_ARRAY[errorCode];
}

inline int_fast32_t util_fileExists(const char *file){
    struct stat st = {0};
    
    return stat(file, &st) == 0;
}

inline ERROR_CODE util_blockAlloc(void** buffer, const uint_fast64_t length){
    *buffer = mmap(NULL, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(*buffer == MAP_FAILED){
        return ERROR_(ERROR_MEMORY_ALLOCATION_ERROR, "Failed to create memory page '%s'.", strerror(errno));
    }

    if(mprotect(*buffer, length, PROT_READ | PROT_WRITE) != 0){
        return ERROR_(ERROR_M_PROTECT, "Failed to protect memory page '%s'.", strerror(errno));
    }else{
        return ERROR(ERROR_NO_ERROR);
    }
}

inline ERROR_CODE util_unMap(void* buffer, const uint_fast64_t length){
    if(munmap(buffer, length) == -1){
        return ERROR_(ERROR_FAILED_TO_UNMAP_MEMORY, "'%s'.", strerror(errno));
    }

    return ERROR(ERROR_NO_ERROR);
}

inline void util_concatenate(char* dst, const uint_fast64_t lengthDst, const char* a, const uint_fast64_t lengthA, const char* b, const uint_fast64_t lengthB) {
	strncpy(dst, a, lengthA);

	if(lengthDst - lengthA > 0)
		strncpy(dst + lengthA, b, lengthB);
}

inline uint_fast16_t util_byteArrayTo_uint16(const int8_t* buffer){
    return (uint_fast16_t)(
		((uint8_t) buffer[0]) << 8 | 
		(uint8_t) buffer[1]);
}

inline uint_fast32_t util_byteArrayTo_uint32(const int8_t* buffer){
    return (uint_fast32_t)((uint8_t) buffer[0]) << 24 |
            ((uint8_t) buffer[1]) << 16 |
            ((uint8_t) buffer[2]) << 8 |
            ((uint8_t) buffer[3]);
}

inline uint_fast64_t util_byteArrayTo_uint64(const int8_t* buffer){
    return (uint_fast64_t)(((uint64_t) ((uint8_t) buffer[0]) << 56) |
            ((uint64_t)((uint8_t) buffer[1]) << 48) |
            ((uint64_t)((uint8_t) buffer[2]) << 40) |
            ((uint64_t)((uint8_t) buffer[3]) << 32) |
            ((uint64_t)((uint8_t) buffer[4]) << 24) |
            ((uint64_t)((uint8_t) buffer[5]) << 16) |
            ((uint64_t)((uint8_t) buffer[6]) << 8) |
            ((uint8_t) buffer[7]));
}

inline int8_t* util_uint16ToByteArray(int8_t* buffer, const uint_fast16_t value){
    buffer[0] = (value >> 8) & 0XFF;
    buffer[1] = value & 0XFF;

    return buffer;
}

inline int8_t* util_uint32ToByteArray(int8_t* buffer, const uint_fast32_t value){
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0XFF;
    buffer[3] = value & 0XFF;

    return buffer;
}

inline int8_t* util_uint64ToByteArray(int8_t* buffer, const uint_fast64_t value){
    buffer[0] = (value >> 56) & 0xFF;
    buffer[1] = (value >> 48) & 0xFF;
    buffer[2] = (value >> 40) & 0XFF;
    buffer[3] = (value >> 32) & 0XFF;
    buffer[4] = (value >> 24) & 0XFF;
    buffer[5] = (value >> 16) & 0XFF;
    buffer[6] = (value >>  8) & 0XFF;
    buffer[7] = value & 0XFF;

    return buffer;
}

inline void util_toLowerChase(char* s){
    for ( ; *s; ++s) {
        *s = tolower(*s);
    }
}

inline void util_stringCopy(char* a, char* b, uint_fast64_t length){
    memmove(a, b, length);
}

inline int_fast64_t util_findFirst(const char* s, const uint_fast64_t length, const char c) {    
    int_fast64_t i;    
    for (i = 0; i < (int_fast64_t) length; i++){
        if(*(s + i) == c){
            return i;
        }
    }

    return -1;
}

inline int_fast64_t util_findFirst_s(const char* buffer, const uint_fast64_t bufferLength, const char* s, const uint_fast64_t stringLength) {
    uint_fast64_t offset = 0;
    int_fast64_t i;
    for (i = 0; i < (int_fast64_t) bufferLength; i++){
        if(buffer[i] == s[offset++]){
            if(offset == stringLength){
                return i - (stringLength - 1);
            }
        }else{
            offset = 0;
        }
    }

    return -1;
}

inline int_fast64_t util_findLast(const char* s, const uint_fast64_t length, const char c) {    
    int_fast64_t i;    
    for (i = length - 1; i >= 0; i--){
        if(*(s + i) == c){
            return i;
        }
    }

    return -1;
}

// TODO: Do some testing, what is actually the fastest way to copy a file on each platform. See: https://stackoverflow.com/questions/10195343/copy-a-file-in-a-sane-safe-and-efficient-way for some example implementations. (Jan - 2018.11.20)
inline ERROR_CODE util_fileCopy(const char* src, const char* dst){
    ERROR_CODE error = ERROR_NO_ERROR;

    FILE* srcFile = fopen(src, "r");
    if(srcFile == NULL){
        error = ERROR_FAILED_TO_OPEN_FILE;

        UTIL_LOG_ERROR_("Failed to open source file: '%s'.", src);

        goto label_return;
    }

    FILE* dstFile = fopen(dst, "w+");
    if(dstFile == NULL){
        error = ERROR_FAILED_TO_OPEN_FILE;

        UTIL_LOG_ERROR_("Failed to open destination file: '%s'.", dst);

        goto label_closeSrc;
    }

    const uint_fast32_t bufferSize = util_getFileSystemBlockSize(src);

    // TODO: Decide if putting the buffer on the stack is a good idea. (Jan - 2018.11.17)
    int8_t* buffer = alloca(bufferSize);

    uint_fast64_t numBytesToWrite;
    for(;;){
        numBytesToWrite = fread(buffer, sizeof(*buffer), bufferSize, srcFile);

        if(ferror(srcFile) != 0){
            error = ERROR_READ_ERROR;

            UTIL_LOG_ERROR_("Failed to read file '%s'.", src);

            break;
        }

        uint_fast64_t numBytesWritten = fwrite(buffer, sizeof(*buffer), numBytesToWrite, dstFile);

        if(numBytesWritten != numBytesToWrite || ferror(dstFile) != 0){
            error = ERROR_WRITE_ERROR;

            UTIL_LOG_ERROR_("Failed to copy data from '%s' to '%s' %" PRIu64" out of %" PRIu64 "bytes written in current block.", src, dst, numBytesWritten, numBytesToWrite);

            break;
        }
        
        if(feof(srcFile) != 0){
            break;
        }

    }

    fclose(dstFile);

label_closeSrc:
    fclose(srcFile);

label_return:
    return ERROR(error);
}

inline uint_fast32_t util_getFileSystemBlockSize(const char* path){
        struct statvfs stat;
        if(statvfs(path, &stat) == 0){
            return stat.f_bsize;
        }else{
            return 8192;
        }
}

inline ERROR_CODE util_deleteFile(const char* file){
    if(unlink(file) == 0){
        return ERROR(ERROR_NO_ERROR);
    }else{
        return ERROR_(ERROR_FAILED_TO_DELETEFILE, "'%s' %s", file, strerror(errno));
    }
}

// http://www.stroustrup.com/new_learning.pdf
char* util_readUserInput(void){
    uint_fast16_t limit = 32;
    char* s = malloc(limit);

    // Skip leading whitespaces.
    while (true) {
        int_fast32_t c = fgetc(stdin);

        if(c == EOF){
            break;
        }

        if(!isspace(c)) {
             ungetc(c, stdin);

             break;
        }
    }

    uint_fast64_t i = 0;
    while (true) {
        int_fast32_t c = fgetc(stdin);
    
        if(c == '\n' || c == '\0' || c == EOF){
            s[i] = 0;

            break;
        }

        s[i] = c;

        if(i == limit - 1) {
            limit *= 2;

            s = realloc(s, limit);
        }

        i++;
    }

    return s;
}

inline void util_replaceAllChars(char* s, const char a, const char b){
    do{
        if(*s == a)
            *s = b;
    }while(*s++);
}

// NOTE:(jan) If both strings are not '\0' terminated, strcat/strncat will jump/move based on unitialised values.
inline void util_append(char* a, const uint_fast64_t lengthA, char* b, const uint_fast64_t lengthB){
    strncat(a, b, lengthB - (lengthA - lengthB));
}

// sdbm - hash implementation. see(http://www.cse.yorku.ca/~oz/hash.html)
inline int util_hashString(const char* s, uint_fast64_t length){
    int hash = 0;

    int c;
    while (length-- != 0){
        c = *s++;

        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

inline ERROR_CODE util_createDirectory(const char* directory){
    if(!util_fileExists(directory)){
        if(mkdir(directory, 0700) == 0){
         return ERROR(ERROR_NO_ERROR);
        }else{
            return ERROR_(ERROR_FAILED_TO_CREATE_DIRECTORY, "\"%s\" - %s", directory, strerror(errno));
        }
    }else{
        return ERROR_(ERROR_ALREADY_EXIST, "\"%s\"", directory);
    }
}

inline ERROR_CODE util_createAllDirectories(const char* path, const uint_fast64_t pathLength){
    ERROR_CODE error = ERROR_NO_ERROR;

    char* _path = alloca(sizeof(*_path) * (pathLength + 1));
    strncpy(_path, path, pathLength);
    _path[pathLength] = '\0';

    uint_fast64_t searchOffset = 1;
    for(;;){
        const int_fast64_t offset = util_findFirst(_path + searchOffset, pathLength - searchOffset, '/');

        if(offset == -1){
            break;
        }

        const uint_fast64_t index = searchOffset + offset + 1;

        const char tmp = _path[index];
        _path[index] = '\0';

        __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ALREADY_EXIST);
        if((error = util_createDirectory(_path)) != ERROR_NO_ERROR){
           if(error != ERROR_ALREADY_EXIST){
               break;
           }else{
               error = ERROR_NO_ERROR;
           }
        }

        _path[index] = tmp;

        searchOffset += offset + 1;
    }

    return ERROR(error);
}

inline bool util_isDirectory(const char* file_path){
	struct stat fileInfo = {0};
	lstat(file_path, &fileInfo);

	return S_ISDIR(fileInfo.st_mode);
}

inline bool util_startsWith(const char* s, const char c){
    return s[0] == c;
}

inline int_fast32_t util_getNumAvailableProcessorCores(void){
    return sysconf(_SC_NPROCESSORS_ONLN);
}

inline ERROR_CODE util_getBaseDirectory(char** baseDirectory, uint_fast64_t* baseDirectoryLength, char* url, uint_fast64_t urlLength){
    const int_fast64_t firstSeperator = util_findFirst(url, urlLength, '/');

    if(firstSeperator == -1){
        return ERROR(ERROR_INVALID_REQUEST_URL);
    // URL: /
    }else{
        const int_fast64_t secondSeperator = util_findFirst(url + firstSeperator + 1, urlLength - (firstSeperator + 1), '/');

        if(secondSeperator == -1){
            if(util_findLast(url + firstSeperator + 1, urlLength - (firstSeperator + 1) , '.') == -1){
                // URL: /img
                *baseDirectory = url + firstSeperator;
                *baseDirectoryLength = urlLength - firstSeperator;
            }else{
                // URL: /img_001.png
                *baseDirectory = url + firstSeperator;
                *baseDirectoryLength = 1;
            }

            return ERROR(ERROR_NO_ERROR);
        }else{
            // URL: /img/img_001.png
            *baseDirectory = url + firstSeperator;
            *baseDirectoryLength = (secondSeperator - firstSeperator) + 1/*Include the /*/;

            return ERROR(ERROR_NO_ERROR);
        }
    }
}

inline ERROR_CODE util_replace(char* buffer, const uint_fast64_t bufferLength, uint_fast64_t* srcStringLength, const char* a, const uint_fast64_t stringLengthA, const char* b, const uint_fast64_t stringLengthB){
    int_fast64_t searchOffset = 0;
    int_fast64_t offset = 0;
    while((offset = util_findFirst_s(buffer + searchOffset, *srcStringLength - offset, a, stringLengthA)) != -1){        
        searchOffset += offset;
        // We can replace the string inplace.
        if(stringLengthB == stringLengthA){
            memcpy(buffer + searchOffset, b, stringLengthB);
        }else{
            if(stringLengthB < stringLengthA){
                uint_fast64_t i;
                for(i = searchOffset + stringLengthB; i < *srcStringLength + 1; i++){
                    buffer[i] = buffer[i + (stringLengthA - stringLengthB)];
                }

                memcpy(buffer + searchOffset, b, stringLengthB);

                *srcStringLength -= stringLengthA - stringLengthB;
            }else{
                if(*srcStringLength + (stringLengthB - stringLengthA) > bufferLength){
                    return ERROR(ERROR_BUFFER_OVERFLOW);
                }

                uint_fast64_t i;
                // searchOffset will always be >= 0, so we can safely cast to unsigned here.
                for(i = 1 + *srcStringLength; i >= ((uint_fast64_t) searchOffset) + stringLengthA; i--){
                    buffer[i + (stringLengthB - stringLengthA)] = buffer[i];
                }

                memcpy(buffer + searchOffset, b, stringLengthB);

                *srcStringLength += stringLengthB - stringLengthA;
            }
        }

        searchOffset += stringLengthB;
    }

    return ERROR(ERROR_NO_ERROR);
}

inline char* util_trim(char* s, const uint_fast64_t length) {
    char c;
    uint_fast64_t i = -1;
    do{
        c = *(s + (length - ++i) - 1);
    }while(isspace(c));

    *(s + length - i) = '\0';

    i = -1;
    do{
        c = *(s + ++i);
    }while(isspace(c));

    uint_fast64_t j;
    for (j = 0; j < length; j++){
        *(s + j) = *(s + j + i);
    }

    return s;
}

inline void util_printBuffer(void* buffer, uint_fast64_t length){
uint_fast64_t i;
    for(i = 0; i < length; i++){
        printf("%c", ((char*) buffer)[i]);
    }

    printf("\n");
}

inline char* util_getHomeDirectory(void){
    #ifdef __linux__ 
        char* homeDir = getenv("HOME");

        if(homeDir != NULL){
            return homeDir;
        }else{
            return getpwuid(getuid())->pw_dir;;
        }
    #elif _WIN32
        ERROR: FUNCTION NOT IMPLEMENTED.
    #else
        ERROR: FUNCTION NOT IMPLEMENTED.
    #endif
}

inline char* util_getFileName(char* filePath, const uint_fast64_t filePathLength){
    return filePath + (util_findLast(filePath, filePathLength, '/') + 1);
}

inline ERROR_CODE util_extractPrefixedNumber(char* s, uint_fast64_t stringLength, int_fast16_t* value, const char prefix){
    int_fast64_t offset;
    while((offset = util_findFirst(s, stringLength, prefix)) != -1){
        s += ++offset;
        stringLength -= offset;

        uint_fast64_t i;
        for(i = 0; i < stringLength; i++){
            if(isdigit(*(s + i)) == 0){
                break;
            }
        }

        if(i > 0){
            char* numberString = alloca(sizeof(*numberString) * (i + 1));
            strncpy(numberString, s, i);
            numberString[i] = '\0';

            util_stringCopy(s - 1, s + i, strlen(s) + 1 - i);

            char* endPtr;
            *value = strtol(numberString, &endPtr, 10);

            if((*value == 0 && endPtr == numberString) || *value == LONG_MIN || *value == LONG_MAX){
                return ERROR_(ERROR_CONVERSION_ERROR, "'%s' is not a valid number.", numberString);
            }

            return ERROR(ERROR_NO_ERROR);
        }
    }

    *value = 0;

    return ERROR(ERROR_ENTRY_NOT_FOUND);
}

inline ERROR_CODE util_stringToInt(const char* s, int64_t* value){
    char* endPtr;
    *value = strtol(s, &endPtr, 10/*Decimal*/);

    if((*value == 0 && endPtr == s) || *value == LONG_MIN || *value == LONG_MAX){
        return ERROR_(ERROR_INVALID_STRING, "'%s' is not a valid number.", s);
    }

    return ERROR(ERROR_NO_ERROR);
}

// Note: Use PATH_MAX, for the size of 'dir'. (Jan- 2019.01.23)
inline ERROR_CODE util_getCurrentWorkingDirectory(char* dir, const uint_fast64_t dirLength){
    if(getcwd(dir, dirLength) == NULL){
        return ERROR_(ERROR_ERROR, "%s", strerror(errno));
    }
    
    return ERROR(ERROR_NO_ERROR);    
}

ERROR_CODE util_getFileExtension(char** extension, char* fileName, const uint_fast64_t fileNameLength){
    const int_fast64_t fileExtensionOffset = util_findLast(fileName, fileNameLength, '.');

    if(fileExtensionOffset == -1){
        extension = NULL;

        return ERROR_(ERROR_INVALID_STRING, "%s does not contain the token '.'", fileName);
    }

    *extension = fileName + (fileExtensionOffset + 1);

    return ERROR(ERROR_NO_ERROR);
}

#endif