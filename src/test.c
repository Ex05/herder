#ifndef TEST_C
#define TEST_C

#include "test.h"

#include "server.c"
#include "argumentParser.c"
#include "que.c"

void test_testBegin(void){
	openlog(TEST_SYSLOG_IDENTIFIER, LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER);

	arrayList_init(&testSuits, 16, sizeof(TestSuit), arrayList_defaultExpandFunction);
}

int32_t test_testEnd(void){
	uint_fast64_t totalTests = 0;
	uint_fast64_t failedTests = 0;
	uint_fast64_t successfulTests = 0;
	
	ArrayListIterator testSuitIterator;
	arrayList_initIterator(&testSuitIterator, &testSuits);

	while(ARRAY_LIST_ITERATOR_HAS_NEXT(&testSuitIterator)){
		TestSuit* testSuit = ARRAY_LIST_ITERATOR_NEXT(&testSuitIterator, TestSuit);

		ArrayListIterator testIterator;
		arrayList_initIterator(&testIterator, &testSuit->testedFunctions);

		uint_fast64_t testSuitTests = testSuit->testedFunctions.length;
		uint_fast64_t failedTestSuitTests = 0;

		while(ARRAY_LIST_ITERATOR_HAS_NEXT(&testIterator)){
			Test* test = ARRAY_LIST_ITERATOR_NEXT(&testIterator, Test);

			if(test->failed){
				failedTestSuitTests++;
				failedTests++;
			}else{
				successfulTests++;
			}

			totalTests++;
		}

		arrayList_initIterator(&testIterator, &testSuit->testedFunctions);

		if(failedTestSuitTests != 0){
			printf("\n\x1b[33mTestSuit:\x1b[0m \x1b[37m%s\x1b[0 m", testSuit->name);
			printf(" \x1b[31m[%" PRIdFAST64 "/%" PRIdFAST64"]\x1b[0m\n", testSuitTests - failedTestSuitTests, testSuitTests);
		}

		while(ARRAY_LIST_ITERATOR_HAS_NEXT(&testIterator)){
			Test* test = ARRAY_LIST_ITERATOR_NEXT(&testIterator, Test);

			if(test->failed){
				printf("\t \x1b[37m%s\x1b[0m: \x1b[31mfailed\x1b[0m\n", test->name);
			}

			free(test->name);
		}

		arrayList_free(&testSuit->testedFunctions);

		free(testSuit->name);
	}

	arrayList_free(&testSuits);

	if(successfulTests == totalTests){
		printf("\n\x1b[35mTotal %" PRIuFAST64 "/%" PRIuFAST64 ".\x1b[0m\n", successfulTests, totalTests);
	}else{
		printf("\n\x1b[31mTotal %" PRIuFAST64 "/%" PRIuFAST64 ".\x1b[0m\n", successfulTests, totalTests);
	}

	closelog();

	return failedTests == 0 ? 0 : -1;
}

void test_testSuitBegin_(const char* name, test_TestSuitConstructFunction* constructFunction, test_TestSuitDestructFunction destructFunction){
	TestSuit* testSuit = test_testSuitBegin(name);

	testSuit->constructFunction = constructFunction;
	testSuit->destructFunction = destructFunction;
}

TestSuit* test_testSuitBegin(const char* name){
	TestSuit testSuit = {0};
	
	testSuit.name = malloc(sizeof(char*) * (strlen(name) + 1));
	if(testSuit.name == NULL){
		UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(ERROR_OUT_OF_MEMORY));
	}

	strcpy(testSuit.name, name);

	arrayList_init(&testSuit.testedFunctions, 16, sizeof(Test), arrayList_defaultExpandFunction);

	ARRAY_LIST_ADD(&testSuits, testSuit, TestSuit);

	return ARRAY_LIST_GET_PTR(&testSuits, ARRAY_LIST_LENGTH(&testSuits) - 1, TestSuit);
}

// Note: Intentionally empty. (jan - 2019.03.01)
void test_testSuitEnd(void){
	// Unused.
}

