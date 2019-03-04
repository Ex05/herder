#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include "util.h"

#define ARRAY_LIST_IS_EMPTY(list)(list->length == 0)
#define ARRAY_LIST_LENGTH(list)(list->length)

#define ARRAY_LIST_ITERATOR_HAS_NEXT(it)(arrayList_iteratorHasNext(it))
#define ARRAY_LIST_ITERATOR_NEXT(it)(arrayList_iteratorNext(it))

#define ARRAY_LIST_INIT_FIXED_SIZE_HEAP_LIST(list, initialSize) do{ \
    arrayList_init_(list, initialSize, NULL);\
    list->elements = alloca(sizeof(void*) * list->maxLength);\
} while(0)

#define ARRAY_LIST_EXPAND_FUNCTION(functionName) uint_fast64_t functionName(const uint_fast16_t expansions, const uint_fast64_t previousSize)
typedef ARRAY_LIST_EXPAND_FUNCTION(ArrayList_ExpandFunction);

typedef struct{
    void** elements;
    uint_fast64_t length;
    uint_fast64_t maxLength;
    uint_fast16_t expansions;
    ArrayList_ExpandFunction* expandFunction;
}ArrayList;

typedef struct{
    ArrayList* list;
    uint_fast64_t index;
}ArrayListIterator;

ERROR_CODE arrayList_init(ArrayList*, const uint_fast64_t, ArrayList_ExpandFunction*);

ERROR_CODE arrayList_initFixedSizeList(ArrayList*, const uint_fast64_t);

void* arrayList_get(ArrayList*, const uint_fast64_t);

ERROR_CODE arrayList_add(ArrayList*, void*);

void arrayList_initIterator(ArrayListIterator*, ArrayList*);

void arrayList_iteratorSetBeginIndex(ArrayListIterator*, const uint_fast64_t);

int_fast32_t arrayList_iteratorHasNext(ArrayListIterator*);

void* arrayList_iteratorNext(ArrayListIterator*);

void arrayList_free(ArrayList*);

#endif