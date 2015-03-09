#include <LPC17xx.h>
#include "kernel_procs.h"
#include "k_process.h"
#include "k_rtx.h"
#include "uart.h"
#include "uart_polling.h"
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
		g_kernel_procs[i].m_priority=0;
		g_kernel_procs[i].m_stack_size=0x100;
	}
	
	g_kernel_procs[0].mpf_start_pc = &timer_i_process;
	g_kernel_procs[0].m_pid = PID_TIMER_IPROC;
	g_kernel_procs[1].mpf_start_pc = &uart_i_process;
	g_kernel_procs[1].m_pid = PID_UART_IPROC;
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
		
		msg_t = (MSG_T*) k_receive_message_t();		
	}
	
	node = timer_head;
	while (node && node->delay <= g_timer_count) {		
		MSG_T* next = node->next;	
		send_message_t(node);						
		node = next;				
	}	
	
	timer_head = node;		
}

uint8_t g_buffer[]= "You Typed a Q\n\r";
uint8_t *gp_buffer = g_buffer;
uint8_t g_send_char = 0;
uint8_t g_char_in;
uint8_t g_char_out;

uint8_t bBuffer[32];
uint8_t *bPtr;
uint8_t bChar;

void uart_i_process(void) {
	
	uint8_t IIR_IntId;	    // Interrupt ID from IIR 		 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;

	__disable_irq();
	
	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
		MSG_BUF *msg;				
		
		/* read UART. Read RBR will clear the interrupt */
		g_char_in = pUart->RBR;		
		/*************************/	
		msg = (MSG_BUF*)k_request_memory_block();		
		if (msg == NULL) {
			return;
		}
		
		msg->mtype = DEFAULT;
		(msg->mtext)[0] = g_char_in;
		(msg->mtext)[1] = '\0';
		k_send_message(PID_KCD, msg);		
		/*************************/
		g_buffer[12] = g_char_in; // nasty hack
		g_send_char = 1;
		
		
	} else if (IIR_IntId & IIR_THRE) {
	/* THRE Interrupt, transmit holding register becomes empty */
		/*************************/		
		int sender;
		int i;
		MSG_BUF* msg = (MSG_BUF*) k_receive_message_nb(&sender);	
		
		if (msg != NULL) {
			char* msg_str = msg->mtext;
			
			#ifdef _DEBUG_HOTKEYS
			if (msg_str[0] == '!') {
				printf("%c\r\n", msg_str[0]);
				printQ();
			} else if (msg_str[0] == '@') {
				printf("%c\r\n", msg_str[0]);
				printBlockedQ();
			} else if (msg_str[0] == '#') {
				printf("%c\r\n", msg_str[0]);
				printBlockedOnReceiveQ();
			}
			#endif
			
			
			for(i=0; i<31 && msg->mtext[i]; i++)
				bBuffer[i] = msg->mtext[i];
			bBuffer[i] = 0;
			
			k_release_memory_block(msg);
		}
		
		for(bPtr = bBuffer; *bPtr != '\0'; bPtr++)
			pUart->THR = *bPtr;
		/*************************/
		/*if (*gp_buffer != '\0' ) {
			g_char_out = *gp_buffer;
			pUart->THR = g_char_out;
			gp_buffer++;
		} else {*/
			pUart->IER ^= IER_THRE; // toggle the IER_THRE bit 
			pUart->THR = '\0';
			g_send_char = 0;
			gp_buffer = g_buffer;		
		//}
	      
	}
	__enable_irq();	
}



