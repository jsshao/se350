#include "list.h"
#include <stdlib.h>

void pushQueue(List *q, void *addr) {	
	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->addr = addr;
	
	if (q->head == NULL && q->tail == NULL) {
		q->tail = newNode;
		q->head = newNode;
	} else {
		q->tail->next = newNode;
		q->tail = newNode;
	}
}

void *popQueue(List *q) {		
	Node* tmp = q->head;
	void* addr = tmp->addr;
	
	if(q->head == NULL)
		return NULL;
	
	q->head = q->head->next;
	
	if (q->head == NULL) {
		q->tail = NULL;
	}
	
	free(tmp);
	
	return addr;
}

