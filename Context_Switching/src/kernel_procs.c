//#include "rtx.h"
#include "uart_polling.h"
#include "kernel_procs.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"

#endif /* DEBUG_0 */

//head and tail of the timeout queue
MSG_T* timer_head = NULL;
MSG_T* timer_tail = NULL; 

void timer_i_process() {
	printf("tingz");
	
	//call retrieve_message repeatedly until you get a message. while loop???
	
	MSG_T* recieved_msg = receive_message_t(); 
	
	
		//add message to tail of timout queue
	while (recieved_msg != NULL) {
		timer_tail->next = NULL;
		if (timer_tail->next != NULL) {
			timer_tail->next = recieved_msg;
		} else {
			timer_head = recieved_msg;
		}
		timer_tail = recieved_msg;
		
		recieved_msg = receive_message_t(); 
	}		
	
	if ( first message in queue delay expired ) {
		//dequeue this message from the timout queue
		MSG_T* env = timer_head;
		timer_head = timer_head->next;
		if (timer_head == NULL) {
			timer_tail = NULL;
		}
		
		int target_pid = env->dest_pid;
		
		//send message to destination
		send_message(target_pid, env);
	}
} 