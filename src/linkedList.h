#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "util.h"

#define LINKED_LIST_IS_EMPTY(list)(list->length == 0)

#define LINKED_LIST_ITERATOR_HAS_NEXT(it) ((it)->node != NULL)
#define LINKED_LIST_ITERATOR_NEXT(it)(linkedList_iteratorNextNode(it)->data)

typedef struct node{
    void* data;
    struct node* next;
}Node;

typedef struct {
    Node* tail;
    uint_fast64_t length;
}LinkedList;

typedef struct {
    Node* node;
}LinkedListIterator;

ERROR_CODE linkedList_init(LinkedList*);

void linkedList_add(LinkedList*, void*);

void linkedList_initIterator(LinkedListIterator*, LinkedList*);

Node* linkedList_iteratorNextNode(LinkedListIterator*);

bool linkedList_remove(LinkedList*, void*);

void linkedList_free(LinkedList* list);

#endif