#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "util.h"

#define LINKED_LIST_ITERATOR_HAS_NEXT(it) LinkedListIteratorHasNext(it)
#define LINKED_LIST_ITERATOR_NEXT_NODE(it) LinkedListIteratorNextNode(it)
// Note: '+1' offsets the next read to the data segment hidden at the end of the 'LinkedListNode' struct. (Jan - 2022.01.20)
#define LINKED_LIST_ITERATOR_NEXT(it) ((void*) (LinkedListIteratorNextNode(it) + 1))
#define LINKED_LIST_ITERATOR_NEXT_PTR(it, type) (*(type**) LINKED_LIST_ITERATOR_NEXT(it))

#define LINKED_LIST_ADD(list, value) linkedList_add(list, &value, sizeof(value))
#define LINKED_LIST_ADD_PTR(list, value) linkedList_add(list, value, sizeof(*(value)))

#define LINKED_LIST_EMPTY(list) (list->tail == NULL)

#define LINKED_LIST_INIT_NODE(size) ()

struct linkedList_node{
	struct linkedList_node* nextNode;
	// void data;
};

typedef struct linkedList_node LinkedList_Node;

typedef struct{
	LinkedList_Node* tail;
	uint_fast64_t length;
}LinkedList;

typedef struct{
	LinkedList_Node* currentNode;
}LinkedListIterator;

ERROR_CODE linkedList_add(LinkedList*, void*, uint_fast64_t);

void linkedList_addNode(LinkedList*, LinkedList_Node*);

bool linkedList_contains(LinkedList*, void*, uint_fast64_t);

void linkedList_free(LinkedList*);

ERROR_CODE linkedList_remove(LinkedList*, void*, const uint_fast64_t);

void linkedList_initIterator(LinkedListIterator*, LinkedList*);

bool LinkedListIteratorHasNext(LinkedListIterator*);

LinkedList_Node* LinkedListIteratorNextNode(LinkedListIterator*);

#endif