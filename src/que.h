#ifndef QUE_H
#define QUE_H

#include "util.h"

typedef struct queElement{
	void* value;
	struct queElement* next;
}QueElement;

typedef struct{
		QueElement* head;
		QueElement* tail;
		uint_fast64_t len;
}Que;

ERROR_CODE que_enque(Que*, void*);

void* que_deque(Que*);

void que_clear(Que*);

#endif 