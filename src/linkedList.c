#ifndef LINKED_LIST_C
#define LINKED_LIST_C

#include "linkedList.h"

inline ERROR_CODE linkedList_add(LinkedList* list, void * data, uint_fast64_t size){
	LinkedList_Node* node = malloc(sizeof(*node) + size);
	if(node == NULL){
		return ERROR(ERROR_OUT_OF_MEMORY);
	}

	node->nextNode = NULL;
	// Data sits after 'nextNode' pointer in the 'LinkedList_Node' struct. (Jan - 2021.08.29)
	memcpy(&node->nextNode + 1, data, size);

	linkedList_addNode(list, node);

	return ERROR(ERROR_NO_ERROR);
}

inline void linkedList_addNode(LinkedList* list, LinkedList_Node* node){
	if(list->tail == NULL){
		list->tail = node;
	}else{
		node->nextNode = list->tail;

		list->tail = node;
	}

	list->length += 1;
}

inline void linkedList_free(LinkedList* list){
	LinkedListIterator it;
	linkedList_initIterator(&it, list);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		LinkedList_Node* node = LINKED_LIST_ITERATOR_NEXT_NODE(&it);

		free(node);
	}
}

ERROR_CODE linkedList_remove(LinkedList* list, void* data, const uint_fast64_t size){
/* This is Linus Torvalds double pointer based approach to removing nodes in linked lists, which compared to
	other approachs (like the one shown below) removes a branch from the code.
	*/
 LinkedList_Node** node;
	for(node = &list->tail; *node;){
		LinkedList_Node* current = *node;

		if(memcmp(current + 1, data, size) == 0){
			*node = current->nextNode;

			list->length--;

			free(current);

			return ERROR(ERROR_NO_ERROR);
		}else{
			node = &current->nextNode;
		}
	}

	return ERROR(ERROR_ENTRY_NOT_FOUND);
/*
LinkedListIterator it;
linkedList_initIterator(&it, list);
Node* prev = NULL;
while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
	Node* current = LinkedListIteratorNextNode(&it);
	
	if(current->data == data){
		// If the searched element is the only one in the list.
		if(prev == NULL){
			free(list->tail);
			list->length--;
			
			list->tail = NULL;
		}else{
		prev->next = current->next;
					
		list->length--;
					
		free(current);
		}	
	}
	prev = current;
}
*/
}

bool linkedList_contains(LinkedList* list, void* data, const uint_fast64_t size){
	LinkedListIterator it;
	linkedList_initIterator(&it, list);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
		if(memcmp(LINKED_LIST_ITERATOR_NEXT(&it), data, size) == 0){
			return true;
		}
	}

	return false;
}

inline void linkedList_initIterator(LinkedListIterator* iterator, LinkedList* list){
	iterator->currentNode = list->tail;
}

inline bool LinkedListIteratorHasNext(LinkedListIterator* it){
	return it->currentNode != NULL;
}

inline LinkedList_Node* LinkedListIteratorNextNode(LinkedListIterator* it){
	LinkedList_Node* node = it->currentNode;
	
	it->currentNode = it->currentNode->nextNode;

	return node;
}

#endif