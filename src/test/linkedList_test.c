#ifndef LINKED_LIST_TEST_C
#define LINKED_LIST_TEST_C

#include "../test.c"
#include <sys/syslog.h>

TEST_TEST_SUIT_CONSTRUCT_FUNCTION(linkedList, LinkedList, list){
	*list = calloc(1, sizeof(**list));
		
	if(*list == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(linkedList, LinkedList, list){
	linkedList_free(list);

	free(list);

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_FUNCTION_(linkedList_iteration, LinkedList, list){
	uint8_t a = 0;
	uint8_t b = 1;
	uint8_t c = 2;
	uint8_t d = 3;

	LINKED_LIST_ADD(list, a);
	LINKED_LIST_ADD_PTR(list, &b);
	LINKED_LIST_ADD(list, c);
	LINKED_LIST_ADD_PTR(list, &d);

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

	linkedList_add(list, &a, sizeof(a));
	linkedList_add(list, &b, sizeof(b));
	linkedList_add(list, &c, sizeof(c));
	linkedList_add(list, &d, sizeof(d));

if(list->length != 4){
		return TEST_FAILURE("Linked list length '%" PRIuFAST64 "' != '%d'.", list->length, 2);
	}

	linkedList_remove(list, &a, sizeof(a));
	linkedList_remove(list, &b, sizeof(b));
	
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
		return TEST_FAILURE("Linked list length '%" PRIuFAST64 "' != '%d'.", list->length, 2);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(linkedList_addPointer, LinkedList, list){
	typedef struct {
		int a;
		int b;
	}Data;

	Data* data = malloc(sizeof(*data));
	data->a = 10;
	data->b = 99;

	LINKED_LIST_ADD_PTR(list, &data);

	LinkedListIterator it;
	linkedList_initIterator(&it, list);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		Data* data = LINKED_LIST_ITERATOR_NEXT_PTR(&it, Data);

		if(data->a != 10 || data->b != 99){
			return TEST_FAILURE("Error:'%s'.", "Failed to retrieve struct data.");
		}

		free(data);
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(linkedList_contains, LinkedList, list){
	uint8_t* a = malloc(sizeof(*a));
	uint8_t* b = malloc(sizeof(*b));
	uint8_t* c = malloc(sizeof(*c));
	uint8_t* d = malloc(sizeof(*d));

	*a = 0;
	*b = 1;
	*c = 2;
	*d = 3;

	LINKED_LIST_ADD_PTR(list, a);
	LINKED_LIST_ADD_PTR(list, b);
	LINKED_LIST_ADD_PTR(list, c);
	LINKED_LIST_ADD_PTR(list, d);

	if(!linkedList_contains(list, c, sizeof(*c))){
		return TEST_FAILURE("%s", "Failed to find value in linked list.");
	}

	free(a);
	free(b);
	free(c);
	free(d);

	uint8_t e = 4;

	if(linkedList_contains(list, &e, sizeof(e))){
		return TEST_FAILURE("%s", "False posetive on linked list containing value.");
	}
	
	return TEST_SUCCESS;
}

#endif