#ifndef QUE_C
#define QUE_C

#include "que.h"

 scope_local void que_initQueElement(QueElement*, void*);

 scope_local void que_freeQueElement(QueElement*);

inline ERROR_CODE que_enque(Que* que, void* value){
	if(que->tail == NULL){
		QueElement* queElement = malloc(sizeof(*queElement));
		if(queElement == NULL){
			return ERROR(ERROR_OUT_OF_MEMORY);
		}

		que_initQueElement(queElement, value);

		que->tail = que->head = queElement;
	}
	else{
		QueElement* queElement = malloc(sizeof(*queElement));
		if(queElement == NULL){
			return ERROR(ERROR_OUT_OF_MEMORY);
		}

		que_initQueElement(queElement, value);

		QueElement* next = queElement;

		que->head->next = next;
		que->head = next;
	}

	que->len++;

	return ERROR(ERROR_NO_ERROR);
}

inline void* que_deque(Que* que){
	if(que->tail == NULL){
		return NULL;
	}

	QueElement* tail = que->tail;

	void* value = tail->value;

	que->tail = tail->next;

	que_freeQueElement(tail);

	que->len--;

	return value;
}

inline void que_initQueElement(QueElement* queElement, void* value){
	queElement->next = NULL;
	queElement->value = value;
}

inline void que_freeQueElement(QueElement* queElement){
	free(queElement);
}

inline void que_clear(Que* que){
	while(que_deque(que));
}

#endif