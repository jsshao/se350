#include "list.h";

void push_back(wat head, mem_block *block) {
	block->next = NULL;
	void runner = *head;
	while(runner->next !=NULL) {
		runner = runner->next;
	}
	runner->next = block;
}

void push_front(Node *head, void *block) {
	block->next = head;
	head = block;
}

void *pop_back(Node *head) {
	if (head == NULL) {
		return NULL;
	}

	void *runner, *returnBlock;

	if (head->next == NULL) {
		runner = *head;
		head = NULL;
		return runner;
	}

	runner = head;
	while (runner->next->next != NULL) {
		runner = runner->next;
	}
	returnBlock = runner->next;
	runner->next = NULL;
}

void *pop_front(Node *head) {
	void block;
	block = head;

	if (block != NULL) {
		block = head->next;
	}

	return block;
}

void remove(Node *head, void *block) {
	if (head == NULL) {
		return;
	}

	void *current, *previous;
	previous = NULL;
	for (current = head; current != NULL; current = current->next) {
		if (current = block) {
			if (previous = NULL) {
				head = current->next;
			} else {
				previous->next = current->next;
			}
			current->next = NULL; ///// how to delete current?
			return;
		}
		previous = current;
	}


}
