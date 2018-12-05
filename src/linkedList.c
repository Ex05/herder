#ifndef LINKED_LIST_C
#define LINKED_LIST_C

#include "linkedList.h"

local Node* linkedList_initNode(void*);

inline void linkedList_init(LinkedList* list) {
    memset(list, 0, sizeof(*list));
}

inline void linkedList_add(LinkedList* list, void* data) {
    Node* node = linkedList_initNode(data);
    
    if (LINKED_LIST_IS_EMPTY(list)) {
        list->tail = node;
    } else {
        node->next = list->tail;
        list->tail = node;
    }

    list->length++;
}

inline Node* linkedList_initNode(void* data){
    Node* node = malloc(sizeof(*node));
    node->data = data;
    node->next = NULL;

    return node;
}

inline void linkedList_initIterator(LinkedListIterator* it, LinkedList* list) {
    it->node = list->tail;
}

inline Node* linkedList_iteratorNextNode(LinkedListIterator* it){
    Node* node = it->node;

    it->node = node->next;

    return node;
}

inline void linkedList_free(LinkedList* list){
    LinkedListIterator it;
    linkedList_initIterator(&it, list);

	while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
        free(linkedList_iteratorNextNode(&it));
    }
}

inline bool linkedList_remove(LinkedList* list, void* data){
/*  This is Linus Torvalds double Pointer based approach to removing links in LinkedLists, which compared to
    other approachs (like the one shown below) removes a branch from the code.
    */
 Node** node;
    for(node = &list->tail; *node;){
        Node* current = *node;

        if(current->data == data){
            *node = current->next;

            list->length--;

            free(current);

            return true;
        }else{
            node = &current->next;
        }
    }

    return false;

/*
LinkedListIterator it;
linkedList_initIterator(&it, list);

Node* prev = NULL;    
while(LINKED_LIST_ITERATOR_HAS_NEXT(&it)){
    Node* current = linkedList_iteratorNextNode(&it);
    
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

#endif