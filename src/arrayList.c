#ifndef ARRAY_LIST_C
#define ARRAY_LIST_C

#include "util.h"
#include "arrayList.h"

uint_fast64_t arrayList_expand(const uint_fast16_t i);

local ERROR_CODE arrayList_expandList(ArrayList*);
local void arrayList_initCore(ArrayList*, const uint_fast64_t, ArrayList_ExpandFunction);

inline void arrayList_initCore(ArrayList* list, const uint_fast64_t initialSize, ArrayList_ExpandFunction expandFunction){
    list->length = 0;
    list->expansions = 1;
    list->expandFunction = expandFunction;
    list->maxLength = initialSize;
}

inline ERROR_CODE arrayList_init(ArrayList* list, const uint_fast64_t initialSize, ArrayList_ExpandFunction expandFunction){    
    arrayList_initCore(list, initialSize, expandFunction);

    list->elements = malloc(sizeof(void*) * list->maxLength);

    if(list->elements == NULL){
       return ERROR(ERROR_OUT_OF_MEMORY);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE arrayList_initFixedSizeList(ArrayList* list, const uint_fast64_t size){
    arrayList_initCore(list, size, NULL);

    list->elements = malloc(sizeof(void*) * list->maxLength);

    if(list->elements == NULL){
       return ERROR(ERROR_OUT_OF_MEMORY);
    }

    return ERROR(ERROR_NO_ERROR);
}

inline ERROR_CODE arrayList_expandList(ArrayList* list){
    list->maxLength = list->expandFunction(list->expansions++, list->maxLength);

    void** elements = realloc((void*) list->elements, sizeof(void*) * list->maxLength);

    // TODO:(jan) Check for 'ENOMEM' to see if the realloc call failed.
    if(elements == NULL){
        return ERROR(ERROR_OUT_OF_MEMORY);
    }else{
        list->elements = elements;
    }
   
   return ERROR(ERROR_NO_ERROR);
}

inline void* arrayList_get(ArrayList* list, const uint_fast64_t i){
    return list->elements[i];
}

inline ERROR_CODE arrayList_add(ArrayList* list, void* value){
    if(list->length < list->maxLength){
        list->elements[list->length++] = value;
    }else{
        if(list->expandFunction != NULL){
            const ERROR_CODE error = arrayList_expandList(list);           

            if(error != ERROR_NO_ERROR){
                return error;
            }

            list->elements[list->length++] = value;
        }else{
            return ERROR(ERROR_TO_MANY_ELEMENTS);
        }
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

inline void* arrayList_iteratorNext(ArrayListIterator* it){
    return arrayList_get(it->list, it->index++);
}

inline void arrayList_free(ArrayList* list){        
    free(list->elements);    
}

#endif