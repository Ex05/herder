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

/*

    // Remove the show name from the fileName.
    if(mostLikely != NULL){
        info->showName = malloc(sizeof(*info->showName) * (nameLength + 1));
        if(info->showName == NULL){
            return ERROR(ERROR_OUT_OF_MEMORY);
        }

        strncpy(info->showName, showName, nameLength + 1);
        info->showNameLength = nameLength;

        char* beginIndex = strstr(s2, lowerChaseShowName);

        if(beginIndex == NULL){
            if(lowerChaseShowName != NULL){
                beginIndex = strstr(s2, lowerChaseShowName);

                 if(beginIndex != NULL){
                    util_stringCopy(beginIndex, beginIndex + nameLength, strlen(beginIndex) + 1 - nameLength);
                }
            }
        }else{
            util_stringCopy(beginIndex, beginIndex + nameLength, strlen(beginIndex) + 1 - nameLength);
        }
    }else{
        info->showNameLength = 0;
        info->showName = NULL;

        return ERROR(ERROR_INCOMPLETE);
    }

    strncpy(fileName, s2, strlen(s2));

    */