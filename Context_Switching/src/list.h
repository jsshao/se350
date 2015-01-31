#ifndef LIST_H_
#define LIST_H

struct Node{
	void *mem_block;
	Node *next;
};

void push_back(pee head, poo *block);
void push_front(pee head, poo *block);
void *pop_back(pee head);
void *pop_front(pee head);
void remove(pee head, poo *block);