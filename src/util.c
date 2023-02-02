#ifndef UTIL_C
#define UTIL_C

#include "util.h"
#include <stdint.h>
#include <sys/syslog.h>

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
		return ERROR_(ERROR_BUFFER_OVERFLOW, "%s", util_toErrorString(ERROR_BUFFER_OVERFLOW));
	}

#define SKIP_VALUE 3
	*bufferSize = length;

	if(value < 1000){
		return ERROR(ERROR_NO_ERROR);
	}

	uint_fast8_t skip = SKIP_VALUE;
	int_fast64_t i;
	for(i = length; i >= 0; i--){
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
	"ERROR_DOES_NOT__EXIST",
	"ERROR_FAILED_TO_COPY_FILE",
	"ERROR_FAILED_TO_DELETEFILE",
	"ERROR_CONVERSION_ERROR",
	"ERROR_FAILED_TO_ADD_PROPERTY",
	"ERROR_FAILED_TO_REMOVE_PROPERTY",
	"ERROR_INVALID_COMMAND_USAGE",
	"ERROR_FAILED_TO_UPDATE_PROPERTY",
	"ERROR_INVALID_STRING",
	"ERROR_INVALID_VALUE",
	"ERROR_FUNCTION_NOT_IMPLEMENTED",
	"ERROR_FAILED_TO_OPEN_DIRECTORY",
	"ERROR_NAME_MISSMATCH",
	"ERROR_END_OF_FILE",
	"ERROR_FAILED_TO_DELETE_DIRECTORY",
	"ERROR_FAILED_TO_REMOVE_NODE",
	"ERROR_NO_VALID_ARGUMENT",
	"ERROR_INVALID_FILE_EXTENSION",
	"ERROR_FAILED_TO_RENAME_FILE",
	"ERROR_RETRY_AGAIN",
	"ERRROR_MISSING_PROPERTY",
	"ERROR_SSL_INITIALISATION_ERROR",
	"ERROR_FAILED_TO_INITIALISE_EPOLL",
	"ERROR_INVALID_SIGNAL",
	"ERROR_FILE_NOT_FOUND",
	"ERROR_NOT_A_NUMBER",
};

