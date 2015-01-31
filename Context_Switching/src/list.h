#ifndef LIST_H_
#define LIST_H_

typedef struct Node Node;

struct Node{
	void *addr;
	Node *next;
};

typedef struct {
	Node *head;
	Node *tail;
} List;

void pushQueue(List *q, void *block);
void *popQueue(List *q);


#endif
