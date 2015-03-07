#include "uart_polling.h"
#include "kernel_procs.h"
#include "k_process.h"
#include "k_rtx.h"
#include "kernel_procs.h"
#ifdef DEBUG_0
#include "printf.h"

#endif /* DEBUG_0 */

extern uint32_t g_timer_count;

PROC_INIT g_kernel_procs[NUM_KERNEL_PROCS];

//head and tail of the timeout queue
MSG_T* timer_head = NULL;
MSG_T* timer_tail = NULL; 

void set_kernel_procs() {	
	int i;
	for( i = 0; i < NUM_KERNEL_PROCS; i++ ) {
		g_kernel_procs[i].m_pid=(U32)(i+1 + NUM_TEST_PROCS);
		g_kernel_procs[i].m_priority=0;
		g_kernel_procs[i].m_stack_size=0x100;
	}
	
	g_kernel_procs[0].mpf_start_pc = &timer_i_process;
}

void timer_i_process() {
	MSG_T* it;
	MSG_T* node;	
	MSG_T* temp;
	MSG_T* msg_t = (MSG_T*)k_receive_message_t();
	
	while(msg_t) {				
		msg_t->delay = msg_t->delay + g_timer_count;
		it = timer_head;
		msg_t->next = NULL;
		
		if (NULL == timer_head) {				//0 nodes
			timer_head = msg_t;
			timer_tail = msg_t;
			//msg_t->next = NULL;
		} else if (msg_t->delay < timer_head->delay) {		//one node
			msg_t->next = timer_head;
			timer_head = msg_t;
		} else {									
			while(it->next && it->next->delay < msg_t->delay) {			//more than 1 node
				it = it->next;
			}
			msg_t->next = it->next;
			it->next = msg_t;			
			
			if (it->next == NULL) {
				timer_tail = it;
			}
		}				
		printf("put message on delayed queue for time %d\r\n", msg_t->delay);
		
		msg_t = (MSG_T*) k_receive_message_t();		
	}
	
	node = timer_head;
	while (node && node->delay <= g_timer_count) {		
		MSG_T* next = node->next;
		printf("sending node %d\r\n", node->delay);		
		send_message_t(node);						
		node = next;				
	}
	
	
	timer_head = node;		
}
