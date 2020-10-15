// POSIX Version ¢2008 clock_gettime()
#define _XOPEN_SOURCE 700

// For mmap flags that are not in the POSIX-Standard.
#define _GNU_SOURCE

#include "util.c"
#include "mediaLibrary.c"
#include "arrayList.c"
#include "linkedList.c"
#include "argumentParser.c"
#include "http.c"
#include "que.c"
#include "threadPool.c"
#include "cache.c"
#include "propertyFile.c"
#include "server.c"
#include "herder.c"

#define TEST_NO_SETUP_FLAG 0x01

#define TEST_TEST_SUIT_CONSTRUCT_FUNCTION(functionName, varName) ERROR_CODE test_testSuitConstruct_##functionName(void** varName)
typedef ERROR_CODE test_TestSuitConstructFunction(void**);

#define TEST_TEST_SUIT_DESTRUCT_FUNCTION(functionName, varName) ERROR_CODE test_testSuitDestruct_##functionName (void* varName)
typedef ERROR_CODE test_TestSuitDestructFunction(void*);

#define TEST_TEST_FUNCTION(functionName) bool test_ ## functionName (void* data)
#define TEST_TEST_FUNCTION_(functionName, type, varName) bool test_ ## functionName (type* varName)
typedef TEST_TEST_FUNCTION(testFunction);

#define TEST_END() return test_testEnd()

#define TEST_BEGIN() test_testBegin()

#define TEST_SUIT_BEGIN_(name) test_testSuitBegin_(# name, test_testSuitConstruct_## name, test_testSuitDestruct_## name)

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

void test_testBegin(void){
    arrayList_init(&testSuits, 16, sizeof(TestSuit), arrayList_defaultExpandFunction);
}

TestSuit* test_testSuitBegin(const char*);

int32_t test_testEnd(void){
    uint_fast64_t totalTests = 0;
    uint_fast64_t failedTests = 0;
    uint_fast64_t successfulTests = 0;
    
    ArrayListIterator testSuitIterator;
    arrayList_initIterator(&testSuitIterator, &testSuits);

    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&testSuitIterator)){
        TestSuit testSuit = ARRAY_LIST_ITERATOR_NEXT(&testSuitIterator, TestSuit);

        ArrayListIterator testIterator;
        arrayList_initIterator(&testIterator, &testSuit.testedFunctions);

        uint_fast64_t testSuitTests = testSuit.testedFunctions.length;
        uint_fast64_t failedTestSuitTests  = 0;

        while(ARRAY_LIST_ITERATOR_HAS_NEXT(&testIterator)){
            Test test = ARRAY_LIST_ITERATOR_NEXT(&testIterator, Test);

            if(test.failed){
                failedTestSuitTests++;
                failedTests++;
            }else{
                successfulTests++;
            }

            totalTests++;
        }

        arrayList_initIterator(&testIterator, &testSuit.testedFunctions);

        if(failedTestSuitTests != 0){
            printf("\nTestSuit: %s", testSuit.name);
            printf(" [%" PRIdFAST64 "/%" PRIdFAST64"]\n", testSuitTests - failedTestSuitTests, testSuitTests);
        }

        while(ARRAY_LIST_ITERATOR_HAS_NEXT(&testIterator)){
            Test test = ARRAY_LIST_ITERATOR_NEXT(&testIterator, Test);

            if(test.failed){
                printf("\t  %s: failed\n", test.name);
            }

            free(test.name);
        }

        arrayList_free(&testSuit.testedFunctions);

        free(testSuit.name);
    }

    arrayList_free(&testSuits);

    printf("\nTotal %" PRIuFAST64 "/%" PRIuFAST64 ".\n", successfulTests, totalTests);

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

void test_test(test_testFunction func, const char* name){
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

// arrayList.c
TEST_TEST_FUNCTION(arraylist_iteration){
    ArrayList list;
    arrayList_initFixedSizeList(&list, 12, sizeof(uint8_t));

    ARRAY_LIST_ADD(&list, 0, uint8_t);
    ARRAY_LIST_ADD(&list, 1, uint8_t);
    ARRAY_LIST_ADD(&list, 2, uint8_t);
    ARRAY_LIST_ADD(&list, 3, uint8_t);

    uint8_t buf[4] = {0};

    ArrayListIterator it;
    arrayList_initIterator(&it, &list);

    uint_fast64_t i = 0;
    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        buf[i++] = ARRAY_LIST_ITERATOR_NEXT(&it, uint8_t);
    }

    if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
        return TEST_FAILURE("%s", "Failed to iterate over array list.");
    }

    arrayList_free(&list);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(arraylist_fixedSizedStackList){
    ArrayList list;
    ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(uint8_t));

    ARRAY_LIST_ADD(&list, 0, uint8_t);
    ARRAY_LIST_ADD(&list, 1, uint8_t);
    ARRAY_LIST_ADD(&list, 2, uint8_t);
    ARRAY_LIST_ADD(&list, 3, uint8_t);

    uint8_t buf[4] = {0};

    ArrayListIterator it;
    arrayList_initIterator(&it, &list);

    uint_fast64_t i = 0;
    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        buf[i++] = ARRAY_LIST_ITERATOR_NEXT(&it, uint8_t);
    }

    if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
        return TEST_FAILURE("%s", "Failed to create fix sized stack list.");
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(arraylist_get){
    // Test_0.
    {
        ArrayList list;
        ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(uint8_t));

        ARRAY_LIST_ADD(&list, 0, uint8_t);
        ARRAY_LIST_ADD(&list, 1, uint8_t);
        ARRAY_LIST_ADD(&list, 2, uint8_t);
        ARRAY_LIST_ADD(&list, 3, uint8_t);

        uint8_t ret;
        if((ret = ARRAY_LIST_GET(&list, 1, uint8_t)) != 1){
            return TEST_FAILURE("Return value '%" PRIu8 " != '%" PRIu8 "'.", ret, 1);
        }
    }

    // Test_1.
    {
        struct testStruct{
            uint8_t a;
            char b;
        };

        ArrayList list;
        ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(struct testStruct));

        struct testStruct b2 = {1, 'c'};
        struct testStruct b3 = {2, 'd'};
        struct testStruct b4 = {3, 'g'};

        ARRAY_LIST_ADD(&list, b2, struct testStruct);
        ARRAY_LIST_ADD(&list, b3, struct testStruct);
        ARRAY_LIST_ADD(&list, b4, struct testStruct);

        struct testStruct* ret;
        ret = ARRAY_LIST_GET_PTR(&list, 0, struct testStruct);

        if(ret->a != 1 && ret->b != 'c'){
            return TEST_FAILURE("%s", "Failed to get arrayList value.");
        }

        ret = ARRAY_LIST_GET_PTR(&list, 2, struct testStruct);

        if(ret->a != 3 && ret->b != 'g'){
            return TEST_FAILURE("%s", "Failed to get arrayList value.");
        }
    }

    // Test_2.
    {
        struct testStruct{
            uint8_t a;
            char b;
        };

        ArrayList list;
        ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST((&list), 4, sizeof(struct testStruct));

        struct testStruct b2 = {1, 'c'};
        struct testStruct* b3 = malloc(sizeof(struct testStruct));
        b3->a = 7;
        b3->b = 'h';

        ARRAY_LIST_ADD(&list, b2, struct testStruct);
        ARRAY_LIST_ADD_PTR(&list, b3, struct testStruct);
        
        free(b3);

        struct testStruct* ret;
        ret = ARRAY_LIST_GET_PTR(&list, 0, struct testStruct);

        if(ret->a != 1 && ret->b != 'c'){
            return TEST_FAILURE("%s", "Failed to get arrayList value.");
        }

        ret = ARRAY_LIST_GET_PTR(&list, 1, struct testStruct);

        if(ret->a != 7 && ret->b != 'h'){
            return TEST_FAILURE("%s", "Failed to get arrayList value.");
        }
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(arraylist_expand){
    ArrayList list;
    arrayList_init(&list, 1, sizeof(uint8_t), arrayList_defaultExpandFunction);

    ARRAY_LIST_ADD(&list, 0, uint8_t);
    ARRAY_LIST_ADD(&list, 1, uint8_t);
    ARRAY_LIST_ADD(&list, 2, uint8_t);
    ARRAY_LIST_ADD(&list, 3, uint8_t);

    uint8_t buf[4] = {0};

    ArrayListIterator it;
    arrayList_initIterator(&it, &list);

    uint_fast64_t i = 0;
    while(ARRAY_LIST_ITERATOR_HAS_NEXT(&it)){
        buf[i++] = ARRAY_LIST_ITERATOR_NEXT(&it, uint8_t);
    }

    if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
        return TEST_FAILURE("%s", "Failed to iterate over array list.");
    }

    arrayList_free(&list);

    return TEST_SUCCESS;
}

