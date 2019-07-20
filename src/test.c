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
// END_HEADER.

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

    bool ret = true;
    if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
        ret = false;
    }

    arrayList_free(&list);

    return ret;
}

TEST_TEST_FUNCTION(arraylist_fixedSizedHeapList){
    ArrayList list;
    ARRAY_LIST_INIT_FIXED_SIZE_HEAP_LIST((&list), 4, sizeof(uint8_t));

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

    bool ret = true;
    if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
        ret = false;
    }

    return ret;
}

TEST_TEST_FUNCTION(arraylist_get){
    ArrayList list;
    ARRAY_LIST_INIT_FIXED_SIZE_HEAP_LIST((&list), 4, sizeof(uint8_t));

    ARRAY_LIST_ADD(&list, 0, uint8_t);
    ARRAY_LIST_ADD(&list, 1, uint8_t);
    ARRAY_LIST_ADD(&list, 2, uint8_t);
    ARRAY_LIST_ADD(&list, 3, uint8_t);

    bool ret = false;
    if(ARRAY_LIST_GET(&list, 1, uint8_t) == 1){
        ret = true;
    }

    return ret;
}

// arguemntParser.c
TEST_TEST_FUNCTION(argumentParser_parse){
    ArgumentParser parser;
    argumentParser_init(&parser);

    #if !defined __GNUC__ && !defined __GNUG__
        // TODO:(jan); Add preprocessor macros for other compiler.
    #endif

    ARGUMENT_PARSER_ADD_ARGUMENT(Test, 2, "-t", "--test");
    ARGUMENT_PARSER_ADD_ARGUMENT(Import, 1, "-i");

    bool ret = false;

    const char* argc[] = {"-i", "test", "12", "--test", "abbab", "---Tester", "12"};

    if(argumentParser_parse(&parser, UTIL_ARRAY_LENGTH(argc), argc) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to parse command line arguments.");

        goto label_free;
    }

    if(argumentParser_contains(&parser, &argumentTest)){
         if(ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentTest)){
             if(strncmp("abbab", argumentTest.value, argumentTest.valueLength) == 0){
                ret = true;
             }
        }
    }

    if(argumentParser_contains(&parser, &argumentImport)){
        if(!ARGUMENT_PARSER_ARGUMENT_HAS_VALUE(argumentImport)){
            ret = true;
        }
    }

label_free:
    argumentParser_free(&parser);

    return ret;
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
    linkedList_initIterator(&it, list);

    uint_fast64_t i = 0;
    while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        buf[i++] = *(uint8_t*) LINKED_LIST_ITERATOR_NEXT(&it);
    }

    bool ret = true;
    if(buf[0] != 3 || buf[1] != 2 || buf[2] != 1 || buf[3] != 0){
        ret = false;
    }

    return ret;
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

    bool ret = true;
    if(buf[0] != 3 || buf[1] != 2){
        ret = false;
    }

    if(list->length != 2){
        ret = false;
    }

    return ret;
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

        if(util_byteArrayTo_uint16(buf) != val){
            return false;
        }
    }

    return true;
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

        if(util_byteArrayTo_uint32(buf) != val){
            return false;
        }
    }

    return true;
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

        if(util_byteArrayTo_uint64(buf) != val){
            return false;
        }
    }

    return true;
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
    
        if(util_byteArrayTo_uint16(util_uint16ToByteArray(buf, val)) != val){
            return false;
        }
    }

    return true;
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
    
        if(util_byteArrayTo_uint32(util_uint32ToByteArray(buf, val)) != val){
            return false;
        }
    }

    return true;
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
    
        if(util_byteArrayTo_uint64(util_uint64ToByteArray(buf, val)) != val){
            return false;
        }
    }

    return true;
}

