#ifndef DOUBLY_LINKED_LIST_C
#define DOUBLY_LINKED_LIST_C

#include "doublyLinkedList.h"
#include "util.h"

ERROR_CODE doublyLinkedList_add(DoublyLinkedList* list, void* data, uint_fast64_t size){
	DoublyLinkedList_Node* newNode = malloc(sizeof(*newNode) + size);
	if(newNode == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	// This becomes the new tail.
	newNode->nextNode = NULL;
	newNode->previousNode = NULL;

	// Insert data into the node struct after 'nextNode' member.
	memcpy(&newNode->nextNode + 1, data, size);

	// Add newNode to the end of the list.
	doublyLinkedList_addNode(list, newNode);

	return ERROR(ERROR_NO_ERROR);
}

inline void doublyLinkedList_addNode(DoublyLinkedList* list, DoublyLinkedList_Node* newNode){
	// If the list is empty.
	if(list->head == NULL){
		// The new node becomes the head and tail of the list.
		list->head = newNode;
		list->tail = newNode;
	}else{
		// The new node becomes the new tail of the list.
		list->tail->nextNode = newNode;

		newNode->previousNode = list->tail;

		list->tail = newNode;
	}

	list->length += 1;
}

bool doublyLinkedList_contains(DoublyLinkedList* list, void* data, const uint_fast64_t size){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		if(memcmp(DOUBLY_LINKED_LIST_ITERATOR_NEXT(&it), data, size) == 0){
			return true;
		}
	}

	return false;
}

inline void doublyLinkedList_initIterator(DoublyLinkedListIterator* iterator, DoublyLinkedList* list){
	iterator->currentNode = list->head;
}

inline bool doublyLinkedListIteratorHasNext(DoublyLinkedListIterator* it){
	return it->currentNode != NULL;
}

inline DoublyLinkedList_Node* doublyLinkedListIteratorNextNode(DoublyLinkedListIterator* it){
	DoublyLinkedList_Node* node = it->currentNode;
	
	it->currentNode = it->currentNode->nextNode;

	return node;
}

inline void doublyLinkedList_free(DoublyLinkedList* list){
	DoublyLinkedListIterator it;
	doublyLinkedList_initIterator(&it, list);

	while(DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		DoublyLinkedList_Node* node = DOUBLY_LINKED_LIST_ITERATOR_NEXT_NODE(&it);

		free(node);
	}
}

ERROR_CODE doublyLinkedList_remove(DoublyLinkedList* list, void* data, const uint_fast64_t size){
/* This is Linus Torvalds double pointer based approach to removing nodes in linked lists, which compared to
	other approachs (like the one shown below) removes a branch from the code.
	*/
 DoublyLinkedList_Node** node;
	for(node = &list->head; *node;){
		DoublyLinkedList_Node* current = *node;

		if(memcmp(current + 1, data, size) == 0){
			*node = current->nextNode;
			current->previousNode->nextNode = current->nextNode;

			list->length -= 1;

			free(current);

			return ERROR(ERROR_NO_ERROR);
		}else{
			node = &current->nextNode;
		}
	}

	return ERROR(ERROR_ENTRY_NOT_FOUND);
}

#endif