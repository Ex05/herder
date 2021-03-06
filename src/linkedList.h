#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "util.h"

#define LINKED_LIST_IS_EMPTY(list)((list)->length == 0)

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

ERROR_CODE linkedList_add(LinkedList*, void*);

void linkedList_initIterator(LinkedListIterator*, LinkedList*);

Node* linkedList_iteratorNextNode(LinkedListIterator*);

ERROR_CODE linkedList_remove(LinkedList*, void*);

void linkedList_free(LinkedList* list);

#endif

// Index:       Watches:         Directories:
//  0               1               /home
//  1               4               /home/ex05
//  2               8               /home/var
//  3               7               /home/ex05/projects