// arguemntParser.c
TEST_TEST_FUNCTION(argumentParser_parse){
    ERROR_CODE error;

    ArgumentParser parser;
    argumentParser_init(&parser);

    #if !defined __GNUC__ && !defined __GNUG__
        // TODO:(jan); Add preprocessor macros for other compiler.
    #endif

    ARGUMENT_PARSER_ADD_ARGUMENT(Test_0, 2, "-t", "--test");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import_0, 1, "-i");
    ARGUMENT_PARSER_ADD_ARGUMENT(Rename_1, 1, "--rename");
    
    const char* argc_0[] = {"./test", "--rename", "/home/ex05", "/home/29a", "-t", "-t", "-v", "--add", "American Dad"};
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_DUPLICATE_ENTRY);
    if((error = argumentParser_parse(&parser, UTIL_ARRAY_LENGTH(argc_0), argc_0)) != ERROR_DUPLICATE_ENTRY){
        return TEST_FAILURE("'arguemntParser_parse' did not fail with '%s'. '%s'.", "ERROR_DUPLICATE_ENTRY", util_toErrorString(error));
    }

    argumentParser_free(&parser);

    argumentParser_init(&parser);

    ARGUMENT_PARSER_ADD_ARGUMENT(Test_1, 2, "-t", "--test");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import_1, 1, "-i");

    const char* argc_1[] = {"./test", "-i"};
    if(argumentParser_parse(&parser, UTIL_ARRAY_LENGTH(argc_1), argc_1) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to parse arguments. '%s'.", util_toErrorString(error));
    }

    if(argumentImport_1.numValues != 0){
        return TEST_FAILURE("Failed to parse argument: '%s' '%s'.", argumentImport_1.arguments[0], util_toErrorString(error));
    }

    argumentParser_free(&parser);

    argumentParser_init(&parser);

    ARGUMENT_PARSER_ADD_ARGUMENT(Test_2, 2, "-t", "--test");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import_2, 1, "-i");
    ARGUMENT_PARSER_ADD_ARGUMENT(Rename_0, 1, "--rename");

    const char* argc_2[] = {"./test", "-v", "-i", "test", "12", "--test", "abbab", "---Tester", "12", "--rename", "/home/123.mpv", "/home/456.lmpv"};
    if(argumentParser_parse(&parser, UTIL_ARRAY_LENGTH(argc_2), argc_2) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to parse arguments. '%s'.", util_toErrorString(error));
    }

    if(!argumentParser_contains(&parser, &argumentTest_2)){
      return TEST_FAILURE("Failed to parse argument: '%s'.", argumentTest_2.arguments[0]);
    }else{
        if(argumentTest_2.numValues != 1){
            return TEST_FAILURE("Failed to parse argument: '%s'.", argumentTest_2.arguments[0]);
        }
    }

    if(!argumentParser_contains(&parser, &argumentImport_2)){
        return TEST_FAILURE("Failed to parse argument: '%s'.", argumentImport_2.arguments[0]);
    }else{
        if(argumentImport_2.numValues != 2){
            return TEST_FAILURE("Failed to parse argument: '%s'.", argumentImport_2.arguments[0]);
        }
    }

    if(!argumentParser_contains(&parser, &argumentRename_0)){
        return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
    }else{
        if(argumentRename_0.numValues != 2){
            return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
        }else{
            if(argumentRename_0.numValues != 2){
                return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
            }

            if(strncmp(argumentRename_0.values[0], "/home/123.mpv", strlen("/home/123.mpv") + 1) != 0){
                return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
            }

            if(strncmp(argumentRename_0.values[1], "/home/456.lmpv", strlen("/home/456.lmpv") + 1) != 0){
                return TEST_FAILURE("Failed to parse argument: '%s'.", argumentRename_0.arguments[0]);
            }
        }
    }

    argumentParser_free(&parser);

    return TEST_SUCCESS;
}

