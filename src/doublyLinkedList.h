#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

#include "util.h"

#define DOUBLY_LINKED_LIST_ITERATOR_HAS_NEXT(it) doublyLinkedListIteratorHasNext(it)
#define DOUBLY_LINKED_LIST_ITERATOR_NEXT_NODE(it) doublyLinkedListIteratorNextNode(it)
// Note: '+2' offsets the next read to the data segment hidden at the end of the 'DoublyLinkedList_Node' struct. (Jan - 2022.01.20)
#define DOUBLY_LINKED_LIST_ITERATOR_NEXT(it) ((void*) ((&(doublyLinkedListIteratorNextNode(it)->nextNode)) + 1))
#define DOUBLY_LINKED_LIST_ITERATOR_NEXT_PTR(it, type) (*(type**) DOUBLY_LINKED_LIST_ITERATOR_NEXT(it))

#define DOUBLY_LINKED_LIST_ADD(list, value) doublyLinkedList_add(list, &value, sizeof(value))
#define DOUBLY_LINKED_LIST_ADD_PTR(list, value) doublyLinkedList_add(list, value, sizeof(*(value)))

#define DOUBLY_LINKED_LIST_EMPTY(list) (list->tail == NULL)

#define DOUBLY_LINKED_LIST_NODE_GET_PTR(node, type) (*(type**) (&((node)->nextNode)) + 1)

typedef struct doublyLinkedList_node{
	struct doublyLinkedList_node* previousNode;
	struct doublyLinkedList_node* nextNode;

	// Note: data gets allocated at runtime.
	// void data[];
}DoublyLinkedList_Node;

typedef struct{
	DoublyLinkedList_Node* head;
	DoublyLinkedList_Node* tail;
	uint_fast64_t length;
}DoublyLinkedList;

typedef struct{
	DoublyLinkedList_Node* currentNode;
}DoublyLinkedListIterator;

ERROR_CODE doublyLinkedList_add(DoublyLinkedList*, void*, uint_fast64_t);

void doublyLinkedList_addNode(DoublyLinkedList*, DoublyLinkedList_Node*);

void doublyLinkedList_free(DoublyLinkedList*);

void doublyLinkedList_initIterator(DoublyLinkedListIterator*, DoublyLinkedList*);

bool doublyLinkedListIteratorHasNext(DoublyLinkedListIterator*);

DoublyLinkedList_Node* doublyLinkedListIteratorNextNode(DoublyLinkedListIterator*t);

#endif