TEST_TEST_FUNCTION(util_findFirst){
    const char s[] = "abcdef012ghi~jklno~p345~qrs";

    if(util_findFirst(s, strlen(s), '~') != 12){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_findFirst_s){
    const char a[] = "aabbccdd--11223344";

    const int_fast64_t offset = util_findFirst_s(a, strlen(a), "--", 2);

    if(offset != 8){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_findLast){
    const char a[] = "01234...567";
    const char b[] = "01234567";

    if(util_findLast(a, strlen(a), '.') != 7){
        return false;
    }

    if(util_findLast(b, strlen(b), '.') != -1){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_replace){
    char s[] = "--$errorCode--$errorCode--";
    char a[64] = {0};

    uint_fast64_t stringLength = strlen(s);
    
    memcpy(a, s, stringLength + 1);

    util_replace(a, 64, &stringLength, "$errorCode", strlen("$errorCode"), "404", strlen("404"));
    
    if(strcmp(a, "--404--404--") != 0){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_getBaseDirectory){
    // Test_1.
    {
        char url[] = "/";

        char* baseDirectory;
        uint_fast64_t baseDirectoryLength;
        if(util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url)) != ERROR_NO_ERROR){
            return false;
        }

        if(baseDirectoryLength != 1){
            return false;
        }

        if(strncmp(url, baseDirectory, baseDirectoryLength) != 0){
            return false;
        }
    }

    // Test_2.
    {
        char url[] = "/extractShowInfo";
        
        char* baseDirectory;
        uint_fast64_t baseDirectoryLength;
        if(util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url)) != ERROR_NO_ERROR){
            return false;
        }

        if(baseDirectoryLength != 16){
            return false;
        }

        if(strncmp(url, baseDirectory, baseDirectoryLength) != 0){
            return false;
        }
    }

    // Test_3.
    {
        char url[] = "/img/img_001.png";

        char* baseDirectory;
        uint_fast64_t baseDirectoryLength;
        if(util_getBaseDirectory(&baseDirectory, &baseDirectoryLength, url, strlen(url)) != ERROR_NO_ERROR){
            return false;
        }

        if(baseDirectoryLength != 4){
            return false;
        }

        if(strncmp("/img", baseDirectory, baseDirectoryLength) != 0){
            return false;
        }
    }

    return true;
}

TEST_TEST_FUNCTION(util_trim){
    char s[] = "  123_457 8910  ";

    util_trim(s, strlen(s));

    if(strcmp(s, "123_457 8910") != 0){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_getFileName){
    char filePath[] = "~/Video/video_001.mkv";

    const char* fileName = util_getFileName(filePath, strlen(filePath));

    if(strcmp(fileName, "video_001.mkv") != 0){
        return false;
    }

    return true;
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
            return false;
        }
    }

    return true;
}