void test_test(test_testFunction* func, const char* name){
	TestSuit* testSuit = ARRAY_LIST_GET_PTR(&testSuits, testSuits.length - 1, TestSuit);

	Test test = {0};
	test.name = malloc(sizeof(*name) * (strlen(name) + 1));
	if(test.name == NULL){
		UTIL_LOG_ERROR(util_toErrorString(ERROR_OUT_OF_MEMORY));
	}

	strcpy(test.name, name);

	ERROR_CODE error = ERROR_NO_ERROR;
	if(testSuit->constructFunction != NULL && testSuit->noSetup != TEST_NO_SETUP_FLAG){
		if((error = testSuit->constructFunction(&testSuit->data)) != ERROR_NO_ERROR){
			test.failed = true;
		}
	}

	if(error == ERROR_NO_ERROR){
		test.failed = !func(testSuit->data);
	}

	if(testSuit->destructFunction != NULL && testSuit->noSetup != TEST_NO_SETUP_FLAG){
		if(testSuit->destructFunction(testSuit->data) != ERROR_NO_ERROR){
			test.failed = true;
		} 
	}

	ARRAY_LIST_ADD(&testSuit->testedFunctions, test, Test);

	testSuit->noSetup = 0;
}	

#include "test/arrayList_test.c"
#include "test/linkedList_test.c"
#include "test/doublyLinkedList_test.c"
#include "test/argumentParser_test.c"
#include "test/que_test.c"
#include "test/threadPool_test.c"
#include "test/util_test.c"
#include "test/properties_test.c"
#include "test/http_test.c"
#include "test/cache_test.c"
#include "test/server_test.c"

// main
#ifndef TEST_BUILD
	int test_totalyNotMain(const int argc, const char* argv[]){
#else
	int main(const int argc, const char* argv[]){
#endif
TEST_BEGIN();
	TEST_SUIT_BEGIN("arrayList");
		TEST(arraylist_iteration);
		TEST(arraylist_fixedSizedStackList);
		TEST(arraylist_get);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN_(linkedList);
		TEST(linkedList_iteration);
		TEST(linkedList_remove);
		TEST(linkedList_contains);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN_(doublyLinkedList);
		TEST(doublyLinkedList_add);
		TEST(doublyLinkedList_iteration);
		TEST(doublyLinkedList_remove);
		TEST(doublyLinkedList_contains);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN("argumentParser");
		TEST(argumentParser_parse);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN("que");
		TEST(que_deAndEnque);
		TEST(que_clear);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN("threadPool");
		TEST(threadPool_run);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN("util");
		// Integer conversions.
		TEST(util_uint16ToByteArray);
		TEST(util_uint32ToByteArray);
		TEST(util_uint64ToByteArray);
		
		// String utils.
		TEST(util_findFirst);
		TEST(util_findFirst_s);
		TEST(util_findLast);
		TEST(util_replace);
		TEST(util_trim);
		TEST(util_toLowerChase);
		TEST(util_replaceAllChars);
		TEST(util_append);
		TEST(util_stringToInt);
		TEST(util_getFileExtension);
		TEST(util_concatenate);
		TEST(util_stringCopy);
		TEST(util_stringStartsWith);
		TEST(util_stringStartsWith_s);
		TEST(util_stringEndsWith);
		TEST(util_intToString);

		// File syste.
		TEST(util_getBaseDirectory);
		TEST(util_getFileName);
		TEST(util_renameFile);
		TEST(util_renameFileRelative);
		TEST(util_getFileDirectory);
		TEST(util_fileExists);
		TEST(util_fileCopy);
		TEST(util_deleteFile);
		TEST(util_deleteDirectory);
		TEST(util_createDirectory);
		TEST(util_createAllDirectories);
		TEST(util_isDirectory);

		// Other.
		TEST(util_hash);
		TEST(util_blockAlloc);
		TEST(util_formatNumber);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN_(properties);
		TEST(properties_parse);
		TEST(properties_get);
		TEST(properties_propertyExists);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN("http");
		TEST(http_initheaderField);
		TEST(http_addHeaderField);
		TEST(http_getHeaderField);
		TEST(http_parseHTTP_Request);
		TEST(http_parseRequestType);
		TEST(http_parseHTTP_Version);
		TEST(HTTP_contentTypeToString);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN_(cache);
		TEST(cache_add);
		TEST(cache_load);
		TEST(cache_remove);
		TEST(cache_get);
	TEST_SUIT_END();

	TEST_SUIT_BEGIN_(server);
		TEST(server_addContext);
		TEST(server_getContextHandler);
		TEST(server_translateSymbolicFileLocation);
		TEST(server_translateSymbolicFileLocationErrorPage);
	TEST_SUIT_END();

	TEST_END();
}

#endif