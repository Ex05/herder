#include "arrayList.h"

ARRAY_LIST_EXPAND_FUNCTION(arrayList_defaultExpandFunction){
	// size = previousSize + (previousSize * 2);
	return previousSize + (previousSize >> 1);
}

uint_fast64_t arrayList_expand(const uint_fast16_t i);

local ERROR_CODE arrayList_expandList(ArrayList*);
local void arrayList_init_(ArrayList*, const uint_fast64_t, const uint_fast64_t, ArrayList_ExpandFunction);

inline void arrayList_init_(ArrayList* list, const uint_fast64_t initialSize, const uint_fast64_t stride, ArrayList_ExpandFunction expandFunction){
	list->length = 0;
	list->stride = stride;
	list->expansions = 1;
	list->expandFunction = expandFunction;
	list->maxLength = initialSize;
}

inline ERROR_CODE arrayList_init(ArrayList* list, const uint_fast64_t initialSize, const uint_fast64_t stride, ArrayList_ExpandFunction expandFunction){
	arrayList_init_(list, initialSize, stride, expandFunction);

	list->elements = malloc(stride * list->maxLength);

	if(list->elements == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE arrayList_initFixedSizeList(ArrayList* list, const uint_fast64_t size, const uint_fast64_t stride){
	arrayList_init_(list, size, stride, NULL);

	list->elements = malloc(stride * list->maxLength);

	if(list->elements == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE arrayList_expandList(ArrayList* list){
	list->maxLength = list->expandFunction(list->expansions++, list->maxLength);

	void* elements = realloc(list->elements, list->stride * list->maxLength);
	// TODO:(jan) Check for 'ENOMEM' to see if the realloc call failed.
	if(elements == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}else{
		list->elements = elements;
	}

	return ERROR(ERROR_NO_ERROR);
}

inline void arrayList_initIterator(ArrayListIterator* it, ArrayList* list){
	it->list = list;
	it->index = 0;
}

inline void arrayList_iteratorSetBeginIndex(ArrayListIterator* it, const uint_fast64_t index){
	it->index = index;
}

inline int_fast32_t arrayList_iteratorHasNext(ArrayListIterator* it){
	return it->index < it->list->length;
}

inline void arrayList_free(ArrayList* list){
	free(list->elements);
}