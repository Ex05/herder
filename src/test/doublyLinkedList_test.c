#ifndef DOUBLY_LINKED_LIST_TEST_C
#define DOUBLY_LINKED_LIST_TEST_C

#include "../test.c"

TEST_TEST_SUIT_CONSTRUCT_FUNCTION(doublyLinkedList, list){
	*list = calloc(1, sizeof(DoublyLinkedList));
		
	if(*list == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_SUIT_DESTRUCT_FUNCTION(doublyLinkedList, DoublyLinkedList, list){
	doublyLinkedList_free(list);

	free(list);

	return ERROR(ERROR_NO_ERROR);
}

TEST_TEST_FUNCTION_(doublyLinkedList_add, DoublyLinkedList, list){
	uint8_t a = 8;

	DOUBLY_LINKED_LIST_ADD(list, a);

	if(list->head == NULL){
		return TEST_FAILURE("%s", "Failed to add to doublyLinkedList.");
	}

	if(list->length != 1){
		return TEST_FAILURE("%s", "Failed to add to doublyLinkedList.");
	}

	if(list->tail == NULL){
		return TEST_FAILURE("%s", "Failed to add to doublyLinkedList.");
	}

	void* dataPtr = ((&list->head->nextNode) + 1);
	if(*((uint8_t*) dataPtr) != 8){
		return TEST_FAILURE("%s", "Failed to add to doublyLinkedList.");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(doublyLinkedList_iteration, DoublyLinkedList, list){
	uint8_t a = 0;
	uint8_t b = 1;
	uint8_t c = 2;
	uint8_t d = 3;

	DOUBLY_LINKED_LIST_ADD(list, a);
	DOUBLY_LINKED_LIST_ADD_PTR(list, &b);
	DOUBLY_LINKED_LIST_ADD(list, c);
	DOUBLY_LINKED_LIST_ADD_PTR(list, &d);

	uint8_t buf[4] = {0};

	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	uint_fast64_t i = 0;
	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		buf[i++] = *(uint8_t*) DOUBLY_LINKED_LIST_ITERATOR_NEXT(&it);
	}

	if(buf[0] != 0 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3){
		return TEST_FAILURE("%s", "Failed to iterate over doubly linked list.");
	}

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(doublyLinkedList_remove, DoublyLinkedList, list){
	uint8_t a = 0;
	uint8_t b = 1;
	uint8_t c = 2;
	uint8_t d = 3;
	uint8_t e = 33;

	doublyLinkedList_add(list, &a, sizeof(a));
	doublyLinkedList_add(list, &b, sizeof(b));
	doublyLinkedList_add(list, &c, sizeof(c));
	doublyLinkedList_add(list, &d, sizeof(d));

	if(list->length != 4){
		return TEST_FAILURE("Linked list length '%" PRIuFAST64 "' != '%d'.", list->length, 2);
	}

	doublyLinkedList_remove(list, &a, sizeof(a));
	doublyLinkedList_remove(list, &b, sizeof(b));

	__UTIL_SUPPRESS_NEXT_ERROR_OF_TYPE__(ERROR_ENTRY_NOT_FOUND);
	if(doublyLinkedList_remove(list, &e, sizeof(e)) != ERROR_ENTRY_NOT_FOUND){
		return TEST_FAILURE("%s", "Failed to indicate error condition when trying to remove non included element from doubly linked list.");
	}
	
	uint8_t buf[4] = {0};

	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	uint_fast64_t i = 0;
	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		buf[i++] = *(uint8_t*) DOUBLY_LINKED_LIST_ITERATOR_NEXT(&it);
	}

	if(buf[0] != 2 || buf[1] != 3){
	 	return TEST_FAILURE("%s", "Failed to remove entry from doubly linked list.");
	}

	if(list->length != 2){
		return TEST_FAILURE("Doubly linked list length '%" PRIuFAST64 "' != '%d'.", list->length, 2);
	}

	doublyLinkedList_remove(list, &c, sizeof(a));
	doublyLinkedList_remove(list, &d, sizeof(b));

	return TEST_SUCCESS;
}

TEST_TEST_FUNCTION_(doublyLinkedList_contains, DoublyLinkedList, list){
	uint8_t* a = malloc(sizeof(*a));
	uint8_t* b = malloc(sizeof(*b));
	uint8_t* c = malloc(sizeof(*c));
	uint8_t* d = malloc(sizeof(*d));

	*a = 0;
	*b = 1;
	*c = 2;
	*d = 3;

	DOUBLY_LINKED_LIST_ADD_PTR(list, a);
	DOUBLY_LINKED_LIST_ADD_PTR(list, b);
	DOUBLY_LINKED_LIST_ADD_PTR(list, c);
	DOUBLY_LINKED_LIST_ADD_PTR(list, d);

	if(!doublyLinkedList_contains(list, c, sizeof(*c))){
		return TEST_FAILURE("%s", "Failed to find value in doubly linked list.");
	}

	free(a);
	free(b);
	free(c);
	free(d);

	uint8_t e = 4;

	if(doublyLinkedList_contains(list, &e, sizeof(e))){
		return TEST_FAILURE("%s", "False posetive on linked list containing value.");
	}
	
	return TEST_SUCCESS;
}

#endif