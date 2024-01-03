#ifndef TEST_H
#define TEST_H

#include "util.h"
#include "arrayList.h"

#define TEST_NO_SETUP_FLAG 0x01

#define TEST_TEST_SUIT_CONSTRUCT_FUNCTION(functionName, type, varName) ERROR_CODE test_testSuitConstruct_##functionName(type** varName)
typedef ERROR_CODE test_TestSuitConstructFunction(void**);

#define TEST_TEST_SUIT_DESTRUCT_FUNCTION(testSuitName, type, varName) ERROR_CODE test_testSuitDestruct_##testSuitName (type* varName)
typedef ERROR_CODE test_TestSuitDestructFunction(void*);

#define TEST_TEST_FUNCTION(functionName) bool test_ ## functionName (void* data)
#define TEST_TEST_FUNCTION_(functionName, type, varName) bool test_ ## functionName (type* varName)
typedef TEST_TEST_FUNCTION(testFunction);

#define TEST_END() return test_testEnd()

#define TEST_BEGIN() test_testBegin()

#define TEST_SUIT_BEGIN_(name) test_testSuitBegin_(# name, (ERROR_CODE (*)(void**)) test_testSuitConstruct_## name, (ERROR_CODE (*)(void*)) test_testSuitDestruct_## name)

#define TEST_SUIT_BEGIN(name) test_testSuitBegin(name)

#define TEST_SUIT_END() test_testSuitEnd()

#define TEST(name) test_test((test_testFunction*)test_ ## name, #name)

#define __TEST_NO_SETUP__(void) do { \
	TestSuit* testSuit = ARRAY_LIST_GET_PTR(&testSuits, ARRAY_LIST_LENGTH((&testSuits)) - 1, TestSuit); \
	testSuit->noSetup = TEST_NO_SETUP_FLAG; \
}while(0)

#define TEST_NO_SETUP(name) __TEST_NO_SETUP__(); \
	TEST(name)

#define TEST_SUCCESS true

#define TEST_FAILURE(format, ...) UTIL_LOG_ERROR_(format, __VA_ARGS__), false

#define TEST_SYSLOG_IDENTIFIER "herder_test"

static ArrayList testSuits;

typedef struct testSuit{
	test_TestSuitConstructFunction* constructFunction;
	test_TestSuitDestructFunction* destructFunction;
	void* data;
	char* name;
	UTIL_FLAG(noSetup, 1);
	ArrayList testedFunctions;
}TestSuit;

typedef struct{
	char* name;
	bool failed;
}Test;

void test_testBegin(void);

int32_t test_testEnd(void);

TestSuit* test_testSuitBegin(const char*);

void test_testSuitBegin_(const char*, test_TestSuitConstructFunction*, test_TestSuitDestructFunction);

void test_testSuitEnd(void);

void test_test(test_testFunction* , const char*);

#endif
