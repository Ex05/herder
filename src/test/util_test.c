#ifndef UTIL_TEST_C
#define UTIL_TEST_C

#include "../test.c"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslog.h>

TEST_TEST_FUNCTION(util_findFirst){
	const char s[] = "abcdef012ghi~jklno~p345~qrs";

	if(util_findFirst(s, strlen(s), '~') != 12){
		return TEST_FAILURE("Failed to find fisrt char:'~' in string:'%s'.", s);
	}

	if(util_findFirst(s, strlen(s), '/') != -1){
		return TEST_FAILURE("Failed to not find char:'/' in string:'%s'.", s);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_findFirst_s){
	{
		const char a[] = "aabbccdd--11223344";

		if(util_findFirst_s(a, strlen(a), "--", 2) != 8){
			return TEST_FAILURE("Failed to find fisrt string: '--' in string:'%s'.", a);
		}
	}

	{
		const uint_fast64_t stringLength = strlen("aabbccdd--11223344");
		char* s = alloca(stringLength+ 1);
		strncpy(s, "aabbccdd--11223344", stringLength + 1);

		if(util_findFirst_s(s, strlen(s), "444", 3) != -1){
			return TEST_FAILURE("Failed to find fisrt string: '444' in string:'%s'.", s);
		}
	}

	{
		char s[] = "----";
		char* a = alloca(64);
		
		uint_fast64_t stringLength = strlen(s);
		
		memcpy(a, s, stringLength + 1);

		if((util_findFirst_s(a, stringLength, "$errorCode", strlen("$errorCode")) != -1)){
			return TEST_FAILURE("Failed to find fisrt string: '$errorCode' in string:'%s'.", s);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_findLast){
	const char a[] = "01234...567";
	const char b[] = "01234567";

	if(util_findLast(a, strlen(a), '.') != 7){
		return TEST_FAILURE("Failed to find last char:'.' in string:'%s'.", a);
	}

	int_fast64_t ret;
	if((ret = util_findLast(b, strlen(b), '.')) != -1){
		return TEST_FAILURE("Return value '%" PRIdFAST64 " != -1'.", ret);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_stringStartsWith){
	char a[] = "01234...567";

	if(util_stringStartsWith(a, '0') != true){
		return TEST_FAILURE("Failed to identify starting character of string:'%s'.", a);
	}

	if(util_stringStartsWith(a, '1') != false){
		return TEST_FAILURE("Failed to identify starting character of string:'%s'.", a);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_stringEndsWith){
	char a[] = "01234...567";
	const uint_fast64_t lengthA = strlen(a);

	if(util_stringEndsWith(a,lengthA, '7') != true){
		return TEST_FAILURE("Failed to identify starting character of string:'%s'.", a);
	}

	if(util_stringEndsWith(a, lengthA, '0') != false){
		return TEST_FAILURE("Failed to identify starting character of string:'%s'.", a);
	}

	return TEST_SUCCESS;
}


TEST_TEST_FUNCTION(util_stringStartsWith_s){
	char a[] = "01234...567";

	if(util_stringStartsWith_s(a, "01", 2) != true){
		return TEST_FAILURE("Failed to identify starting character of string:'%s'.", a);
	}

	if(util_stringStartsWith_s(a, "02", 2) != false){
		return TEST_FAILURE("Failed to identify starting character of string:'%s'.", a);
	}

	if(util_stringStartsWith_s(a, "01234...56789", 13) != false){
		return TEST_FAILURE("Failed to identify starting character of string:'%s'.", a);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_getBaseDirectory){
	ERROR_CODE error;

	// Test_1.
	{
		char url[] = "/";

		char* baseDirectory;
		uint_fast64_t baseDirectoryLength;
		if((error = util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url))) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to get base directory of path: '%s'. '%s'.", url, util_toErrorString(error));
		}

		if(baseDirectoryLength != 1){
			return TEST_FAILURE("Base directory length '%" PRIdFAST64 "' != '%d'", baseDirectoryLength, 1);
		}

		if(strncmp(url, baseDirectory, baseDirectoryLength) != 0){
			return TEST_FAILURE("'util_getBaseDirectory' '%s' != '%s'.", url, baseDirectory);
		}
	}

	// Test_2.
	{
		char url[] = "/extractShowInfo";

		char* baseDirectory;
		uint_fast64_t baseDirectoryLength;
		if((error = util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url))) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to get base directory of path: '%s'. '%s'.", url, util_toErrorString(error));
		}

		if(baseDirectoryLength != 16){
			return TEST_FAILURE("Base directory length '%" PRIdFAST64 "' != '%d'", baseDirectoryLength, 16);
		}

		if(strncmp(url, baseDirectory, baseDirectoryLength) != 0){
			return TEST_FAILURE("'util_getBaseDirectory' '%s' != '%s'.", baseDirectory, "/img");
		}
	}

	// Test_3.
	{
		char url[] = "/git.html";

		char* baseDirectory;
		uint_fast64_t baseDirectoryLength;
		if((error = util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url))) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to get base directory of path: '%s'. '%s'.", url, util_toErrorString(error));
		}

		if(baseDirectoryLength != 1){
			return TEST_FAILURE("Base directory length '%" PRIdFAST64 "' != '%d'", baseDirectoryLength, 1);
		}

		if(strncmp("/", baseDirectory, baseDirectoryLength) != 0){
			return TEST_FAILURE("'util_getBaseDirectory' '%s' != '%s'.", baseDirectory, "/");
		}
	}

	// Test_4.
	{
		char url[] = "/img/img_001.png";

		char* baseDirectory;
		uint_fast64_t baseDirectoryLength;
		if((error = util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url))) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to get base directory of path: '%s'. '%s'.", url, util_toErrorString(error));
		}

		if(baseDirectoryLength != 4){
			return TEST_FAILURE("Base directory length '%" PRIdFAST64 "' != '%d'", baseDirectoryLength, 4);
		}

		if(strncmp("/img", baseDirectory, baseDirectoryLength) != 0){
			return TEST_FAILURE("'util_getBaseDirectory' '%s' != '%s'.", baseDirectory, "/img");
		}
	}

	// Test_5.
	{
		char url[] = "check.proxyradar.com:80";

		char* baseDirectory;
		uint_fast64_t baseDirectoryLength;
		__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_INVALID_REQUEST_URL);
		if((error = util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url))) != ERROR_INVALID_REQUEST_URL){
			return TEST_FAILURE("Failed to throw an error for path: '%s'. Expected '%s' buit functiion returned '%s'.", url, util_toErrorString(ERROR_INVALID_REQUEST_URL), util_toErrorString(error));
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_byteArrayTo_uint16){
	const uint16_t testData[] = {
		0,
		1,
		UINT8_MAX,
		UINT16_MAX
	};

	int8_t buf[sizeof(uint16_t)];

	uint_fast64_t i;
	for(i = 0; i < sizeof(testData) / sizeof(testData[0]); i++){
		const uint16_t val = testData[i];

	#if __BYTE_ORDER == __LITTLE_ENDIAN
		buf[1] = (val >> (8 * 0)) & 0xFF;
		buf[0] = (val >> (8 * 1)) & 0xFF;
	#elif __BYTE_ORDER == __BIG_ENDIAN
		buf[0] = (val >> (8 * 0)) & 0xFF;
		buf[1] = (val >> (8 * 1)) & 0xFF;
	#elif __BYTE_ORDER == __PDP_ENDIAN
		ERROR: PDP/mixed endianes is not supported at the moment.
	#else
		ERROR: Your system architectures endianes is not supported at the moment.
	#endif

		uint16_t ret;
		if((ret = util_byteArrayTo_uint16(buf)) != val){
			return TEST_FAILURE("Return value '%" PRIu16 " != '%" PRIu16 "'.", ret, val);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_byteArrayTo_uint32){
	const uint32_t testData[] = {
		0,
		1,
		UINT8_MAX,
		UINT16_MAX,
		UINT32_MAX
	};

	int8_t buf[sizeof(uint32_t)];

	uint_fast64_t i;
	for(i = 0; i < sizeof(testData) / sizeof(testData[0]); i++){
		const uint32_t val = testData[i];

	#if __BYTE_ORDER == __LITTLE_ENDIAN
		buf[3] = (val >> (8 * 0)) & 0xFF;
		buf[2] = (val >> (8 * 1)) & 0xFF;
		buf[1] = (val >> (8 * 2)) & 0xFF;
		buf[0] = (val >> (8 * 3)) & 0xFF;
	#elif __BYTE_ORDER == __BIG_ENDIAN
		buf[0] = (val >> (8 * 0)) & 0xFF;
		buf[1] = (val >> (8 * 1)) & 0xFF;
		buf[2] = (val >> (8 * 2)) & 0xFF;
		buf[3] = (val >> (8 * 3)) & 0xFF;
	#elif __BYTE_ORDER == __PDP_ENDIAN
		ERROR: PDP/mixed endianes is not supported at the moment.
	#else
		ERROR: Your system architectures endianes is not supported at the moment.
	#endif

		uint32_t ret;
		if((ret = util_byteArrayTo_uint32(buf)) != val){
			return TEST_FAILURE("Return value '%" PRIu32 " != '%" PRIu32 "'.", ret, val);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_byteArrayTo_uint64){
	const uint64_t testData[] = {
		0,
		1,
		UINT8_MAX,
		UINT16_MAX,
		UINT32_MAX,
		UINT64_MAX
	};

	int8_t buf[sizeof(uint64_t)];

	uint_fast64_t i;
	for(i = 0; i < sizeof(testData) / sizeof(testData[0]); i++){
		const uint64_t val = testData[i];

	#if __BYTE_ORDER == __LITTLE_ENDIAN
		buf[7] = (val >> (8 * 0)) & 0xFF;
		buf[6] = (val >> (8 * 1)) & 0xFF;
		buf[5] = (val >> (8 * 2)) & 0xFF;
		buf[4] = (val >> (8 * 3)) & 0xFF;
		buf[3] = (val >> (8 * 4)) & 0xFF;
		buf[2] = (val >> (8 * 5)) & 0xFF;
		buf[1] = (val >> (8 * 6)) & 0xFF;
		buf[0] = (val >> (8 * 7)) & 0xFF;
	#elif __BYTE_ORDER == __BIG_ENDIAN
		buf[0] = (val >> (8 * 0)) & 0xFF;
		buf[1] = (val >> (8 * 1)) & 0xFF;
		buf[2] = (val >> (8 * 2)) & 0xFF;
		buf[3] = (val >> (8 * 3)) & 0xFF;
		buf[4] = (val >> (8 * 4)) & 0xFF;
		buf[5] = (val >> (8 * 5)) & 0xFF;
		buf[6] = (val >> (8 * 6)) & 0xFF;
		buf[7] = (val >> (8 * 7)) & 0xFF;
	#elif __BYTE_ORDER == __PDP_ENDIAN
		ERROR: PDP/mixed endianes is not supported at the moment.
	#else
		ERROR: Your system architectures endianes is not supported at the moment.
	#endif

		uint64_t ret;
		if((ret = util_byteArrayTo_uint64(buf)) != val){
			return TEST_FAILURE("Return value '%" PRIu64 " != '%" PRIu64 "'.", ret, val);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_uint16ToByteArray){
	const uint16_t testData[] = {
		0,
		1,
		UINT8_MAX,
		UINT16_MAX / 2,
		UINT16_MAX
	};

	int8_t buf[sizeof(uint16_t)];

	uint_fast64_t i;
	for(i = 0; i < sizeof(testData) / sizeof(testData[0]); i++){
		const uint16_t val = testData[i];
	
		uint16_t ret;
		if((ret = util_byteArrayTo_uint16(util_uint16ToByteArray(buf, val))) != val){
			return TEST_FAILURE("Return value '%" PRIu16 " != '%" PRIu16 "'.", ret, val);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_uint32ToByteArray){
	const uint32_t testData[] = {
		0,
		1,
		UINT8_MAX,
		UINT16_MAX,
		UINT32_MAX
	};

	int8_t buf[sizeof(uint32_t)];

	uint_fast64_t i;
	for(i = 0; i < sizeof(testData) / sizeof(testData[0]); i++){
		const uint32_t val = testData[i];

		uint32_t ret;
		if((ret = util_byteArrayTo_uint32(util_uint32ToByteArray(buf, val))) != val){
			return TEST_FAILURE("Return value '%" PRIu32 " != '%" PRIu32 "'.", ret, val);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_uint64ToByteArray){
	const uint64_t testData[] = {
		0,
		1,
		UINT8_MAX,
		UINT16_MAX,
		UINT32_MAX,
		UINT64_MAX
	};

	int8_t buf[sizeof(uint64_t)];

	uint_fast64_t i;
	for(i = 0; i < sizeof(testData) / sizeof(testData[0]); i++){
		const uint64_t val = testData[i];
	
		uint64_t ret;
		if((ret = util_byteArrayTo_uint64(util_uint64ToByteArray(buf, val))) != val){
			return TEST_FAILURE("Return value '%" PRIu64 " != '%" PRIu64 "'.", ret, val);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_replace){
	{
		char a[] = "--$errorCode--$errorCode--";
		char* s = alloca(64);
		
		uint_fast64_t stringLength = strlen(a);
		
		memcpy(s, a, stringLength + 1);

		util_replace(s, 64, &stringLength, "$errorCode", strlen("$errorCode"), "404", strlen("404"));

		if(strcmp(s, "--404--404--") != 0){
			return TEST_FAILURE("Return value '%s' != '--404--404--'.", s);
		}
	}

	{
		char a[] = "abbcdbb";
		char* s = alloca(64);
		
		uint_fast64_t stringLength = strlen(a);
		
		memcpy(s, a, stringLength + 1);

		util_replace(s, 64, &stringLength, "bb", strlen("bb"), "4444", strlen("4444"));

		if(strcmp(s, "a4444cd4444") != 0){
			return TEST_FAILURE("Return value '%s' != 'a4444cd4444'.", s);
		}

		util_replace(s, 64, &stringLength, "4444", strlen("4444"), "c", strlen("c"));

		if(strcmp(s, "accdc") != 0){
			return TEST_FAILURE("Return value '%s' != 'acdc'.", s);
		}
	}
	
	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_trim){
	char* testStrings[] = {
		"      ",
		"",
		"    9    ",
		" 123_457 8910 ",
		" 123_457 8910",
		"123_457 8910 ",
		"   123_457 8910     ",
	};

	char* expectedResults[] = {
		"",
	 	"",
		"9",
		"123_457 8910",
		"123_457 8910",
		"123_457 8910",
		"123_457 8910",
	};

	uint_fast64_t i;
	for(i = 0; i < UTIL_ARRAY_LENGTH(testStrings); i++){
		uint_fast64_t testStringLength = strlen(testStrings[i]);

		char* testString = alloca(sizeof(*testString) * testStringLength + 1);
		memcpy(testString, testStrings[i], testStringLength + 1);

		testStringLength += 1;

		testString = util_trim(testString, &testStringLength);

		if(strncmp(testString, expectedResults[i], strlen(expectedResults[i]) + 1) != 0){
			return TEST_FAILURE("'util_trim' '%s' != '%s'.", testString, expectedResults[i]);
		}

		if(strlen(expectedResults[i]) + 1 != testStringLength){
			return TEST_FAILURE("'util_trim' '%" PRIuFAST64 "' != '%" PRIuFAST64 "'.", testStringLength, strlen(expectedResults[i]));
		}
	}

	for(i = 0; i < UTIL_ARRAY_LENGTH(testStrings); i++){
		uint_fast64_t testStringLength = strlen(testStrings[i]);

		char* testString = alloca(sizeof(*testString) * testStringLength + 1);
		memcpy(testString, testStrings[i], testStringLength + 1);

		testString = util_trim(testString, &testStringLength);

		char* trimedString = alloca(sizeof(*testString) * testStringLength + 1);
		memcpy(trimedString, testString, testStringLength);
		trimedString[testStringLength] = '\0';

		if(strncmp(trimedString, expectedResults[i], strlen(expectedResults[i]) + 1) != 0){
			return TEST_FAILURE("'util_trim' '%s' != '%s'.", trimedString, expectedResults[i]);
		}

		if(strlen(expectedResults[i]) != testStringLength){
			return TEST_FAILURE("'util_trim' '%" PRIuFAST64 "' != '%" PRIuFAST64 "'.", testStringLength, strlen(expectedResults[i]));
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_getFileName){
	char filePath[] = "~/Video/video_001.mkv";

	const char* fileName = util_getFileName(filePath, strlen(filePath));

	if(strcmp(fileName, "video_001.mkv") != 0){
		return TEST_FAILURE("'util_getFileName' '%s' != '%s'.", fileName, "video_001.mkv");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_formatNumber){
	char s[UTIL_FORMATTED_NUMBER_LENGTH];

	char* strings[] = {
		"000",
		"017",
		"199",
		"1_856",
		"1_111_111",
		"99_999_999_999"
	};

	uint_fast64_t numbers[] = {
		0,
		17,
		199,
		1856,
		1111111,
		99999999999
	};

	uint_fast64_t i;
	for(i = 0; i < (sizeof(numbers) / sizeof(*numbers)); i++){
		uint_fast64_t stringLength = UTIL_FORMATTED_NUMBER_LENGTH;
		util_formatNumber(s, &stringLength, numbers[i]);

		if(strncmp(s, strings[i], strlen(strings[i])) != 0){
			return TEST_FAILURE("'util_formatNumber' '%s' != '%s'.", s, strings[i]);
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_toLowerChase){
	char s[] = "AbCDEFg0123";
	
	util_toLowerChase(s);

	if(strncmp(s, "abcdefg0123", strlen(s)) != 0){
		return TEST_FAILURE("'util_toLowerChase' '%s' != '%s'.", s, "abcdefg0123");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_replaceAllChars){
	char a[] = "010101110101";

	util_replaceAllChars(a, '1', '0');

	const char b[] = "000000000000";

	if(strncmp(a, b, strlen(b)) != 0){
		return TEST_FAILURE("'util_replaceAllChars' '%s' != '%s'.", a, b);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_append){
	char a[11] = {'1', '2', '3', '4', '5'};
	char b[] = "67890";

	util_append(a, strlen(a), b, strlen(b));

	if(strncmp(a, "1234567890", 11) != 0){
		return TEST_FAILURE("'util_append' '%s' != '%s'.", a, "1234567890");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_stringToInt){
	ERROR_CODE error;

	const char a[] = "27";
	const char b[] = "a88b";

	int_fast64_t value;
	if((error = util_stringToInt(a, &value)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to convert value: '%s' to integer. '%s'.", a, util_toErrorString(error));
	}

	if(value != 27){
		return TEST_FAILURE("Value '%" PRIdFAST64 "' != '%d'.", value, 27);
	}

	 __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_INVALID_STRING);
	if((error = util_stringToInt(b, &value)) != ERROR_INVALID_STRING){
		return TEST_FAILURE("'util_stringToInt' did not fail as expected with: '%s'. '%s'.","ERROR_INVALID_STRING", util_toErrorString(error));
	}

	if(value != 0){
		return TEST_FAILURE("Value '%" PRIdFAST64 "' != '%d'.", value, 0);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_getFileExtension){
	ERROR_CODE error;

	char a[] = "The_Big_Bang_Theory_s10e05_the_hot_tub_contamination.mkv";
	char b[] = "The_Big_Bang_Theory_s10e05_the_hot_tub_contamination";

	char* fileExtension;
	uint_fast64_t fileExtensionLength;

	if((error = util_getFileExtension(&fileExtension, &fileExtensionLength, a, strlen(a))) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to extract file extension from file: '%s'. '%s'.", a, util_toErrorString(error));
	}

	if(strncmp(fileExtension, "mkv", 3) != 0 && fileExtensionLength != 3){
		return TEST_FAILURE("File extension '%s' != '%s'.", fileExtension, "mkv");
	}

	__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_INVALID_STRING);
	if((error = util_getFileExtension(&fileExtension, &fileExtensionLength, b, strlen(b))) == ERROR_NO_ERROR){
		return TEST_FAILURE("'util_getFileExtension' did not fail as expected with 'ERROR_INVALID_STRING' function returned '%s'", util_toErrorString(error));
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_fileExists){
	#define TEST_FILE_NAME "/tmp/herder_util_test_fileExists_XXXXXX"
	#define TEST_FILE_NEW_NAME "/tmp/herder_util_test_fileExists.tmp"

	char* filePath = alloca(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
	strcpy(filePath, TEST_FILE_NAME);

	char* newFileName = alloca(sizeof(*newFileName) * (strlen(TEST_FILE_NEW_NAME) + 1));
	strcpy(newFileName, TEST_FILE_NEW_NAME);

	#undef TEST_FILE_NAME
	#undef TEST_FILE_NEW_NAME

	const int fileDescriptor = mkstemp(filePath);
	if(fileDescriptor < 1){
		return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
	}

	close(fileDescriptor);

	ERROR_CODE error;
	if((error = util_renameFile(filePath, newFileName)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to rename file: '%s'. '%s'.", filePath, util_toErrorString(error));
	}

	// Test_0.
	{
		if(!util_fileExists(newFileName)){
			return TEST_FAILURE("Failed to confirm file:'%s' exists.", filePath);
		} 
	}

	// Test_1.
	{
		if(util_fileExists(filePath)){
			return TEST_FAILURE("Failed to confirm file:'%s' exists.", filePath);
		} 
	}

	if(util_deleteFile(newFileName) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete file:'%s'.", newFileName);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_blockAlloc){
	ERROR_CODE error;

	void* buffer;
	if((error = util_blockAlloc(&buffer, 2048)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to allocate buffer. '%s'.", util_toErrorString(error));
	}

	if((error = util_unMap(buffer, 2048)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to free buffer. '%s'.", util_toErrorString(error));
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_concatenate){
	char a[] = "01234";
	char b[] = "56789";

	char c[11];

	ERROR_CODE error;
	if((error = util_concatenate(c, a, strlen(a), b, strlen(b) + 1)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to concatenate strings. '%s'.", util_toErrorString(error));
	}

	if(strncmp(c, "0123456789", 11) != 0){
		return TEST_FAILURE("Failed to concatenate strings. '%s' != '0123456789'.", c);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_stringCopy){
	char a[] = "012345";

	char b[7] = {0};

	util_stringCopy(a, b, strlen(a) + 1);

	if(strncmp(a, b, strlen(a) + 1) != 0){
		return TEST_FAILURE("Failed to copy string. '%s' != '012345'.", b);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_hash){
	char a[] = "0123456789";
	char b[] = "0132456780";

	const int32_t hash = util_hash((uint8_t*) a, sizeof(*a) * strlen(a));

	if(hash != util_hash((uint8_t*) a, sizeof(*a) * strlen(a))){
		return TEST_FAILURE("Failed to hash '%s'.", a);
	}

	if(hash == util_hash((uint8_t*) b, sizeof(*b) * strlen(b))){
		return TEST_FAILURE("Failed to hash collision for '%s' and '%s'.", a, b);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_renameFile){
	ERROR_CODE error;

	#define TEST_FILE_NAME "/tmp/herder_util_test_renameFile_XXXXXX"
	#define TEST_FILE_NEW_NAME "/tmp/herder_util_test_renameFile.xmpe"

	char* filePath = alloca(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
	strcpy(filePath, TEST_FILE_NAME);

	char* newFileName = alloca(sizeof(*newFileName) * (strlen(TEST_FILE_NEW_NAME) + 1));
	strcpy(newFileName, TEST_FILE_NEW_NAME);

	#undef TEST_FILE_NAME
	#undef TEST_FILE_NEW_NAME

	const int fileDescriptor = mkstemp(filePath);
	if(fileDescriptor < 1){
		return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
	}

	if((error = util_renameFile(filePath, newFileName)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to rename file: '%s'. '%s'.", filePath, util_toErrorString(error));
	}

	if(util_fileExists(filePath)){
		return TEST_FAILURE("Failed to rename file to:'%s'.", filePath);
	} 

	if(!util_fileExists(newFileName)){
		return TEST_FAILURE("Failed to rename file to:'%s'.", newFileName);
	}

	if(util_deleteFile(newFileName) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete file:'%s'.", newFileName);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_renameFileRelative){
	ERROR_CODE error;

	#define TEST_DIR_NAME "/tmp/"
	#define TEST_FILE_NAME "herder_util_test_renameFileRelative_XXXXXX"
	#define TEST_NEW_FILE_NAME "herder_util_test_renameFileRelative.xmpe"

	char* dir = malloc(sizeof(*dir) * (strlen(TEST_DIR_NAME) + 1));
	strcpy(dir, TEST_DIR_NAME);

	char* fileName = malloc(sizeof(*fileName) * (strlen(TEST_FILE_NAME) + 1));
	strcpy(fileName, TEST_FILE_NAME);

	char* filePath = malloc(sizeof(*filePath) * (strlen(TEST_DIR_NAME) + strlen(TEST_FILE_NAME) + 1));
	strcpy(filePath, TEST_DIR_NAME);
	strcpy(filePath + strlen(TEST_DIR_NAME), TEST_FILE_NAME);

	char* newFileName = malloc(sizeof(*newFileName) * (strlen(TEST_NEW_FILE_NAME) + 1));
	strcpy(newFileName, TEST_NEW_FILE_NAME);

	char* newFilePath = malloc(sizeof(*newFilePath) * (strlen(TEST_DIR_NAME) + strlen(TEST_FILE_NAME) + 1));
	strcpy(newFilePath, TEST_DIR_NAME);
	strcpy(newFilePath + strlen(TEST_DIR_NAME), TEST_NEW_FILE_NAME);

	#undef TEST_DIR_NAME
	#undef TEST_FILE_NAME
	#undef TEST_NEW_FILE_NAME

	const int fileDescriptor = open(filePath, O_RDONLY | O_CREAT);

	if(fileDescriptor < 1){
		return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
	}

	if((error = util_renameFileRelative(dir, fileName, newFileName)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to rename file:'%s'. '%s'.", fileName, util_toErrorString(error));
	}

	if(util_fileExists(fileName)){
		return TEST_FAILURE("Failed to rename file:'%s'.", fileName);
	} 
	
	if(!util_fileExists(newFilePath)){
		return TEST_FAILURE("Failed to rename file to:'%s'.", newFilePath);
	}

	if(util_deleteFile(newFilePath) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete file:'%s'.", newFilePath);
	}

	free(dir);
	free(fileName);
	free(filePath);
	free(newFileName);
	free(newFilePath);

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_getFileDirectory){
	ERROR_CODE error;

	char path[] = "/home/user/file.xmp";

	char* dir = alloca(sizeof(*dir) * (strlen(path) + 1));

	if((error = util_getFileDirectory(dir, path, strlen(path))) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to extract files directory from path: '%s'. '%s'.", path, util_toErrorString(error));
	}

	if(strcmp(dir, "/home/user") != 0){
		return TEST_FAILURE("File directory '%s' != '%s'.", dir, "/home/user");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_fileCopy){
	#define TEST_FILE_NAME "/tmp/herder_util_test_fileExists_XXXXXX"
	#define TEST_FILE_NEW_NAME "/tmp/herder_util_test_fileExists.tmp"

	char* filePath = alloca(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
	strcpy(filePath, TEST_FILE_NAME);

	char* newFileName = alloca(sizeof(*newFileName) * (strlen(TEST_FILE_NEW_NAME) + 1));
	strcpy(newFileName, TEST_FILE_NEW_NAME);

	const int fileDescriptor = mkstemp(filePath);
	if(fileDescriptor < 1){
		return TEST_FAILURE("Failed to create temporary file:'%s' [%s].", filePath, strerror(errno));
	}

	const int_fast64_t bufferLength = strlen(TEST_FILE_NAME) + 1;
	char* buffer = alloca(sizeof(*buffer) * bufferLength);
	strncpy(buffer, TEST_FILE_NAME, bufferLength);

	if(write(fileDescriptor, buffer, sizeof(char) * bufferLength) != bufferLength){
		return TEST_FAILURE("Failed to write to temporary file:'%s'.", filePath);
	}

	close(fileDescriptor);

	ERROR_CODE error;
	if((error = util_fileCopy(filePath, newFileName)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to copy file from '%s' to '%s'.", filePath, newFileName);
	}

	if(!util_fileExists(newFileName)){
		return TEST_FAILURE("Failed to copy file file:'%s' does not exist.", newFileName);
	} 

	int fd = open(newFileName, O_RDONLY);

	if(fd == 0){
		return TEST_FAILURE("Failed to open file:'%s'.", newFileName);
	}

	memset(buffer, 0, bufferLength);

	if(read(fd, buffer, sizeof(char) * bufferLength) != (ssize_t) (sizeof(char) * bufferLength)){
		return TEST_FAILURE("Failed to read from file:'%s'.", newFileName);
	}

	close(fd);

	if(strncmp(buffer, TEST_FILE_NAME, bufferLength) != 0){
		return TEST_FAILURE("'%s' != '%s'.", buffer, TEST_FILE_NAME);
	}

	if(util_deleteFile(filePath) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete file:'%s'.", filePath);
	}

	if(util_deleteFile(newFileName) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete file:'%s'.", newFileName);
	}

	#undef TEST_FILE_NAME

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_deleteFile){
	#define TEST_FILE_NAME "/tmp/herder_util_test_deleteFile_XXXXXX"

	char* filePath = alloca(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
	strcpy(filePath, TEST_FILE_NAME);
		
	#undef TEST_FILE_NAME

	const int fileDescriptor = mkstemp(filePath);
	if(fileDescriptor < 1){
		return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
	}

	close(fileDescriptor);

	ERROR_CODE error;
	if((error = util_deleteFile(filePath)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete file:'%s'. '%s'", filePath, util_toErrorString(error));
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_deleteDirectory){
	// Test_0.
	{
		#define TMP_DIRECTORY_NAME "/tmp/herder_util_test_deleteDirectory_0_XXXXXX"

		char* tmpPath = alloca(sizeof(*tmpPath) * (strlen(TMP_DIRECTORY_NAME) + 1));
		strcpy(tmpPath, TMP_DIRECTORY_NAME);

		#undef TMP_DIRECTORY_NAME

		const char* dir = mkdtemp(tmpPath);

		if(dir == NULL){
			return TEST_FAILURE("Failed to create unique directory name. '%s'", strerror(errno));
		}

		ERROR_CODE error;
		if((error = util_deleteDirectory(dir, false, false)) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to delete directory '%s' [%s].", dir, util_toErrorString(error));
		}

		if(util_directoryExists(dir)){
			return TEST_FAILURE("Failed to delete directory:'%s'.", dir);
		}
	}

	// Test_1.
	{
		#define TMP_DIRECTORY_NAME "/tmp/herder_util_test_deleteDirectory_1_XXXXXX"
	
		char* tmpPath = alloca(sizeof(*tmpPath) * (strlen(TMP_DIRECTORY_NAME) + 1));
		strcpy(tmpPath, TMP_DIRECTORY_NAME);

		#undef TMP_DIRECTORY_NAME

		const char* dir = mkdtemp(tmpPath);

		if(dir == NULL){
			return TEST_FAILURE("Failed to create unique directory name. '%s'", strerror(errno));
		}

		ERROR_CODE error;
		if((error = util_deleteDirectory(dir, true, false)) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to delete directory '%s' [%s].", dir, util_toErrorString(error));
		}

		if(!util_directoryExists(dir)){
			return TEST_FAILURE("Failed to delete directory:'%s'.", dir);
		}

		if((error = util_deleteDirectory(dir, false, false)) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to delete directory '%s' [%s].", dir, util_toErrorString(error));
		}

		// TODO: Add directory into tmp dir.
	}

	// Test_2.
	{
		#define TMP_DIRECTORY_NAME "/tmp/herder_util_test_deleteDirectory_2_XXXXXX"

		char* tmpPath = alloca(sizeof(*tmpPath) * (strlen(TMP_DIRECTORY_NAME) + 1));
		strcpy(tmpPath, TMP_DIRECTORY_NAME);

		#undef TMP_DIRECTORY_NAME

		const char* dir = mkdtemp(tmpPath);

		if(dir == NULL){
			return TEST_FAILURE("Failed to create unique directory name. '%s'", strerror(errno));
		}

		ERROR_CODE error;
		if((error = util_deleteDirectory(dir, false, true)) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to delete directory '%s' [%s].", dir, util_toErrorString(error));
		}

		if(util_directoryExists(dir)){
			return TEST_FAILURE("Failed to delete directory:'%s'.", dir);
		}

		// TODO: Add tmp file into tmp dir.
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_createDirectory){
	#define TMP_DIRECTORY_NAME "/tmp/herder_util_test_deleteDirectory_0_XXXXXX"

	char* tmpPath = alloca(sizeof(*tmpPath) * (strlen(TMP_DIRECTORY_NAME) + 1));
	strcpy(tmpPath, TMP_DIRECTORY_NAME);

	#undef TMP_DIRECTORY_NAME

	const char* dir = mkdtemp(tmpPath);

	if(dir == NULL){
		return TEST_FAILURE("Failed to create unique directory name. '%s'", strerror(errno));
	}

	ERROR_CODE error;
	if((error = util_deleteDirectory(dir, false, false)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete directory '%s' [%s].", dir, util_toErrorString(error));
	}

	if((error = util_createDirectory(dir)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to create directory '%s' [%s].", dir, util_toErrorString(error));
	}

	if(!util_directoryExists(dir)){
		return TEST_FAILURE("Failed to delete directory:'%s'.", dir);
	}

	if((error = util_deleteDirectory(dir, false, false)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete directory '%s' [%s].", dir, util_toErrorString(error));
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_createAllDirectories){
	const char dir[] = "/tmp/util_createAllDirectories/0/1/2/3/";
	const uint_fast64_t dirLength = strlen(dir);

	ERROR_CODE error;
	if((error = util_createAllDirectories(dir, dirLength)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to create directory structure '%s' [%s].", dir, util_toErrorString(error));
	}

	if(!util_directoryExists("/tmp/util_createAllDirectories/0/1/2/3/")){
		return TEST_FAILURE("Failed to delete directory:'%s'.", "/tmp/util_createAllDirectories");
	}

if((error = util_deleteDirectory("/tmp/util_createAllDirectories/", false, false)) != ERROR_NO_ERROR){
		return TEST_FAILURE("Failed to delete directory '%s' [%s].", "/tmp/util_createAllDirectories", util_toErrorString(error));
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_isDirectory){
	// Test_0.
	{
		#define TMP_DIRECTORY_NAME "/tmp/herder_util_test_isDirectory_0_XXXXXX"

		char* tmpPath = alloca(sizeof(*tmpPath) * (strlen(TMP_DIRECTORY_NAME) + 1));
		strcpy(tmpPath, TMP_DIRECTORY_NAME);

		#undef TMP_DIRECTORY_NAME

		const char* dir = mkdtemp(tmpPath);

		if(dir == NULL){
			return TEST_FAILURE("Failed to create unique directory name. '%s'", strerror(errno));
		}

		if(!util_isDirectory(dir)){
			return TEST_FAILURE("Failed to identify directory '%s'.", dir);
		}

		ERROR_CODE error;
		if((error = util_deleteDirectory(dir, false, false)) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to delete directory '%s' [%s].", dir, util_toErrorString(error));
		}
	}

	// Test_1.
	{
		#define TEST_FILE_NAME "/tmp/herder_util_test_isDirectory_1_XXXXXX"

		char* filePath = alloca(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
		strcpy(filePath, TEST_FILE_NAME);

		#undef TEST_FILE_NAME

		const int fileDescriptor = mkstemp(filePath);
		if(fileDescriptor < 1){
			return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
		}

		close(fileDescriptor);

		if(util_isDirectory(filePath)){
			return TEST_FAILURE("Failed to identify directory '%s'.", filePath);
		}

		ERROR_CODE error;
		if((error = util_deleteFile(filePath)) != ERROR_NO_ERROR){
			return TEST_FAILURE("Failed to delete file:'%s'. '%s'", filePath, util_toErrorString(error));
		}
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_intToString){
	UTIL_INT_TO_STRING_HEAP_ALLOCATED(string, 10);

	if(strncmp(string, "10", stringLength) != 0){
	 	return TEST_FAILURE("'%s' != '10'", string);
	}

	return TEST_SUCCESS;
}

#endif