inline const char* util_toErrorString(const ERROR_CODE errorCode){
	return UTIL_ERROR_CODE_MESSAGE_MAPPING_ARRAY[errorCode];
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


inline uint_fast32_t util_getFileSystemBlockSize(const char* path){
	struct statvfs stat;
	if(statvfs(path, &stat) == 0){
		return stat.f_bsize;
	}else{
		return 8192;
	}
}

inline bool util_stringStartsWith(const char* s, const char c){
	return s[0] == c;
}

inline bool util_stringEndsWith(const char* s, const uint_fast64_t stringLength, const char c){
	return s[stringLength - 1] == c;
}

inline bool util_stringStartsWith_s(const char* a, const char* b, const uint_fast64_t lengthB){
	return strncmp(a, b, lengthB) == 0;
}

inline ERROR_CODE util_deleteFile(const char* file){
	if(unlink(file) != 0){
		return ERROR_(ERROR_FAILED_TO_DELETEFILE, "'%s' %s", file, strerror(errno));
	}

	return ERROR(ERROR_NO_ERROR);
}

inline bool util_fileExists(const char *file){
	struct stat st = {0};

	return stat(file, &st) == 0;
}

inline bool util_directoryExists(const char* dir){
	struct stat st = {0};

	return stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
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

// NOTE:(jan) If both strings are not '\0' terminated, strcat/strncat will jump/move based on unitialised values.
inline void util_append(char* a, const uint_fast64_t lengthA, char* b, const uint_fast64_t lengthB){
	strncat(a, b, lengthB - (lengthA - lengthB));
}

inline int util_hash(uint8_t* s, uint_fast64_t length){
	int hash = 0;

	int c;
	while (length-- != 0){
		c = *s++;

		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

// sdbm - hash implementation. see(http://www.cse.yorku.ca/~oz/hash.html)
inline int util_hashString(const char* s, uint_fast64_t length){
	return util_hash((uint8_t*) s, length);
}

// Note: 'directory' has to be slash terminated. (jan - 2019.05.20)
ERROR_CODE util_deleteDirectory(const char* directory, const bool preserveRoot, const bool emptyDirectoriesOnly){
	ERROR_CODE error = ERROR_NO_ERROR;

	DIR* currentDirectory = opendir(directory);
	if(currentDirectory == NULL){
		error = ERROR_FAILED_TO_OPEN_DIRECTORY;

		goto label_closeDir;
	}

	struct dirent* directoryEntry;

	const uint_fast64_t directoryLength = strlen(directory);

	int_fast8_t isDirectoryEmpty = 2;
	while((directoryEntry = readdir(currentDirectory)) != NULL){
		// Avoid reentering current and parent directory.
		const uint_fast64_t currentEntryLength = strlen(directoryEntry->d_name);
		if(strncmp(directoryEntry->d_name, ".", currentEntryLength) == 0 || strncmp(directoryEntry->d_name, "..", currentEntryLength) == 0){
			isDirectoryEmpty -= 1;

			continue;
		}

		if(directoryEntry->d_type == DT_DIR){
			isDirectoryEmpty += 1;

			const uint_fast64_t directoryPathLength = directoryLength + currentEntryLength + 1;

			char* directoryPath;
			directoryPath = alloca(sizeof(*directoryPath) * (directoryPathLength + 1));
			strncpy(directoryPath, directory, directoryLength);
			directoryPath[directoryLength] = '\0';

			util_append(directoryPath + directoryLength, directoryPathLength - 1 - directoryLength, directoryEntry->d_name, currentEntryLength);
			directoryPath[directoryPathLength - 1] = '/';
			directoryPath[directoryPathLength] = '\0';
		 
			if((error = util_deleteDirectory(directoryPath, false, emptyDirectoriesOnly)) != ERROR_NO_ERROR){
				goto label_closeDir;
			}
		}else{
			const uint_fast64_t filePathLength = directoryLength + currentEntryLength;

			char* filePath;
			filePath = alloca(sizeof(*filePath) * (filePathLength + 1));
			strncpy(filePath, directory, directoryLength);
			filePath[directoryLength] = '\0';

			util_append(filePath + directoryLength, filePathLength - directoryLength, directoryEntry->d_name, currentEntryLength);
			filePath[filePathLength] = '\0';

			if(emptyDirectoriesOnly && isDirectoryEmpty != 0){
				if((error = util_deleteFile(filePath)) != ERROR_NO_ERROR){
					goto label_closeDir;
				}
			}
		}
	}

	if(!preserveRoot && !(emptyDirectoriesOnly && isDirectoryEmpty != 0)){
		if(rmdir(directory) != 0){
			return ERROR_(ERROR_FAILED_TO_DELETE_DIRECTORY, "Dir:'%s'. [%s]", directory, strerror(errno));
		}
	}
	
label_closeDir:
	closedir(currentDirectory);
		
	return ERROR(error);
}

inline ERROR_CODE util_concatenate(char* dst, const char* a, const uint_fast64_t lengthA, const char* b, const uint_fast64_t lengthB) {
	strncpy(dst, a, lengthA);

	strncpy(dst + lengthA, b, lengthB);

	return ERROR(ERROR_NO_ERROR);
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
	buffer[6] = (value >> 8) & 0XFF;
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

// http://www.stroustrup.com/new_learning.pdf
ERROR_CODE util_readUserInput(char** s, uint_fast64_t* charRead){
	uint_fast16_t limit = 64;
	*s = malloc(limit);
	if(*s == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	// Skip leading whitespaces.
	while (true) {
		int_fast32_t c = fgetc(stdin);

		if(c == EOF){
			break;
		}

		if(!isspace(c) || c == '\n') {
			 ungetc(c, stdin);

			 break;
		}
	}

	uint_fast64_t i = 0;
	while (true) {
		const int_fast32_t c = fgetc(stdin);
	
		if(c == '\n' || c == '\0' || c == EOF){
			(*s)[i] = '\0';

			break;
		}

		(*s)[i] = c;

		if(i == limit - 1) {
			limit *= 2;

			*s = realloc(*s, limit);
		}

		i++;
	}

	*charRead = i;

	return ERROR(ERROR_NO_ERROR);
}

inline void util_replaceAllChars(char* s, const char a, const char b){
	do{
		if(*s == a)
			*s = b;
	}while(*s++);
}

inline int_fast32_t util_getNumAvailableProcessorCores(void){
	return sysconf(_SC_NPROCESSORS_ONLN);
}

inline ERROR_CODE util_getBaseDirectory(char** baseDirectory, uint_fast64_t* baseDirectoryLength, char* url, uint_fast64_t urlLength){
	const int_fast64_t firstSeperator = util_findFirst(url, urlLength, '/');

	if(firstSeperator == -1){
		return ERROR(ERROR_INVALID_REQUEST_URL);
	}else{
		 // URL: /
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
			*baseDirectoryLength = (secondSeperator - firstSeperator) + 1/*Include the second '/'*/;

			return ERROR(ERROR_NO_ERROR);
		}
	}
}

inline void util_replace(char* buffer, const uint_fast64_t bufferLength, uint_fast64_t* srcStringLength, const char* a, const uint_fast64_t stringLengthA, const char* b, const uint_fast64_t stringLengthB){
	const bool shrink = stringLengthB <= stringLengthA;

	int_fast64_t searchOffset = 0;
	int_fast64_t writeOffset = 0;
	while((writeOffset = util_findFirst_s(buffer + searchOffset, *srcStringLength - searchOffset, a, stringLengthA)) != -1){
		if(shrink){
			// Replace string 'a' with 'b'.
			memcpy(buffer + searchOffset + writeOffset, b, stringLengthB);
			// Move everything that came behind 'a' to the end of 'b'.
			memcpy(buffer + searchOffset + writeOffset + stringLengthB, buffer + searchOffset + writeOffset + stringLengthA, *srcStringLength + 1 - writeOffset - stringLengthA);

			// Adjust src string length.
			*srcStringLength -= stringLengthA - stringLengthB;

			searchOffset += writeOffset + stringLengthB;
		}else{			
			const uint_fast64_t copyLength = *srcStringLength + 1 - searchOffset - writeOffset - stringLengthA;

			uint_fast64_t i;
			for(i = 0; i <= copyLength; i++){
				const uint_fast64_t srcIndex = searchOffset + writeOffset + (stringLengthB - stringLengthA) + copyLength - i;
				const uint_fast64_t dstIndex = searchOffset + writeOffset + stringLengthB + copyLength - i;

				buffer[dstIndex] = buffer[srcIndex];
			}

			memcpy(buffer + searchOffset + writeOffset, b, stringLengthB);

			*srcStringLength += stringLengthB - stringLengthA;

			searchOffset += writeOffset + stringLengthB;
		}
	}
}

char* util_trimTrailingWhiteSpace(char* s, uint_fast64_t* length, const bool isStringNullTerminated){
	uint_fast64_t searchOffset = *length - (isStringNullTerminated ? 1 : 0);

	// Count number of trailing white spaces.
	char character;
	do{
		searchOffset -= 1;

		character = s[searchOffset];

		if(searchOffset == 0){
			break;
		}

		if(!isspace(character)){
			searchOffset += 1;

			break;
		}

	}while(true);

	// Set new string length.
	*length -= (*length) - (searchOffset + (isStringNullTerminated ? 1 : 0));
	
	// If the base string was null terminated we can shift the '\0' character to the new end of the string to 'cut' the trailing white space of.
	if(isStringNullTerminated){
		s[(*length) - 1] = '\0';
	}

	return s;
}

char* util_trimLeadingWhiteSpace(char* s, uint_fast64_t* length, const bool isStringNullTerminated){
	// Count the number of leading white spaces.
	uint_fast64_t searchIndex;
	for(searchIndex = 0; searchIndex < (*length) - 1; searchIndex++){
		if(!isspace(s[searchIndex])){
			break;
		}
	}

	// Adjust string length by number of removed white spaces.
	*length -= searchIndex;

	// Shift string by number of white spaces to the left.
	uint_fast64_t i;
	for(i = 0; i < (*length); i++){
		s[i] = s[i + searchIndex];
	}

	return s;
}

char* util_trim(char* s, uint_fast64_t* length){
	// 
	if(*length == 0 || s[0] == '\0'){
		return s;
	}

	// To not put a trailing '\0' at the end 's' if 's' is a sub string, we check if 's' is null terminated.
	const bool isStringNullTerminated = s[(*length) - 1] == '\0';

	s = util_trimTrailingWhiteSpace(s, length, isStringNullTerminated);

	// Cover the special case where an all white space string was passed and 'util_trimTrailingWhiteSpace' has already squashed it to zero-length.
	if(*length > 0){
		s = util_trimLeadingWhiteSpace(s, length, isStringNullTerminated);
	}

	return s;
}

inline void util_printBuffer(void* buffer, uint_fast64_t length){
uint_fast64_t i;
	for(i = 0; i < length; i++){
		printf("%c", ((char*) buffer)[i]);
	}
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

inline ERROR_CODE util_stringToInt(const char* s, int64_t* value){
	char* endPtr;
	*value = strtol(s, &endPtr, 10/*Decimal*/);

	if((*value == 0 && endPtr == s) || *value == LONG_MIN || *value == LONG_MAX){
		return ERROR_(ERROR_NOT_A_NUMBER, "'%s' is not a valid number.", s);
	}

	return ERROR(ERROR_NO_ERROR);
}

// Note: Use PATH_MAX, for the size of 'dir'. (jan - 2019.01.23)
inline ERROR_CODE util_getCurrentWorkingDirectory(char* dir, const uint_fast64_t dirLength){
	if(getcwd(dir, dirLength) == NULL){
		return ERROR_(ERROR_ERROR, "%s", strerror(errno));
	}

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE util_getFileExtension(char** extension, uint_fast64_t* fileExtensionLength, char* fileName, const uint_fast64_t fileNameLength){
	const int_fast64_t fileExtensionOffset = util_findLast(fileName, fileNameLength, '.');

	if(fileExtensionOffset == -1){
		return ERROR_(ERROR_INVALID_STRING, "%s does not contain the token '.'", fileName);
	}

	*fileExtensionLength = fileNameLength - (fileExtensionOffset + 1);
	*extension = fileName + (fileExtensionOffset + 1);

	return ERROR(ERROR_NO_ERROR);
}

inline char* util_getFileName(char* filePath, const uint_fast64_t filePathLength){
	return filePath + (util_findLast(filePath, filePathLength, '/') + 1);
}

inline ERROR_CODE util_getFileDirectory(char* dir, char* filePath, const uint_fast64_t filePathLength){
	const uint_fast64_t dirLength = util_findLast(filePath, filePathLength, '/');

	if(dirLength <= 0){
		return ERROR_(ERROR_INVALID_STRING, "The path '%s' does not contain a directory.", filePath);
	}

	strncpy(dir, filePath, dirLength);
	dir[dirLength] = '\0';

	return ERROR(ERROR_NO_ERROR);
}

ERROR_CODE _util_walkDirectory(LinkedList* list, bool stepIntoChildDir, const char* directory, WalkDirectoryFilter filter){
	ERROR_CODE error = ERROR_NO_ERROR;

	DIR* currentDirectory = opendir(directory);
	if(currentDirectory == NULL){
		error = ERROR_FAILED_TO_OPEN_DIRECTORY;

		goto label_closeDir;
	}

	const uint_fast64_t directoryLength = strlen(directory);

	struct dirent* directoryEntry;
	while((directoryEntry = readdir(currentDirectory)) != NULL){
		const uint_fast64_t currentEntryLength = strlen(directoryEntry->d_name);

		// Avoid reentering current and parent directory.
		const bool selfOrParent = directoryEntry->d_name[0] == '.';
		if(selfOrParent || (selfOrParent && directoryEntry->d_name[1] == '.')){
			continue;
		}

		if(directoryEntry->d_type == DT_DIR){
			const uint_fast64_t directoryPathLength = directoryLength + currentEntryLength + 1;

			char* directoryPath;
			directoryPath = (filter == WALK_DIRECTORY_FILTER_DIRECTORIES_ONLY) ? malloc(sizeof(*directoryPath) * (directoryPathLength + 1)) : alloca(sizeof(*directoryPath) * (directoryPathLength + 1));
			strncpy(directoryPath, directory, directoryLength + 1);

			UTIL_LOG_CONSOLE_(LOG_DEBUG, "Dir: '%s'.", directory);

			util_append(directoryPath + directoryLength, directoryPathLength - 1 - directoryLength, directoryEntry->d_name, currentEntryLength);
			directoryPath[directoryPathLength - 1] = '/';
			directoryPath[directoryPathLength] = '\0';

			if(filter == WALK_DIRECTORY_FILTER_DIRECTORIES_ONLY){
				if((error = linkedList_add(list, &directoryPath, sizeof(char*))) !=ERROR_NO_ERROR){
					goto label_closeDir;
				}
			}

			if(stepIntoChildDir){
				if((error = util_walkDirectory(list, directoryPath, filter)) != ERROR_NO_ERROR){
					goto label_closeDir;
				}
			}
		}else{
			if(filter == WALK_DIRECTORY_FILTER_FILES_ONLY){
				const uint_fast64_t pathLength = directoryLength + currentEntryLength;

				char* path;
				path = malloc(sizeof(*path) * (pathLength + 1));
				strncpy(path, directory, directoryLength + 1);

				util_append(path + directoryLength, pathLength - directoryLength, directoryEntry->d_name, currentEntryLength);
				path[pathLength] = '\0';

				if((error = linkedList_add(list, &path, sizeof(char*))) !=ERROR_NO_ERROR){
					goto label_closeDir;
				}
			}
		}
	}

label_closeDir:
	closedir(currentDirectory);

	return ERROR(error);
}

ERROR_CODE util_listDirectoryContent(LinkedList* list, const char* directory, WalkDirectoryFilter filter){
	return ERROR(_util_walkDirectory(list, false, directory, filter));
}

ERROR_CODE util_walkDirectory(LinkedList* list, const char* directory, WalkDirectoryFilter filter){
	return ERROR(_util_walkDirectory(list, true, directory, filter));
}

inline ERROR_CODE util_renameFile(char* file, char* newName){
	if(rename(file, newName) != 0){
		return ERROR(ERROR_FAILED_TO_RENAME_FILE);
	}

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE util_renameFileRelative(char* dir, char* file, char* newName){
	const int fileDescriptor = open(dir, O_RDONLY | O_DIRECTORY);

	if(fileDescriptor == -1){
		return ERROR_(ERROR_FAILED_TO_OPEN_DIRECTORY, "'%s'", strerror(errno));
	}

	if(renameat(fileDescriptor, file, fileDescriptor, newName) != 0){
		return ERROR_(ERROR_FAILED_TO_RENAME_FILE, "'%s'", strerror(errno));
	}

	return ERROR(ERROR_NO_ERROR);
}

// TODO: Check if there is a kernel space utility to copy files. (jan - 2022.04.03)
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

	// TODO: Decide if putting the buffer on the stack is a good idea. (jan - 2018.11.17)
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
	struct stat fileInfo;
	lstat(file_path, &fileInfo);

	return S_ISDIR(fileInfo.st_mode);
}

inline ERROR_CODE util_getFileSize(const char* filePath, uint_fast64_t* fileSize){
	struct stat fileInfo;
	if(lstat(filePath, &fileInfo) == -1){
		return ERROR_(ERROR_FAILED_TO_RETRIEV_FILE_INFO, "File:'%s'", filePath);
	}

	*fileSize = fileInfo.st_size;

	return ERROR(ERROR_NO_ERROR);
}

#endif