TEST_TEST_FUNCTION(util_toLowerChase){
    char s[] = "AbCDEFg0123";
    
    util_toLowerChase(s);

    if(strncmp(s, "abcdefg0123", strlen(s)) != 0){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_replaceAllChars){
    char a[] = "010101110101";

    util_replaceAllChars(a, '1', '0');

    const char b[] = "000000000000";

    if(strncmp(a, b, strlen(b)) != 0){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_append){
    char a[11] = {'1', '2', '3', '4', '5'};
    char b[] = "67890";
    
    util_append(a, strlen(a), b, strlen(b));

    if(strncmp(a, "1234567890", 11) != 0){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(util_stringToInt){
    const char a[] = "27";
    const char b[] = "a88b";

    int_fast64_t value;
    if(util_stringToInt(a, &value) != ERROR_NO_ERROR){
        return false;
    }

    if(value != 27){
        return false;
    }

    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_INVALID_STRING);
    if(util_stringToInt(b, &value) != ERROR_INVALID_STRING){
        return false;
    }

    if(value != 0){
        return false;
    }

    return true;
}

// http.c
TEST_TEST_FUNCTION(http_addHeaderField){
    #define BUFFER_SIZE 8192

    void* buffer;
    if(util_blockAlloc(&buffer, BUFFER_SIZE) != ERROR_NO_ERROR){
        goto label_free;
    }

    HTTP_Request request;
    http_initRequest_(&request, buffer, BUFFER_SIZE, 0);

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD(request, Connection, connection);
   
    const HTTP_HeaderField* headerFieldConnection = http_getHeaderField(&request, "Connection");
    
    bool ret = false;
    if(headerFieldConnection == NULL){
        goto label_free;
    }

    if(strncmp("close", headerFieldConnection->value, headerFieldConnection->valueLength) == 0){
        ret = true;
    }

label_free:
    http_freeHTTP_Request(&request);

    util_unMap(buffer, BUFFER_SIZE);
    #undef BUFFER_SIZE

    return ret;
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

    bool ret = true;
    if(*((int*)que_deque(&que)) != 0){
        ret = false;
    }

    if(*((int*)que_deque(&que)) != 1){
        ret = false;
    }

    if(*((int*)que_deque(&que)) != 2){
        ret = false;
    }

    que_clear(&que);

    return ret;
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

    bool ret = true;
    if(que_deque(&que) != NULL){
        ret = false;
    }

    return ret;
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

    if(pthread_mutex_init(&counterLock, NULL)){
        UTIL_LOG_ERROR("Failed to initialise 'pthread_mutex'.");   
     
        return false;
    }

    if(sem_init(&threadSync, 0, 0)){
        UTIL_LOG_ERROR("Failed to initialise 'semaphore'.");

        return false;
    }

    threadPool_run(&threadPool, test_threadPoolRunner, NULL);
    threadPool_run(&threadPool, test_threadPoolRunner, NULL);
    threadPool_run(&threadPool, test_threadPoolRunner, NULL);
    threadPool_run(&threadPool, test_threadPoolRunner, NULL);

    uint_fast64_t i;
    for(i = 4; i > 0; i--){
        sem_wait(&threadSync);
    }

    bool ret = false;
    if(counter == 40){
        ret = true;
    }

    pthread_mutex_destroy(&counterLock);

    sem_destroy(&threadSync);

    threadPool_free(&threadPool);

    return ret;
}

// cache.c
TEST_TEST_FUNCTION(cache_load){
    #define TEST_FILE_NAME "/tmp/herder_cache_test_file_XXXXXX"

    // TODO:(jan) Replace with temporary file.
    char* filePath = malloc(sizeof(*filePath) * (strlen(TEST_FILE_NAME) + 1));
    strcpy(filePath, TEST_FILE_NAME);

    #undef TEST_FILE_NAME

    int fileDescriptor = mkstemp(filePath);


    if(fileDescriptor < 1){
        UTIL_LOG_ERROR_("Failed to create temporary file '%s' [%s].", filePath, strerror(errno));

        return false;
    }

    uint8_t* buffer = malloc(sizeof(*buffer) * 256);
    memset(buffer, 8, 64);
    memset(buffer + 64, 16, 64);
    memset(buffer + 128, 8, 64);
    memset(buffer + 192, 32, 64);

    if(write(fileDescriptor, buffer, 256) != 256){
        UTIL_LOG_ERROR("Failed to write media library file version.");

        return false;
    }

    free(buffer);
    
    Cache cache;
    cache_init(&cache, 2, MB(8));

    char symbolicFileLocation[] = "/herderTestFile";

    CacheObject* cacheObject;
    cache_load(&cache, &cacheObject, filePath, strlen(filePath), symbolicFileLocation, strlen(symbolicFileLocation));

    bool ret = true;
    if(cacheObject->size != 256){
        ret = false;
    }

    uint_fast16_t i;
    for(i = 0; i < 256; i++){
        if(i < 64){
            if(cacheObject->data[i] != 8){
                ret = false;

                break;
            }
        }else if(i < 128){
            if(cacheObject->data[i] != 16){
                ret = false;

                break;
            }
        }else if(i < 192){
            if(cacheObject->data[i] != 8){
                ret = false;

                break;
            }
        }else if(i < 256){
            if(cacheObject->data[i] != 32){
                ret = false;

                break;
            }
        }
    }
    
    util_deleteFile(filePath);

    cache_free(&cache);

    return ret;
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
    char currentDir[PATH_MAX];
    util_getCurrentWorkingDirectory(currentDir, PATH_MAX);

    const uint_fast64_t currentDirLength = strlen(currentDir);
    const uint_fast64_t propertyFilePathLength = currentDirLength + 34/*"/tmp/propertyFile_create_test_file"*/;

    char* propertyFilePath = alloca(sizeof(*propertyFilePath) * (propertyFilePathLength + 1));
    strncpy(propertyFilePath, currentDir, currentDirLength);
    strncpy(propertyFilePath + currentDirLength, "/tmp/propertyFile_create_test_file", 35);
    
    if(util_fileExists(propertyFilePath)){
        if(!util_deleteFile(propertyFilePath)){
            UTIL_LOG_ERROR_("Failed to delete old file '%s'.\n", propertyFilePath);       

            return false;
        }
    }

    if(propertyFile_create(propertyFilePath, propertyFilePathLength) != ERROR_NO_ERROR){
        UTIL_LOG_ERROR_("Failed to create property file '%s'.\n", propertyFilePath);                
 
        return false;
    }

    util_deleteFile(propertyFilePath);

    return true;
}

TEST_TEST_FUNCTION_(propertyFile_add, PropertyFile, propertyFile){
    bool ret = true;
    Property* testProperty;
    if(propertyFile_addProperty(propertyFile, &testProperty, "testProperty", sizeof(uint64_t)) != ERROR_NO_ERROR){
        ret =  false;

        goto label_return;
    }

    int8_t* buffer = alloca(sizeof(uint16_t));
    util_uint64ToByteArray(buffer, 19875431121);

    propertyFile_setBuffer(testProperty, buffer);

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    if(propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty") != ERROR_NO_ERROR){
        UTIL_LOG_ERROR("Failed to retrieve property: 'testProperty'.");

        ret =  false;
    }else{
        propertyFile_freeProperty(retrievedProperty);
        free(retrievedProperty);
    }

label_return:
    return ret;
}    

TEST_TEST_FUNCTION_(propertyFile_remove, PropertyFile, propertyFile){
    Property* testProperty;
    propertyFile_addProperty(propertyFile, &testProperty, "testProperty", sizeof(uint64_t));

    int8_t* buffer = alloca(sizeof(uint16_t));
    util_uint64ToByteArray(buffer, 222333444555);

    propertyFile_setBuffer(testProperty, buffer);

    bool ret = true;
    if(propertyFile_removeProperty(testProperty) != ERROR_NO_ERROR){
        ret = false;
    }

    propertyFile_freeProperty(testProperty);
    free(testProperty);

    Property* retrievedProperty;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if(propertyFile_getProperty(propertyFile, &retrievedProperty, "testProperty") != ERROR_ENTRY_NOT_FOUND){
        UTIL_LOG_ERROR("Failed to remove property: 'testProperty'.");

        propertyFile_freeProperty(retrievedProperty);
        free(retrievedProperty);

        ret =  false;
    }

    return ret;
}    

// herder.c
TEST_TEST_FUNCTION(herder_constructFilePath){
    bool ret = false;

    EpisodeInfo info;
    mediaLibrary_initEpisodeInfo(&info);

    char showName[] = "American Dad";
    char episodeName[] = "The last Smith";

    info.showName = showName;
    info.showNameLength = strlen(showName);

    info.name = episodeName;
    info.nameLength = strlen(episodeName);

    info.season = 2;
    info.episode = 14;

    char* path;
    uint_fast64_t pathLength;
    HERDER_CONSTRUCT_FILE_PATH(&path, &pathLength, &info);

    if(strcmp(path, "American_Dad/American_Dad - Season_02/American_Dad_s02e14_The_last_Smith") == 0){
        ret = true;
    }

    return ret;
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
    char showName[] = "American Dad";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* show;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if(medialibrary_getShow(library, &show, showName, showNameLength) == ERROR_NO_ERROR){
        return false;
    }

    if(mediaLibrary_addShow(library, &show, showName, showNameLength) != ERROR_NO_ERROR){
        return false;
    }

    if(medialibrary_getShow(library, &show, showName, showNameLength) != ERROR_NO_ERROR){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION_(mediaLibrary_getSeason, MediaLibrary, library){    
    char showName[] = "American Dad";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* americanDad;
    if(mediaLibrary_addShow(library, &americanDad, showName, showNameLength) != ERROR_NO_ERROR){
        return false;
    }

    Season* season_1;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if(medialibrary_getSeason(americanDad, &season_1, 1) != ERROR_ENTRY_NOT_FOUND){
        return false;
    }

    if(mediaLibrary_addSeason(library, &season_1, americanDad, 1) != ERROR_NO_ERROR){
        return false;
    }

    if(medialibrary_getSeason(americanDad, &season_1, 1) != ERROR_NO_ERROR){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION_(mediaLibrary_addEpisode, MediaLibrary, library){
    char showName[] = "American Dad";
    const uint_fast64_t showNameLength = strlen(showName);

    Show* americanDad;
    if(mediaLibrary_addShow(library, &americanDad, showName, showNameLength) != ERROR_NO_ERROR){
        return false;
    }

    Season* season_01;
    if(mediaLibrary_addSeason(library, &season_01, americanDad, 1) != ERROR_NO_ERROR){
        return false;
    }

    Episode* episode;
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
    if(mediaLibrary_getEpisode(season_01, &episode, 15) != ERROR_ENTRY_NOT_FOUND){
        return false;
    }

    char episdoeName[] = "Threat Levels";
    const uint_fast64_t episodeNameLength = strlen(episdoeName);

    if(mediaLibrary_addEpisode(library, &episode, americanDad, season_01, 15, episdoeName, episodeNameLength, ".mkv", 4, true) != ERROR_NO_ERROR){
        return false;
    }

    if(mediaLibrary_getEpisode(season_01, &episode, 15) != ERROR_NO_ERROR){
        return false;
    }

    return true;
}

TEST_TEST_FUNCTION(mediaLibrary_extractShowName){
    bool ret = true;

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

    EpisodeInfo info = {0};

    char fileName_0[] = "American_Dad_Rogers_Spot";
    if(mediaLibrary_extractShowName(&info, &shows, fileName_0, strlen(fileName_0)) != ERROR_NO_ERROR){
        ret =  false;

        goto label_freeEpisodeInfo;
    }

    if(strncmp(info.showName, americanDad.name, americanDad.nameLength + 1) != 0){
        ret = false;

        goto label_freeEpisodeInfo;
    }

    if(strncmp(fileName_0, "Rogers_Spot", strlen("Rogers_Spot") + 1) != 0){
        ret = false;
    }

    mediaLibrary_freeEpisodeInfo(&info);

    memset(&info, 0, sizeof(info));

    char fileName_1[] = "Family_Guy_s01e01_Death_Has_a_Shadow.avi";
    if(mediaLibrary_extractShowName(&info, &shows, fileName_1, strlen(fileName_1)) != ERROR_NO_ERROR){
        ret =  false;

        goto label_freeEpisodeInfo;
    }

    if(strncmp(info.showName, familyGuy.name, familyGuy.nameLength + 1) != 0){
        ret = false;

        goto label_freeEpisodeInfo;
    }

    if(strncmp(fileName_1, "s01e01_Death_Has_a_Shadow.avi", strlen("s01e01_Death_Has_a_Shadow.avi") + 1) != 0){
        ret = false;
    }

    mediaLibrary_freeEpisodeInfo(&info);
    
    memset(&info, 0, sizeof(info));

    char fileName_2[] = "s01e01_Death_Has_a_Shadow.avi";
    __UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_INCOMPLETE);
    if(mediaLibrary_extractShowName(&info, &shows, fileName_2, strlen(fileName_2)) != ERROR_INCOMPLETE){
        ret =  false;

        goto label_freeEpisodeInfo;
    }

    mediaLibrary_freeEpisodeInfo(&info);

    goto label_freeShows;

label_freeEpisodeInfo:
    mediaLibrary_freeEpisodeInfo(&info);

label_freeShows:
    linkedList_free(&shows);

    return ret;
}

TEST_TEST_FUNCTION(mediaLibrary_extractPrefixedNumber){
    bool ret = true;

    char a[] = "American_Dad_s01e02_Threat_Levels.mkv";

    int_fast16_t val;
    if(mediaLibrary_extractPrefixedNumber(a, strlen(a), &val, 's') != ERROR_NO_ERROR){
        ret = false;

        goto label_return;
    }

    if(val != 1){
        ret = false;

        goto label_return;
    }

    if(strncmp(a, "American_Dad_e02_Threat_Levels.mkv", strlen("American_Dad_e02_Threat_Levels.mkv") != 0)){
        ret = false;

        goto label_return;
    }

    if(mediaLibrary_extractPrefixedNumber(a, strlen(a), &val, 'e') != ERROR_NO_ERROR){
        ret = false;

        goto label_return;
    }

    if(val != 2){
        ret = false;

        goto label_return;
    }

    if(strncmp(a, "American_Dad__Threat_Levels.mkv", strlen("American_Dad__Threat_Levels.mkv") != 0)){
        ret = false;

        goto label_return;
    }

label_return:
    return ret;
}

TEST_TEST_FUNCTION(mediaLibrary_extractEpisodeInfo){
    bool ret = true;

    char a[] = "American_Dad_s01e01_Threat_Levels.mkv"; 

    LinkedList shows;
    linkedList_init(&shows);

    const char showName[] = "American Dad";

    Show americanDad;
    medialibrary_initShow(&americanDad, showName, strlen(showName));

    linkedList_add(&shows, &americanDad);

    EpisodeInfo info;
    mediaLibrary_initEpisodeInfo(&info);

    if(mediaLibrary_extractEpisodeInfo(&info, &shows, a, strlen(a)) != ERROR_NO_ERROR){
        ret = false;

        goto label_free;
    }

    if(info.season != 1 || info.episode != 1 || strncmp(showName, info.showName, strlen(showName)) || strncmp("Threat Levels", info.name, strlen("Threat Levels"))){
        ret = false;

        goto label_free;
    }

label_free:
    mediaLibrary_freeEpisodeInfo(&info);

    linkedList_free(&americanDad.seasons);

    free(americanDad.name);

    linkedList_free(&shows);

    return ret;
}

TEST_TEST_FUNCTION(mediaLibrary_sortEpisodes){
    bool ret = true;

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
            ret = false;

            break;
        }
    }

    mediaLibrary_freeEpisode(&s01e04);
    mediaLibrary_freeEpisode(&s01e05);
    mediaLibrary_freeEpisode(&s01e01);
    mediaLibrary_freeEpisode(&s01e03);
    mediaLibrary_freeEpisode(&s01e02);

    linkedList_free(&unsortedEpisdoes);

    return ret;
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
    bool ret = true;    

    ThreadPool threadPool;
    threadPool_init(&threadPool, 1);

    threadPool_run(&threadPool, server_thread, NULL);

    sleep(2);

    #define BUFFER_SIZE 8096
    void* requestBuffer;
    if(util_blockAlloc(&requestBuffer, BUFFER_SIZE) != ERROR_NO_ERROR){
        goto label_free;
    }

    const char url[] = "/test";
    const uint_fast64_t urlLength = strlen(url);

    HTTP_Request request;
    http_initRequest(&request, url, urlLength, requestBuffer, BUFFER_SIZE, HTTP_VERSION_1_1, REQUEST_TYPE_GET);

    const char connection[] = "close";
    HTTP_ADD_HEADER_FIELD(request, Connection, connection);
   
    void* responseBuffer;
    if(util_blockAlloc(&responseBuffer, BUFFER_SIZE) != ERROR_NO_ERROR){
        goto label_free;
    }

    HTTP_Response response;
    http_initResponse(&response, responseBuffer, BUFFER_SIZE);

    int_fast32_t socketFD;
    if(http_openConnection(&socketFD, "localhost", 8888) != ERROR_NO_ERROR){
        goto label_free;
    }

    http_sendRequest(&request, &response, socketFD);
    
label_free:
    http_closeConnection(socketFD);

    http_freeHTTP_Request(&request);

    http_freeHTTP_Response(&response);

    threadPool_free(&threadPool);

    util_unMap(responseBuffer, BUFFER_SIZE);
    util_unMap(requestBuffer, BUFFER_SIZE);
    #undef BUFFER_SIZE

    return ret;
}

// main
int main(void){
    TEST_BEGIN();

    TEST_SUIT_BEGIN("arrayList");
        TEST(arraylist_iteration);
        TEST(arraylist_fixedSizedHeapList);
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
                
        TEST(util_getBaseDirectory);
        TEST(util_getFileName);
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
    TEST_SUIT_END();
    
    TEST_SUIT_BEGIN_(propertyFile);
        __TEST_NO_SETUP__(); TEST(propertyFile_create);
        TEST(propertyFile_add);
        TEST(propertyFile_remove);
    TEST_SUIT_END();   

    TEST_SUIT_BEGIN_(mediaLibrary);
        TEST(mediaLibrary_getShow);
        TEST(mediaLibrary_getSeason);
        TEST(mediaLibrary_addEpisode);
        TEST_NO_SETUP(mediaLibrary_extractShowName);
        TEST_NO_SETUP(mediaLibrary_extractPrefixedNumber);
        TEST_NO_SETUP(mediaLibrary_extractEpisodeInfo);
        TEST_NO_SETUP(mediaLibrary_sortEpisodes);
    TEST_SUIT_END();

    TEST_SUIT_BEGIN("herder");
        TEST(herder_constructFilePath);
    TEST_SUIT_END();

    // TEST_SUIT_BEGIN("server");
        // Note:(jan) Running the server in valgrind takes to long, probably due to the 'accept' call having to be interrupted in the server loop.
        // TEST(server_addContext);
        // Note:(jan) No need to run this, as this was not meant to be an automated test, and needed console output in 'http.c' that is no longer present to be useful.
        //TEST(server_send);
    // TEST_SUIT_END();

    TEST_END();
}