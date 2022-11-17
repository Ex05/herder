#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include "util.h"

#define ARRAY_LIST_IS_EMPTY(list)(list->length == 0)
#define ARRAY_LIST_LENGTH(list)((list)->length)

#define ARRAY_LIST_ITERATOR_HAS_NEXT(it)(arrayList_iteratorHasNext(it))

#define ARRAY_LIST_INIT_FIXED_SIZE_STACK_LIST(list, initialSize, stride) do{ \
	arrayList_init_(list, initialSize, stride, NULL);\
	list->elements = alloca(stride * list->maxLength);\
} while(0)

#define ARRAY_LIST_GET(list, index, type) ((type*) (list)->elements)[index]

#define ARRAY_LIST_GET_PTR(list, index, type) &(((type*)(list)->elements)[index])

#define ARRAY_LIST_ADD(list, value, type) do{ \
	if((list)->length == (list)->maxLength){ \
		ERROR_CODE error; \
		if((error = arrayList_expandList((list))) != ERROR_NO_ERROR){ \
			UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error)); \
		} \
	} \
	 \
	 ((type*)(list)->elements)[(list)->length] = value; \
	 (list)->length++; \
} while(0)

#define ARRAY_LIST_ADD_PTR(list, value, type) do{ \
	if((list)->length == (list)->maxLength){ \
		ERROR_CODE error; \
		if((error = arrayList_expandList((list))) != ERROR_NO_ERROR){ \
			UTIL_LOG_CONSOLE(LOG_ERR, util_toErrorString(error)); \
		} \
	} \
	 \
	memcpy(&((type*)(list)->elements)[(list)->length], value, sizeof(type)); \
	(list)->length++; \
	 \
	ERROR(ERROR_NO_ERROR); \
} while(0)

#define ARRAY_LIST_ITERATOR_NEXT(it, type) ARRAY_LIST_GET_PTR((it)->list, (it)->index++, type)

#define ARRAY_LIST_EXPAND_FUNCTION(functionName) uint_fast64_t functionName(const uint_fast16_t numExpansions, const uint_fast64_t previousSize)
typedef ARRAY_LIST_EXPAND_FUNCTION(ArrayList_ExpandFunction);

typedef struct{
	void* elements;
	uint_fast64_t stride;
	uint_fast64_t length;
	uint_fast64_t maxLength;
	uint_fast64_t expansions;
	ArrayList_ExpandFunction* expandFunction;
}ArrayList;

typedef struct{
	ArrayList* list;
	uint_fast64_t index;
}ArrayListIterator;


ERROR_CODE arrayList_initFixedSizeList(ArrayList*, const uint_fast64_t, const uint_fast64_t);

ERROR_CODE arrayList_init(ArrayList*, const uint_fast64_t, const uint_fast64_t, ArrayList_ExpandFunction*);

void arrayList_initIterator(ArrayListIterator*, ArrayList*);

void arrayList_iteratorSetBeginIndex(ArrayListIterator*, const uint_fast64_t);

int_fast32_t arrayList_iteratorHasNext(ArrayListIterator*);

void* arrayList_iteratorNext(ArrayListIterator*);

void arrayList_free(ArrayList*);

#endif