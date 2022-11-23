#ifndef DOUBLY_LINKED_LIST_TEST_C
#define DOUBLY_LINKED_LIST_TEST_C

#include "../test.c"
#include <stdint.h>
#include <sys/syslog.h>

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
		return TEST_FAILURE("%s", "Failed to iterate over linked list.");
	}

	return TEST_SUCCESS;
}

#endif