// linkedList.c
TEST_TEST_SUIT_CONSTRUCT_FUNCTION(linkedList, list){
    *list = malloc(sizeof(LinkedList));
        
    if(*list == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }

    return ERROR(linkedList_init(*list));
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(linkedList, list){
    linkedList_free(list);

    free(list);

    return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_FUNCTION_(linkedList_iteration, LinkedList, list){
    uint8_t a = 0;
    uint8_t b = 1;
    uint8_t c = 2;
    uint8_t d = 3;

    linkedList_add(list, &a);
    linkedList_add(list, &b);
    linkedList_add(list, &c);
    linkedList_add(list, &d);

    uint8_t buf[4] = {0};

    LinkedListIterator it;

    // Test_0.
    {
        linkedList_initIterator(&it, list);

        uint_fast64_t i = 0;
        while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
            buf[i++] = *(uint8_t*) LINKED_LIST_ITERATOR_NEXT(&it);
        }

        if(buf[0] != 3 || buf[1] != 2 || buf[2] != 1 || buf[3] != 0){
            return TEST_FAILURE("%s", "Failed to iterate over linked list.");
        }
    }

    // Test_1.
    {
        linkedList_initIterator(&it, list);

        uint_fast64_t i = 0;
        while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
            buf[i++] = *(uint8_t*) LINKED_LIST_ITERATOR_NEXT(&it);
        }

        if(buf[0] != 3 || buf[1] != 2 || buf[2] != 1 || buf[3] != 0){
            return TEST_FAILURE("%s", "Failed to iterate over linked list.");
        }
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(linkedList_remove, LinkedList, list){
    uint8_t a = 0;
    uint8_t b = 1;
    uint8_t c = 2;
    uint8_t d = 3;

    linkedList_add(list, &a);
    linkedList_add(list, &b);
    linkedList_add(list, &c);
    linkedList_add(list, &d);

    linkedList_remove(list, &a);
    linkedList_remove(list, &b);
    
    uint8_t buf[4] = {0};

    LinkedListIterator it;
    linkedList_initIterator(&it, list);

    uint_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        buf[i++] = *(uint8_t*) LINKED_LIST_ITERATOR_NEXT(&it);
    }

    if(buf[0] != 3 || buf[1] != 2){
        return TEST_FAILURE("%s", "Failed to remove link from linked list.");
    }

    if(list->length != 2){
        return TEST_FAILURE("linked list length '%" PRIuFAST64 "' != '%d'.", list->length, 2);
    }

    return TEST_SUCCESS;
}

// util.c
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
    const char a[] = "aabbccdd--11223344";

    const int_fast64_t offset = util_findFirst_s(a, strlen(a), "--", 2);

    if(offset != 8){
        return TEST_FAILURE("Failed to find fisrt string: '--' in string:'%s'.", a);
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

TEST_TEST_FUNCTION(util_replace){
    ERROR_CODE error;

    char s[] = "--$errorCode--$errorCode--";
    char a[64] = {0};

    uint_fast64_t stringLength = strlen(s);
    
    memcpy(a, s, stringLength + 1);

    if((error = util_replace(a, 64, &stringLength, "$errorCode", strlen("$errorCode"), "404", strlen("404"))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to replace char in string:'%s'. '%s'.", a, util_toErrorString(error));
    }

    if(strcmp(a, "--404--404--") != 0){
        return TEST_FAILURE("Return value '%s' != '--404--404--'.", a);
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

TEST_TEST_FUNCTION(util_trim){
    char s[] = "  123_457 8910  ";

    util_trim(s, strlen(s));

    if(strcmp(s, "123_457 8910") != 0){
        return TEST_FAILURE("'util_trim' '%s' != '%s'.", s, "123_457 8910");
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

TEST_TEST_FUNCTION(util_fileExists){
    #define TEST_FILE_NAME "/tmp/herder_util_test_fileExists_XXXXXX"
    #define TEST_FILE_NEW_NAME "/tmp/herder_util_test_fileExists.tmp"

    char* filePath = alloca(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
    strcpy(filePath, TEST_FILE_NAME);

    char* newFileName = alloca(sizeof(*newFileName) * (strlen(TEST_FILE_NEW_NAME) + 1));
    strcpy(newFileName, TEST_FILE_NEW_NAME);

    #undef TEST_FILE_NAME

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
    if((error = util_concatenate(c, a, strlen(a), b, strlen(b) + 1)) !=  ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to concatenate strings. '%s'.", util_toErrorString(error));
    }

    if(strncmp(c, "0123456789", 11) != 0){
        return TEST_FAILURE("Failed to concatenate strings. '%s' != '0123456789'.", c);
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(util_stringCopy){
    // Test_0.
    {
        char a[] = "012345";
        char b[7] = {0};

        util_stringCopy(a, b, strlen(a) + 1);

        if(strncmp(a, b, strlen(a) + 1) != 0){
            return TEST_FAILURE("Failed to copy string. '%s' != '012345'.", b);
        }
    }

    // Test_1.
    {
        char a[] = "012345";
        
        util_stringCopy(a + 1, a + 4, 3);

        if(strncmp(a, "045", strlen("045") + 1) != 0){
            return TEST_FAILURE("Failed to copy string. '%s' != '045'.", a);
        }
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

TEST_TEST_FUNCTION(util_startsWith){
    const char a[] = "0123456789";

    if(!util_startsWith(a, '0')){
        return TEST_FAILURE("Error '%c' = '%c'.", a[0], '0');
    }

    if(util_startsWith(a, 'a')){
        return TEST_FAILURE("Error '%c' != '%c'.", a[0], 'a');
    }

    return TEST_SUCCESS;
}

// http.c
TEST_TEST_FUNCTION(http_addHeaderField){
    #define BUFFER_SIZE 8192

    void* buffer;
    if(util_blockAlloc(&buffer, BUFFER_SIZE) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to allocate buffer of size %d bytes.", BUFFER_SIZE);
    }

    HTTP_Request request;
    http_initRequest_(&request, buffer, BUFFER_SIZE, 0);

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD(request, Connection, connection);

    const HTTP_HeaderField* headerFieldConnection = http_getHeaderField(&request, "Connection");

    if(headerFieldConnection == NULL){
        return TEST_FAILURE("Failed to retrieve headeer field: '%s'.", "Connection");
    }

    if(strncmp("close", headerFieldConnection->value, headerFieldConnection->valueLength) != 0){
        return TEST_FAILURE("Header field value '%s' != '%s'.", headerFieldConnection->value, "close");
    }

    http_freeHTTP_Request(&request);

    util_unMap(buffer, BUFFER_SIZE);
    #undef BUFFER_SIZE

    return TEST_SUCCESS;
}

// que.c
TEST_TEST_FUNCTION(que_deAndEnque){
    Que que;
    que_init(&que);

    int a = 0;
    int b = 1;
    int c = 2;

    que_enque(&que, &a);
    que_enque(&que, &b);
    que_enque(&que, &c);

    int value;
    if((value = *((int*)que_deque(&que))) != 0){
        return TEST_FAILURE("'que_deque' '%d' != '%d'.", value, 0);
    }

    if((value = *((int*)que_deque(&que))) != 1){
        return TEST_FAILURE("'que_deque' '%d' != '%d'.", value, 1);
    }

    if((value = *((int*)que_deque(&que))) != 2){
        return TEST_FAILURE("'que_deque' '%d' != '%d'.", value, 2);
    }

    que_clear(&que);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(que_clear){
    Que que;
    que_init(&que);

    int a = 0;
    int b = 1;
    int c = 2;

    que_enque(&que, &a);
    que_enque(&que, &b);
    que_enque(&que, &c);

    que_clear(&que);

    if(que_deque(&que) != NULL){
        return TEST_FAILURE("Failed to clear que, 'que_deque' did not return: '%s'", "NULL");
    }

    return TEST_SUCCESS;
}

// threadPool.c
local uint_fast64_t counter = 0;

local pthread_mutex_t counterLock;

local sem_t threadSync;

THREAD_POOL_RUNNABLE(test_threadPoolRunner){
    pthread_mutex_lock(&counterLock);

    uint_fast64_t i;
    for(i = 0; i < 10; i++){
        counter++;
    }

    pthread_mutex_unlock(&counterLock);

    sem_post(&threadSync);

    return NULL;
}

TEST_TEST_FUNCTION(threadPool_run){
    ThreadPool threadPool;
    threadPool_init(&threadPool, 4);

    ERROR_CODE error;
    if((error = pthread_mutex_init(&counterLock, NULL)) != 0){
        return TEST_FAILURE("Failed to initialise 'pthread_mutex'. '%d'", error);
    }

    if((error = sem_init(&threadSync, 0, 0)) != 0){
        return TEST_FAILURE("Failed to initialise 'semaphore'. '%d'", error);
    }

    threadPool_run(&threadPool, test_threadPoolRunner, NULL);
    threadPool_run(&threadPool, test_threadPoolRunner, NULL);
    threadPool_run(&threadPool, test_threadPoolRunner, NULL);
    threadPool_run(&threadPool, test_threadPoolRunner, NULL);

    uint_fast64_t i;
    for(i = 4; i > 0; i--){
        sem_wait(&threadSync);
    }

    if(counter != 40){
        return TEST_FAILURE("Counter value '%" PRIdFAST64 "' != '%d'.", counter, 40);
    }

    pthread_mutex_destroy(&counterLock);

    sem_destroy(&threadSync);

    threadPool_free(&threadPool);

    return TEST_SUCCESS;
}

// cache.c
TEST_TEST_FUNCTION(cache_load){
    ERROR_CODE error;

    #define TEST_FILE_NAME "/tmp/herder_cache_test_file_XXXXXX"

    // TODO:(jan) Replace with temporary file.
    char* filePath = malloc(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
    strcpy(filePath, TEST_FILE_NAME);

    #undef TEST_FILE_NAME

    int fileDescriptor = mkstemp(filePath);
    if(fileDescriptor < 1){
       return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
    }

    uint8_t* buffer = malloc(sizeof(*buffer) * 256);
    memset(buffer, 8, 64);
    memset(buffer + 64, 16, 64);
    memset(buffer + 128, 8, 64);
    memset(buffer + 192, 32, 64);

    if(write(fileDescriptor, buffer, 256) != 256){
        return TEST_FAILURE("Failed to write media library file version. Expected to write %d bytes.", 256);
    }

    free(buffer);
    
    Cache cache;
    if((error = cache_init(&cache, 2, MB(8))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to initialise cache. '%s'", util_toErrorString(error));
    }

    char symbolicFileLocation[] = "/herderTestFile";

    CacheObject* cacheObject;
    if((error = cache_load(&cache, &cacheObject, filePath, strlen(filePath), symbolicFileLocation, strlen(symbolicFileLocation))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to load cache object. '%s'", util_toErrorString(error));
    }

    if(cacheObject->size != 256){
        return TEST_FAILURE("Cache object size %" PRIdFAST64 " != %d", cacheObject->size, 256);
    }

    uint_fast16_t i;
    for(i = 0; i < 256; i++){
        if(i < 64){
            if(cacheObject->data[i] != 8){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 8);
            }
        }else if(i < 128){
            if(cacheObject->data[i] != 16){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 16);
            }
        }else if(i < 192){
            if(cacheObject->data[i] != 8){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 8);
            }
        }else if(i < 256){
            if(cacheObject->data[i] != 32){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 32);
            }
        }
    }

    if((error = util_deleteFile(filePath)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to delete test file: '%s'. '%s'.", filePath, util_toErrorString(error));
    }

    cache_free(&cache);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(cache_get){
    ERROR_CODE error;

    #define TEST_FILE_NAME "/tmp/herder_cache_get_test_file_XXXXXX"

    // TODO:(jan) Replace with temporary file.
    char* filePath = malloc(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
    strcpy(filePath, TEST_FILE_NAME);

    #undef TEST_FILE_NAME

    int fileDescriptor = mkstemp(filePath);
    if(fileDescriptor < 1){
       return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
    }

    uint8_t* buffer = malloc(sizeof(*buffer) * 256);
    memset(buffer, 8, 64);
    memset(buffer + 64, 16, 64);
    memset(buffer + 128, 8, 64);
    memset(buffer + 192, 32, 64);

    if(write(fileDescriptor, buffer, 256) != 256){
        return TEST_FAILURE("Failed to write media library file version. Expected to write %d bytes.", 256);
    }

    free(buffer);
    
    Cache cache;
    if((error = cache_init(&cache, 2, MB(8))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to initialise cache. '%s'", util_toErrorString(error));
    }

    char symbolicFileLocation[] = "/herderTestFile";

    CacheObject* o;
    if((error = cache_load(&cache, &o, filePath, strlen(filePath), symbolicFileLocation, strlen(symbolicFileLocation))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to load cache object. '%s'", util_toErrorString(error));
    }

    CacheObject* cacheObject;
    if((error = cache_get(&cache, &cacheObject, symbolicFileLocation, strlen(symbolicFileLocation))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to retireve cache object. '%s'", util_toErrorString(error));
    }

    uint_fast16_t i;
    for(i = 0; i < 256; i++){
        if(i < 64){
            if(cacheObject->data[i] != 8){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 8);
            }
        }else if(i < 128){
            if(cacheObject->data[i] != 16){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 16);
            }
        }else if(i < 192){
            if(cacheObject->data[i] != 8){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 8);
            }
        }else if(i < 256){
            if(cacheObject->data[i] != 32){
                return TEST_FAILURE("failed to readcache object data '%d' != '%d'", cacheObject->data[i], 32);
            }
        }
    }
    
    if((error = util_deleteFile(filePath)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to delete test file: '%s'. '%s'.", filePath, util_toErrorString(error));
    }

    cache_free(&cache);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(cache_overflowBehaviour){
 ERROR_CODE error;

    Cache cache;
    if((error = cache_init(&cache, 2, 400)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to initialise cache. '%s'", util_toErrorString(error));
    }

    char symbolicFileLocation_000[] = "/herderTestFile_000";

    char* filePath_000;

{
     // File_0.
    #define TEST_FILE_NAME "/tmp/herder_cache_overflowBehaviour_000_XXXXXX"

    // TODO:(jan) Replace with temporary file.
    filePath_000 = malloc(sizeof(*filePath_000) * (strlen(TEST_FILE_NAME) + 1));
    strcpy(filePath_000, TEST_FILE_NAME);

    #undef TEST_FILE_NAME

    int fileDescriptor = mkstemp(filePath_000);
    if(fileDescriptor < 1){
       return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath_000, strerror(errno));
    }

    uint8_t* buffer = malloc(sizeof(*buffer) * 256);
    memset(buffer, 8, 64);
    memset(buffer + 64, 16, 64);
    memset(buffer + 128, 8, 64);
    memset(buffer + 192, 32, 64);

    if(write(fileDescriptor, buffer, 256) != 256){
        return TEST_FAILURE("Failed to write media library file version. Expected to write %d bytes.", 256);
    }

    free(buffer);  

    CacheObject* o;
    if((error = cache_load(&cache, &o, filePath_000, strlen(filePath_000), symbolicFileLocation_000, strlen(symbolicFileLocation_000))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to load cache object. '%s'", util_toErrorString(error));
    }
}

    // File_1.
    #define TEST_FILE_NAME "/tmp/herder_cache_overflowBehaviour_001_XXXXXX"

    // TODO:(jan) Replace with temporary file.
    char* filePath = malloc(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
    strcpy(filePath, TEST_FILE_NAME);

    #undef TEST_FILE_NAME

    int fileDescriptor = mkstemp(filePath);
    if(fileDescriptor < 1){
       return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
    }

    uint8_t* buffer = malloc(sizeof(*buffer) * 256);
    memset(buffer, 8, 64);
    memset(buffer + 64, 16, 64);
    memset(buffer + 128, 8, 64);
    memset(buffer + 192, 32, 64);

    if(write(fileDescriptor, buffer, 256) != 256){
        return TEST_FAILURE("Failed to write media library file version. Expected to write %d bytes.", 256);
    }

    free(buffer);

    char symbolicFileLocation[] = "/herderTestFile_001";

    CacheObject* o;
    if((error = cache_load(&cache, &o, filePath, strlen(filePath), symbolicFileLocation, strlen(symbolicFileLocation))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to load cache object. '%s'", util_toErrorString(error));
    }

    CacheObject* cacheObject;
    if((error = cache_get(&cache, &cacheObject, symbolicFileLocation_000, strlen(symbolicFileLocation_000))) == ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to retireve cache object. '%s'", util_toErrorString(error));
    }
    
    cache_free(&cache);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(cache_remove){
    ERROR_CODE error;

    #define TEST_FILE_NAME "/tmp/herder_cache_remove_test_file_XXXXXX"

    // TODO:(jan) Replace with temporary file.
    char* filePath = malloc(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
    strcpy(filePath, TEST_FILE_NAME);

    #undef TEST_FILE_NAME

    int fileDescriptor = mkstemp(filePath);
    if(fileDescriptor < 1){
       return TEST_FAILURE("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));
    }

    uint8_t* buffer = malloc(sizeof(*buffer) * 256);
    memset(buffer, 8, 64);
    memset(buffer + 64, 16, 64);
    memset(buffer + 128, 8, 64);
    memset(buffer + 192, 32, 64);

    if(write(fileDescriptor, buffer, 256) != 256){
        return TEST_FAILURE("Failed to write media library file version. Expected to write %d bytes.", 256);
    }

    free(buffer);
    
    Cache cache;
    if((error = cache_init(&cache, 2, MB(8))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to initialise cache. '%s'", util_toErrorString(error));
    }

    char symbolicFileLocation[] = "/herderTestFile";

    CacheObject* o;
    if((error = cache_load(&cache, &o, filePath, strlen(filePath), symbolicFileLocation, strlen(symbolicFileLocation))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to load cache object. '%s'", util_toErrorString(error));
    }

    if((error = util_deleteFile(filePath)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to delete test file: '%s'. '%s'.", filePath, util_toErrorString(error));
    }

    if((error = cache_remove(&cache, o)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to remove cache object. '%s'", util_toErrorString(error));
    }

    CacheObject* cacheObject;
    if((error = cache_get(&cache, &cacheObject, symbolicFileLocation, strlen(symbolicFileLocation))) == ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to retireve cache object. '%s'", util_toErrorString(error));
    }
    
    cache_free(&cache);

    return TEST_SUCCESS;
}

// propertyFile.c
TEST_TEST_SUIT_CONSTRUCT_FUNCTION(propertyFile, propertyFile){
    ERROR_CODE error = ERROR_NO_ERROR;

    char currentDir[PATH_MAX];
    util_getCurrentWorkingDirectory(currentDir, PATH_MAX);

    const uint_fast64_t currentDirLength = strlen(currentDir);
    const uint_fast64_t propertyFilePathLength = currentDirLength + 31/*"/tmp/propertyFile_add_test_file"*/;

    char* propertyFilePath = alloca(sizeof(*propertyFilePath) * (propertyFilePathLength + 1));
    strncpy(propertyFilePath, currentDir, currentDirLength);
    strncpy(propertyFilePath + currentDirLength, "/tmp/propertyFile_add_test_file", 32);

    if(util_fileExists(propertyFilePath)){
        if((error = util_deleteFile(propertyFilePath)) != ERROR_NO_ERROR){
            goto label_return;
        }
    }

    if((error = propertyFile_create(propertyFilePath, propertyFilePathLength)) != ERROR_NO_ERROR){
        goto label_return;
    }

    *propertyFile = malloc(sizeof(PropertyFile));

    if(*propertyFile == NULL){
        error = ERROR_OUT_OF_MEMORY;

        goto label_return;
    }

    if((error = propertyFile_init(*propertyFile, propertyFilePath)) != ERROR_NO_ERROR){
        goto label_return;
    }

label_return:
    return ERROR(error);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(propertyFile, propertyFile){
    char currentDir[PATH_MAX];
    util_getCurrentWorkingDirectory(currentDir, PATH_MAX);

    const uint_fast64_t currentDirLength = strlen(currentDir);
    const uint_fast64_t propertyFilePathLength = currentDirLength + 31/*"/tmp/propertyFile_add_test_file"*/;

    char* propertyFilePath = alloca(sizeof(*propertyFilePath) * (propertyFilePathLength + 1));
    strncpy(propertyFilePath, currentDir, currentDirLength);
    strncpy(propertyFilePath + currentDirLength, "/tmp/propertyFile_add_test_file", 32);

    propertyFile_free(propertyFile);

    ERROR_CODE error;
    error = util_deleteFile(propertyFilePath);

    free(propertyFile);

    return ERROR(error);
}

TEST_TEST_FUNCTION(propertyFile_create){
    ERROR_CODE error;

    char currentDir[PATH_MAX];
    util_getCurrentWorkingDirectory(currentDir, PATH_MAX);

    const uint_fast64_t currentDirLength = strlen(currentDir);
    const uint_fast64_t propertyFilePathLength = currentDirLength + 34/*"/tmp/propertyFile_create_test_file"*/;

    char* propertyFilePath = alloca(sizeof(*propertyFilePath) * (propertyFilePathLength + 1));
    strncpy(propertyFilePath, currentDir, currentDirLength);
    strncpy(propertyFilePath + currentDirLength, "/tmp/propertyFile_create_test_file", 35);
    
    if(util_fileExists(propertyFilePath)){
        if(!util_deleteFile(propertyFilePath)){
            return TEST_FAILURE("Failed to delete old file '%s'.\n", propertyFilePath);
        }
    }

    if((error = propertyFile_create(propertyFilePath, propertyFilePathLength)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to create property file '%s'. '%s'", propertyFilePath, util_toErrorString(error));
    }

    util_deleteFile(propertyFilePath);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(propertyFile_addProperty, PropertyFile, propertyFile){
    ERROR_CODE error;

    Property* testProperty;
    if((error = propertyFile_addProperty(propertyFile, &testProperty, "testProperty", sizeof(uint64_t))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add property:'testProperty'. '%s'.", util_toErrorString(error));
    }

    int8_t* buffer = alloca(sizeof(uint16_t));
    util_uint64ToByteArray(buffer, 19875431121);

    propertyFile_setBuffer(testProperty, buffer);

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    if(propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty") != ERROR_NO_ERROR){
       return TEST_FAILURE("Failed to retrieve property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    propertyFile_freeProperty(retrievedProperty);
    free(retrievedProperty);

    return TEST_SUCCESS;
}    

TEST_TEST_FUNCTION_(propertyFile_getProperty, PropertyFile, propertyFile){
    ERROR_CODE error;

    Property* testProperty;
    if((error = propertyFile_addProperty(propertyFile, &testProperty, "testProperty", sizeof(uint64_t))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add property:'testProperty'. '%s'.", util_toErrorString(error));
    }

    int8_t* buffer = alloca(sizeof(uint16_t));
    util_uint64ToByteArray(buffer, 19875431121);

    propertyFile_setBuffer(testProperty, buffer);

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    if(propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty") != ERROR_NO_ERROR){
       return TEST_FAILURE("Failed to retrieve property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    propertyFile_freeProperty(retrievedProperty);
    free(retrievedProperty);

    return TEST_SUCCESS;
}    

TEST_TEST_FUNCTION_(propertyFile_contains, PropertyFile, propertyFile){
    ERROR_CODE error;

    Property* testProperty;
    if((error = propertyFile_addProperty(propertyFile, &testProperty, "testProperty", sizeof(uint64_t))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add property:'testProperty'. '%s'.", util_toErrorString(error));
    }

    int8_t* buffer = alloca(sizeof(uint16_t));
    util_uint64ToByteArray(buffer, 19875431121);

    propertyFile_setBuffer(testProperty, buffer);

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    if(!propertyFile_contains(propertyFile, "testProperty")){
       return TEST_FAILURE("Failed to retrieve property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    if(!propertyFile_contains(propertyFile, "testProperty_2")){
       return TEST_FAILURE("Failed to retrieve property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    return TEST_SUCCESS;
}    

TEST_TEST_FUNCTION_(propertyFile_removeProperty, PropertyFile, propertyFile){
    ERROR_CODE error;

    Property* testProperty;
    propertyFile_addProperty(propertyFile, &testProperty, "testProperty", sizeof(uint64_t));

    int8_t* buffer = alloca(sizeof(uint16_t));
    util_uint64ToByteArray(buffer, 222333444555);

    propertyFile_setBuffer(testProperty, buffer);

    if((error = propertyFile_removeProperty(testProperty)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to remove property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if((error = propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty")) != ERROR_ENTRY_NOT_FOUND){
        propertyFile_freeProperty(retrievedProperty);
        free(retrievedProperty);

        return TEST_FAILURE("'propertyFile_getProperty' did not fail as expected with 'ERROR_ENTRY_NOT_FOUND' function returned '%s'", util_toErrorString(error));
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(propertyFile_createAndSetStringProperty, PropertyFile, propertyFile){
    ERROR_CODE error;

    #define TEST_STRING "test_123"

    Property* testProperty = NULL;

    if((error = propertyFile_createAndSetStringProperty(propertyFile, &testProperty, "testProperty", TEST_STRING, strlen(TEST_STRING))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add property:'testProperty'. '%s'.", util_toErrorString(error));
    }

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    if(propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty") != ERROR_NO_ERROR){
       return TEST_FAILURE("Failed to retrieve property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    if(strncmp(TEST_STRING , (char*) retrievedProperty->buffer, strlen(TEST_STRING) + 1) != 0){
        return TEST_FAILURE("Error 'testProperty' value '%s' != '%s'.", (char*) retrievedProperty->buffer, TEST_STRING);
    }

    #undef TEST_STRING

    propertyFile_freeProperty(retrievedProperty);
    free(retrievedProperty);

    return TEST_SUCCESS;
}   

TEST_TEST_FUNCTION_(propertyFile_createAndSetDirectoryProperty, PropertyFile, propertyFile){
    ERROR_CODE error;

    #define IMPORT_DIRECTORY "/home/ex05/herder/import"

    Property* testProperty = NULL;
    if((error = propertyFile_createAndSetDirectoryProperty(propertyFile, &testProperty, "testProperty", IMPORT_DIRECTORY, strlen(IMPORT_DIRECTORY))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add property:'testProperty'. '%s'.", util_toErrorString(error));
    }

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    if(propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty") != ERROR_NO_ERROR){
       return TEST_FAILURE("Failed to retrieve property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    if(strncmp(IMPORT_DIRECTORY "/", (char*) retrievedProperty->buffer, strlen(IMPORT_DIRECTORY) + 2) != 0){
        return TEST_FAILURE("Error 'testProperty' value '%s' != '%s'.", (char*) retrievedProperty->buffer, IMPORT_DIRECTORY "/");
    }

    #undef IMPORT_DIRECTORY

    propertyFile_freeProperty(retrievedProperty);
    free(retrievedProperty);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(propertyFile_createAndSetUINT16Property, PropertyFile, propertyFile){
    ERROR_CODE error;

    #define TEST_VALUE 124

    Property* testProperty = NULL;
    if((error = propertyFile_createAndSetUINT16Property(propertyFile, &testProperty, "testProperty", TEST_VALUE)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add property:'testProperty'. '%s'.", util_toErrorString(error));
    }

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    if(propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty") != ERROR_NO_ERROR){
       return TEST_FAILURE("Failed to retrieve property 'testProperty'. '%s'.", util_toErrorString(error));
    }

    const uint16_t retrievedValue = util_byteArrayTo_uint16(retrievedProperty->buffer);

    if(retrievedValue != TEST_VALUE){
        return TEST_FAILURE("Error 'testProperty' value '%" PRIu16 "' != '%" PRIu16" '.", retrievedValue, TEST_VALUE);
    }

    #undef TEST_VALUE

    propertyFile_freeProperty(retrievedProperty);
    free(retrievedProperty);

    return TEST_SUCCESS;
}   

// herder.c
TEST_TEST_FUNCTION(herder_constructFilePath){
    EpisodeInfo info;
    mediaLibrary_initEpisodeInfo(&info);

    char showName[] = "American Dad";
    char episodeName[] = "The last Smith";
    char fileExtension[] = "mkv";

    info.showName = showName;
    info.showNameLength = strlen(showName);

    info.name = episodeName;
    info.nameLength = strlen(episodeName);

    info.season = 2;
    info.episode = 14;

    info.fileExtension = fileExtension;
    info.fileExtensionLength = strlen(fileExtension);

    char* path;
    uint_fast64_t pathLength;
    HERDER_CONSTRUCT_RELATIVE_FILE_PATH(&path, &pathLength, &info);

    if(strcmp(path, "American_Dad/American_Dad - Season_02/American_Dad_s02e14_The_last_Smith.mkv") != 0){
       return TEST_FAILURE("'%s' does not equal '%s'.", path, "American_Dad/American_Dad - Season_02/American_Dad_s02e14_The_last_Smith.mkv");
    }

    return TEST_SUCCESS;
}

// mediaLibrary.c
TEST_TEST_SUIT_CONSTRUCT_FUNCTION(mediaLibrary, library){
    ERROR_CODE error = ERROR_NO_ERROR;

    char currentDir[PATH_MAX];
    util_getCurrentWorkingDirectory(currentDir, PATH_MAX);

    const uint_fast64_t currentDirLength = strlen(currentDir);
    const uint_fast64_t libraryFileLocationLength = currentDirLength + 5/*"/tmp/"*/;

    char* libraryFileLocation = alloca(sizeof(*libraryFileLocation) * (libraryFileLocationLength + 1));
    strncpy(libraryFileLocation, currentDir, currentDirLength);
    strncpy(libraryFileLocation + currentDirLength, "/tmp/", 6);

    *library = malloc(sizeof(MediaLibrary));

    if((error = mediaLibrary_init(*library, libraryFileLocation, libraryFileLocationLength)) != ERROR_NO_ERROR){
        goto label_error;
    }

label_error:
    return ERROR(error);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(mediaLibrary, library){
    ERROR_CODE error;

    mediaLibrary_free(library);

    char currentDir[PATH_MAX];
    util_getCurrentWorkingDirectory(currentDir, PATH_MAX);

    const uint_fast64_t currentDirLength = strlen(currentDir);
    const uint_fast64_t libraryFileLocationLength = currentDirLength + 5/*"/tmp/"*/;

    char* libraryFileLocation = alloca(sizeof(*libraryFileLocation) * (libraryFileLocationLength + 1));
    strncpy(libraryFileLocation, currentDir, currentDirLength);
    strncpy(libraryFileLocation + currentDirLength, "/tmp/", 6);

    char* libraryFilePath = alloca(sizeof(*libraryFilePath) * (libraryFileLocationLength + 8/*lib_data*/ + 1));
    strncpy(libraryFilePath, libraryFileLocation, libraryFileLocationLength);
    strncpy(libraryFilePath + libraryFileLocationLength, "lib_data", 9);

    if(util_fileExists(libraryFilePath)){
        if((error = util_deleteFile(libraryFilePath)) != ERROR_NO_ERROR){
            UTIL_LOG_ERROR_("Failed to delete media library file. '%s'.", libraryFilePath);
        }
    }
    
    free(library);

    return ERROR(error);
}

TEST_TEST_FUNCTION_(mediaLibrary_getShow, MediaLibrary, library){
    ERROR_CODE error;

    char showName[] = "American Dad";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* show;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if((error = medialibrary_getShow(library, &show, showName, showNameLength)) != ERROR_ENTRY_NOT_FOUND){
        return TEST_FAILURE("'medialibrary_getShow' did not fail as expected with 'ERROR_ENTRY_NOT_FOUND' function returned '%s'", util_toErrorString(error));
    }

    if((error = mediaLibrary_addShow(library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add show: '%s'. '%s'.", showName, util_toErrorString(error));
    }

    if((error = medialibrary_getShow(library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to retrieve show '%s' from media library. '%s'.", showName, util_toErrorString(error));
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(mediaLibrary_getSeason, MediaLibrary, library){
    ERROR_CODE error;

    char showName[] = "American Dad";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* americanDad;
    if((error = mediaLibrary_addShow(library, &americanDad, showName, showNameLength)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add show: '%s'. '%s'.", showName, util_toErrorString(error));
    }

    Season* season_1;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if((error = medialibrary_getSeason(americanDad, &season_1, 1)) != ERROR_ENTRY_NOT_FOUND){
        return TEST_FAILURE("'mediaLiobrary_getSeason' did not fail as expected with 'ERROR_ENTRY_NOT_FOUND' function returned '%s'", util_toErrorString(error));
    }

    if((error = mediaLibrary_addSeason(library, &season_1, americanDad, 1)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add season 01 to '%s'. '%s'.", showName, util_toErrorString(error));
    }

    if((error = medialibrary_getSeason(americanDad, &season_1, 1)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to retrieve season '%d' from media library. '%s'.", 1, util_toErrorString(error));
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(mediaLibrary_addEpisode, MediaLibrary, library){
    ERROR_CODE error;

    char showName[] = "American Dad";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* americanDad;
    if((error = mediaLibrary_addShow(library, &americanDad, showName, showNameLength)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add show: '%s'. '%s'.", showName, util_toErrorString(error));
    }

    Season* season_01;
    if(mediaLibrary_addSeason(library, &season_01, americanDad, 1) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add season 01 to '%s'. '%s'.", showName, util_toErrorString(error));
    }

    Episode* episode;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if((error = mediaLibrary_getEpisode(season_01, &episode, 15)) != ERROR_ENTRY_NOT_FOUND){
        return TEST_FAILURE("'mediaLiobrary_getEpisode' did not fail as expected with 'ERROR_ENTRY_NOT_FOUND' function returned '%s'", util_toErrorString(error));
    }

    char episodeName[] = "Threat Levels";
    const uint_fast64_t episodeNameLength = strlen(episodeName);

    if(mediaLibrary_addEpisode(library, &episode, americanDad, season_01, 15, episodeName, episodeNameLength, ".mkv", 4, true) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add episode '%s' to media library. '%s'.", episodeName, util_toErrorString(error));
    }

    if(mediaLibrary_getEpisode(season_01, &episode, 15) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to retrieve episode '%s' from media library. '%s'.", episodeName, util_toErrorString(error));
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(medialibrary_removeEpisode, MediaLibrary, library){
    ERROR_CODE error;

    const char showName[] = "Enen no Shouboutai";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* show;
    if((error = mediaLibrary_addShow(library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add show: '%s'. '%s'.", showName, util_toErrorString(error));
    }

    Season* season_01;
    if(mediaLibrary_addSeason(library, &season_01, show, 1) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add season 01 to '%s'. '%s'.", showName, util_toErrorString(error));
    }

    const char episodeName[] = "Shinra Kusakabe Enlists";
    const uint_fast64_t episodeNameLength = strlen(episodeName);

    Episode* episode_01;
    if(mediaLibrary_addEpisode(library, &episode_01, show, season_01, 1, episodeName, episodeNameLength, ".mkv", 4, true) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add episode '%s' to media library. '%s'.", episodeName, util_toErrorString(error));
    }

    if(medialibrary_removeEpisode(library, show, season_01, episode_01, true) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to remove episode '%s' from media library. '%s'.", episodeName, util_toErrorString(error));
    }

    if(!LINKED_LIST_IS_EMPTY(&show->seasons)){
        return TEST_FAILURE("Failed to remove episode '%s' from media library. '%s'.", episodeName, util_toErrorString(error));
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(medialibrary_removeEpisodeFrromLibraryFile, MediaLibrary, library){
    ERROR_CODE error;

    const char showName[] = "Enen no Shouboutai";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* show;
    if((error = mediaLibrary_addShow(library, &show, showName, showNameLength)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add show:'%s'. '%s'.", showName, util_toErrorString(error));
    }

    Season* season_01;
    if((error = mediaLibrary_addSeason(library, &season_01, show, 1)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add season 01 to '%s'. '%s'.", showName, util_toErrorString(error));
    }

    const char episodeName[] = "Shinra Kusakabe Enlists";
    const uint_fast64_t episodeNameLength = strlen(episodeName);

    Episode* episode_01;
    if((error = mediaLibrary_addEpisode(library, &episode_01, show, season_01, 1, episodeName, episodeNameLength, ".mkv", 4, true)) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to add episode '%s' to media library. '%s'.", episodeName, util_toErrorString(error));
    }

    if(medialibrary_removeEpisodeFrromLibraryFile(library, show, season_01, episode_01) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to remove episode '%s' from library file. '%s'.", episode_01->name, util_toErrorString(error));
    } 

    // TODO: Reload library and check if episode was removed. (jan - 2019.08.08)

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(mediaLibrary_extractShowName){
    ERROR_CODE error;

    LinkedList shows;
    linkedList_init(&shows);

    Show familyGuy;
    familyGuy.name = "Family Guy";
    familyGuy.nameLength = strlen(familyGuy.name);

    linkedList_add(&shows, &familyGuy);

    Show americanDad;
    americanDad.name = "American Dad";
    americanDad.nameLength = strlen(americanDad.name);

    linkedList_add(&shows, &americanDad);

    Show avatar;
    avatar.name = "Avatar - The Last Airbender";
    avatar.nameLength = strlen(avatar.name);

    linkedList_add(&shows, &avatar);

    Show drawnTogether;
    drawnTogether.name = "Drawn Together";
    drawnTogether.nameLength = strlen(drawnTogether.name);

    linkedList_add(&shows, &drawnTogether);

    Show starGateAtlantis;
    starGateAtlantis.name = "Stargate Atlantis";
    starGateAtlantis.nameLength = strlen(starGateAtlantis.name);

    linkedList_add(&shows, &starGateAtlantis);

    // Test_0.
    {
        EpisodeInfo info = {0};

        // 1x1 The Hot Tub.avi
        char fileName[] = "American_Dad_Rogers_Spot";
        if((error = mediaLibrary_extractShowName(&info, &shows, fileName, strlen(fileName))) != ERROR_NO_ERROR){
            return TEST_FAILURE("Failed to extract show name from string:'%s'. '%s'.", fileName, util_toErrorString(error));
        }

        if(strncmp(info.showName, americanDad.name, americanDad.nameLength + 1) != 0){
            return TEST_FAILURE("Extracted value '%s' != '%s'.", info.name, americanDad.name);
        }

        if(strncmp(fileName, "Rogers_Spot", strlen("Rogers_Spot") + 1) != 0){
            return TEST_FAILURE("Extracted value '%s' != '%s'.", fileName, "Rogers_Spot");
        }

        mediaLibrary_freeEpisodeInfo(&info);
    }

    // Test_1.
    {
        EpisodeInfo info = {0};

        char fileName[] = "Family_Guy_s01e01_Death_Has_a_Shadow.avi";
        if(mediaLibrary_extractShowName(&info, &shows, fileName, strlen(fileName)) != ERROR_NO_ERROR){
            return TEST_FAILURE("Failed to extract show name from string:'%s'. '%s'.", fileName, util_toErrorString(error));
        }

        if(strncmp(info.showName, familyGuy.name, familyGuy.nameLength + 1) != 0){
            return TEST_FAILURE("Extracted value '%s' != '%s'.", info.name, familyGuy.name);
        }

        if(strncmp(fileName, "s01e01_Death_Has_a_Shadow.avi", strlen("s01e01_Death_Has_a_Shadow.avi") + 1) != 0){
            return TEST_FAILURE("Extracted value '%s' != '%s'.", fileName, "s01e01_Death_Has_a_Shadow.avi");
        }

        mediaLibrary_freeEpisodeInfo(&info);
    }
    
    // Test_2.
    {
        EpisodeInfo info = {0};

        char fileName[] = "s01e01_Death_Has_a_Shadow.avi";
        __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_INCOMPLETE);
        if(mediaLibrary_extractShowName(&info, &shows, fileName, strlen(fileName)) != ERROR_INCOMPLETE){
            return TEST_FAILURE("Extracting a show name from string:'%s' did not fail as expected. '%s'.", fileName, util_toErrorString(error));
        }

        mediaLibrary_freeEpisodeInfo(&info);
    }

    // Test_3.
    {
        EpisodeInfo info = {0};

        char fileName[] = "Stargate.Atlantis.S05E01.Such.und.Rettungsaktion.GERMAN.DUBBED.DL.720p.BluRay.x264-TVP.mkv";
        if(mediaLibrary_extractShowName(&info, &shows, fileName, strlen(fileName)) != ERROR_NO_ERROR){
        
        }

        if(strncmp(info.showName, starGateAtlantis.name, starGateAtlantis.nameLength + 1) != 0){
            return TEST_FAILURE("Extracted value '%s' != '%s'.", info.name, starGateAtlantis.name);
        }

        if(strncmp(fileName, "S05E01.Such.und.Rettungsaktion.GERMAN.DUBBED.DL.720p.BluRay.x264-TVP.mkv", strlen("S05E01.Such.und.Rettungsaktion.GERMAN.DUBBED.DL.720p.BluRay.x264-TVP.mkv") + 1) != 0){
        return TEST_FAILURE("Extracted value '%s' != '%s'.", fileName, "S05E01.Such.und.Rettungsaktion.GERMAN.DUBBED.DL.720p.BluRay.x264-TVP.mkv");
        }

        mediaLibrary_freeEpisodeInfo(&info);
    }

    linkedList_free(&shows);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(mediaLibrary_extractPrefixedNumber){
    ERROR_CODE error;

    char a[] = "American_Dad_s01e02_Threat_Levels.mkv";

    int_fast16_t val;
    if((error = mediaLibrary_extractPrefixedNumber(a, strlen(a), &val, 's')) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to extract prefixed number from string:'%s'. '%s'.", a, util_toErrorString(error));
    }

    if(val != 1){
        return TEST_FAILURE("Extracted value '%" PRIdFAST16 "' != '%d'.", val, 1);
    }

    if(strncmp(a, "American_Dad_e02_Threat_Levels.mkv", strlen("American_Dad_e02_Threat_Levels.mkv")) != 0){
        return TEST_FAILURE("'%" PRIdFAST16 "' != '%d'.", val, 2);
    }

    if((error = mediaLibrary_extractPrefixedNumber(a, strlen(a), &val, 'e')) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to extract prefixed number from string:'%s'. '%s'.", a, util_toErrorString(error));
    }

    if(val != 2){
        return TEST_FAILURE("Extracted value '%" PRIdFAST16 "' != '%d'.", val, 2);
    }

    if(strncmp(a, "American_Dad__Threat_Levels.mkv", strlen("American_Dad__Threat_Levels.mkv")) != 0){
        return TEST_FAILURE("'%s' != '%s'.", a, "American_Dad__Threat_Levels.mkv");
    }

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(mediaLibrary_extractEpisodeInfo){
    char a[] = "American_Dad_s01e01_Threat_Levels.mkv";

    LinkedList shows;
    linkedList_init(&shows);

    const char showName[] = "American Dad";

    Show americanDad;
    medialibrary_initShow(&americanDad, showName, strlen(showName));

    linkedList_add(&shows, &americanDad);

    EpisodeInfo info;
    mediaLibrary_initEpisodeInfo(&info);

    ERROR_CODE error;
    if((error = mediaLibrary_extractEpisodeInfo(&info, &shows, a, strlen(a))) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to extract episode info. '%s'.", util_toErrorString(error));
    }

    if(info.season != 1){
        return TEST_FAILURE("Failed to extract season number '%" PRIdFAST16 "' != '%d'.", info.season, 1);
    } 

    if(info.episode != 1){
        return TEST_FAILURE("Failed to extract episode number '%" PRIdFAST16 "' != '%d'.", info.episode, 1);
    }

    if(strncmp(showName, info.showName, strlen(showName))){
        return TEST_FAILURE("Failed to extract show name '%s' != '%s'.", info.showName, showName);
    }
        
    if(strncmp("Threat Levels", info.name, strlen("Threat Levels"))){
        return TEST_FAILURE("Failed to extract episode name '%s' != '%s'.", info.name, "Threat Levels");
    }

    mediaLibrary_freeEpisodeInfo(&info);

    linkedList_free(&americanDad.seasons);

    free(americanDad.name);

    linkedList_free(&shows);

    return TEST_SUCCESS;
}

TEST_TEST_FUNCTION(mediaLibrary_sortEpisodes){
    LinkedList unsortedEpisdoes;
    linkedList_init(&unsortedEpisdoes);

    Episode s01e04;
    medialibrary_initEpisode(&s01e04, 4, "Francine's Flashback", strlen("Francine's Flashback"), "mkv", 3);
    linkedList_add(&unsortedEpisdoes, &s01e04);

    Episode s01e05;
    medialibrary_initEpisode(&s01e05, 5, "Roger Codger", strlen("Roger Codger"), "mkv", 3);
    linkedList_add(&unsortedEpisdoes, &s01e05);

    Episode s01e01;
    medialibrary_initEpisode(&s01e01, 1, "Pilot", strlen("Pilot"), "mkv", 3);
    linkedList_add(&unsortedEpisdoes, &s01e01);

    Episode s01e03;
    medialibrary_initEpisode(&s01e03, 3, "Stan Knows Best", strlen("Stan Knows Best"), "mkv", 3);
    linkedList_add(&unsortedEpisdoes, &s01e03);

    Episode s01e02;
    medialibrary_initEpisode(&s01e02, 2, "Threat Levels", strlen("Threat Levels"), "mkv", 3);
    linkedList_add(&unsortedEpisdoes, &s01e02);

    Episode** episodes = alloca(sizeof(Episode) * 5);

    mediaLibrary_sortEpisodes(&episodes, &unsortedEpisdoes);

    uint_fast64_t i;
    for(i = 0; i < 5; i++){
        if(episodes[i]->number != i + 1){
            return TEST_FAILURE("Failed to sort episode;'%" PRIiFAST64 "'", i + 1);
        }
    }

    mediaLibrary_freeEpisode(&s01e04);
    mediaLibrary_freeEpisode(&s01e05);
    mediaLibrary_freeEpisode(&s01e01);
    mediaLibrary_freeEpisode(&s01e03);
    mediaLibrary_freeEpisode(&s01e02);

    linkedList_free(&unsortedEpisdoes);

    return TEST_SUCCESS;
}

// server.c
TEST_TEST_FUNCTION(server_addContext){
    bool ret = true;

    char* userHome = util_getHomeDirectory();
    const uint_fast64_t userHomeLength = strlen(userHome);

    char* serverRootDirectory = alloca(sizeof(*serverRootDirectory) * (userHomeLength + 12/*/herder/www/*/ + 1));
    strncpy(serverRootDirectory, userHome, userHomeLength);
    strncpy(serverRootDirectory + userHomeLength, "/herder/www/", 13);

    const uint_fast64_t serverRootDirectoryLength = strlen(serverRootDirectory);

    HerderServer server;
    ERROR_CODE error;
    if((error = server_init(&server, serverRootDirectory, serverRootDirectoryLength, 8088) != ERROR_NO_ERROR)){
        ret = false;

        goto label_free;
    }
  
    #define BUFFER_SIZE 8096
    void* buffer;
    if(util_blockAlloc(&buffer, BUFFER_SIZE) != ERROR_NO_ERROR){
        goto label_free;
    }

    const char requestURL[] = "/img/img_001.png";

    HTTP_Request request;
    http_initRequest(&request, requestURL, strlen(requestURL), buffer, 8096, HTTP_VERSION_1_0, REQUEST_TYPE_GET);

    ContextHandler* contextHandler;
    if((error = server_getContext(&server, &contextHandler, &request)) == ERROR_NO_ERROR){
        ret = false;

        goto label_free;
    }

    if((error = server_addContext(&server, "/img", server_defaultContextHandler)) != ERROR_NO_ERROR){
        ret = false;

        goto label_free;
    }

    if((error = server_getContext(&server, &contextHandler, &request)) != ERROR_NO_ERROR){
        ret = false;

        goto label_free;
    }

    if(contextHandler == NULL){
        ret = false;

        goto label_free;
    }

    // Test_2.
    {
        const char requestURL[] = "check.proxyradar.com:80";

        UTIL_LOG_CONSOLE_(LOG_DEBUG, "Test:'%s'", requestURL);

        http_freeHTTP_Request(&request);

        http_initRequest(&request, requestURL, strlen(requestURL), buffer, 8096, HTTP_VERSION_1_0, REQUEST_TYPE_GET);

        ContextHandler* contextHandler;
        if((error = server_getContext(&server, &contextHandler, &request)) == ERROR_NO_ERROR){
            ret = false;

            UTIL_LOG_CONSOLE(LOG_DEBUG, "GET_CONTEXT.");

            goto label_free;
        }

        if(contextHandler != NULL){
            ret = false;

            goto label_free;
        }
    }


label_free:
    http_freeHTTP_Request(&request);

    server_free(&server);

    util_unMap(buffer, BUFFER_SIZE);
    #undef BUFFER_SIZE

    return ret;
}

local SERVER_CONTEXT_HANDLER(server_testContextHandler){
    http_setHTTP_Version(response, HTTP_VERSION_1_1);
    response->statusCode = _200_OK;

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD((*response), Connection, connection);
 
    const char str[] = "Test !~!";
    const uint_fast64_t stringLength = strlen(str);

    memcpy(response->data + response->dataLength, str, stringLength + 1);
    response->dataLength += stringLength + 1;

    return ERROR(ERROR_NO_ERROR);
}

local THREAD_POOL_RUNNABLE(server_thread){
    char* userHome = util_getHomeDirectory();
    const uint_fast64_t userHomeLength = strlen(userHome);

    char* serverRootDirectory = alloca(sizeof(*serverRootDirectory) * (userHomeLength + 12/*/herder/www/*/ + 1));
    strncpy(serverRootDirectory, userHome, userHomeLength);
    strncpy(serverRootDirectory + userHomeLength, "/herder/www/", 13);

    const uint_fast64_t serverRootDirectoryLength = strlen(serverRootDirectory);

    HerderServer server;
    ERROR_CODE error;
    if((error = server_init(&server, serverRootDirectory, serverRootDirectoryLength, 8888) != ERROR_NO_ERROR)){
        goto label_free;
    }

    server_addContext(&server, "/test", server_testContextHandler);

    server_start(&server);

label_free:
    server_free(&server);

    return NULL;
}

TEST_TEST_FUNCTION(server_send){
    ERROR_CODE error;

    ThreadPool threadPool;
    threadPool_init(&threadPool, 1);

    threadPool_run(&threadPool, server_thread, NULL);

    sleep(2);

    #define BUFFER_SIZE 8096
    void* requestBuffer;
    if((error = util_blockAlloc(&requestBuffer, BUFFER_SIZE)) != ERROR_NO_ERROR){
        return TEST_FAILURE("ERROR:'%s'.", util_toErrorString(error));
    }

    const char url[] = "/test";
    const uint_fast64_t urlLength = strlen(url);

    HTTP_Request request;
    if((error = http_initRequest(&request, url, urlLength, requestBuffer, BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_GET)) != ERROR_NO_ERROR){
        return TEST_FAILURE("ERROR:'%s'.", util_toErrorString(error));
    }

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD(request, Connection, connection);
   
    void* responseBuffer;
    if((error = util_blockAlloc(&responseBuffer, BUFFER_SIZE)) != ERROR_NO_ERROR){
        return TEST_FAILURE("ERROR:'%s'.", util_toErrorString(error));
    }

    HTTP_Response response;
    http_initResponse(&response, responseBuffer, BUFFER_SIZE);

    int_fast32_t socketFD;
    if(http_openConnection(&socketFD, "localhost", 8888) != ERROR_NO_ERROR){
        return TEST_FAILURE("Failed to open connection to '%s:%d'.", "localhost", 8888);
    }

    http_sendRequest(&request, &response, socketFD);
    
    http_closeConnection(socketFD);

    http_freeHTTP_Request(&request);

    http_freeHTTP_Response(&response);

    threadPool_free(&threadPool);

    util_unMap(responseBuffer, BUFFER_SIZE);
    util_unMap(requestBuffer, BUFFER_SIZE);
    #undef BUFFER_SIZE

    return TEST_SUCCESS;
}

// main
int main(void){
    TEST_BEGIN();

    TEST_SUIT_BEGIN("arrayList");
        TEST(arraylist_iteration);
        TEST(arraylist_fixedSizedStackList);
        TEST(arraylist_get);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN_(linkedList);
        TEST(linkedList_iteration);
        TEST(linkedList_remove);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("util");
        // ByteArray to Integer conversions.
        TEST(util_byteArrayTo_uint16);
        TEST(util_byteArrayTo_uint32);
        TEST(util_byteArrayTo_uint64);

        // Integer to ByteArray conversions.
        TEST(util_uint16ToByteArray);
        TEST(util_uint32ToByteArray);
        TEST(util_uint64ToByteArray);
        TEST(util_formatNumber);

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
        TEST(util_startsWith);

        // File I/O.
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
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("argumentParser");
        TEST(argumentParser_parse);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("http");
        TEST(http_addHeaderField);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("que");
        TEST(que_deAndEnque);
        TEST(que_clear);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("threadPool");
        TEST(threadPool_run);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("cache");
        TEST(cache_load);
        TEST(cache_get);
        TEST(cache_remove);
        TEST(cache_overflowBehaviour);
    TEST_SUIT_END();
    
    TEST_SUIT_BEGIN_(propertyFile);
        __TEST_NO_SETUP__(); TEST(propertyFile_create);
        TEST(propertyFile_addProperty);
        TEST(propertyFile_createAndSetUINT16Property);
        TEST(propertyFile_createAndSetStringProperty);
        TEST(propertyFile_createAndSetDirectoryProperty);
        TEST(propertyFile_removeProperty);
        TEST(propertyFile_getProperty);
        TEST(propertyFile_contains);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN_(mediaLibrary);
        TEST(mediaLibrary_getShow);
        TEST(mediaLibrary_getSeason);
        TEST(mediaLibrary_addEpisode);
        TEST(medialibrary_removeEpisode);
        TEST(medialibrary_removeEpisodeFrromLibraryFile);
        TEST_NO_SETUP(mediaLibrary_extractShowName);
        TEST_NO_SETUP(mediaLibrary_extractPrefixedNumber);
        TEST_NO_SETUP(mediaLibrary_extractEpisodeInfo);
        TEST_NO_SETUP(mediaLibrary_sortEpisodes);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("herder");
        TEST(herder_constructFilePath);
    TEST_SUIT_END();

   /* TEST_SUIT_BEGIN("server");
        // Note:(jan) Running the server in valgrind takes to long, probably due to the 'accept' call having to be interrupted in the server loop.
        TEST(server_addContext);
        // Note:(jan) No need to run this, as this was not meant to be an automated test, and needed console output in 'http.c' that is no longer present to be useful.
        // TEST(server_send);
    TEST_SUIT_END(); */

    TEST_END();
}