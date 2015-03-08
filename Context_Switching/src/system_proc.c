#include "rtx.h"
#include "uart.h"
#include "system_proc.h"
#include <LPC17xx.h>
#include <system_LPC17xx.h>
#ifdef DEBUG_0
#include "printf.h"

#endif /* DEBUG_0 */

char crt_buffer[32];
PROC_INIT g_system_procs[NUM_SYSTEM_PROCS];

void set_system_procs() {	
	int i;
	for( i = 0; i < NUM_SYSTEM_PROCS; i++ ) {
		g_system_procs[i].m_priority=0;
		g_system_procs[i].m_stack_size=0x100;
	}
	
	g_system_procs[0].mpf_start_pc = &a_process;
	g_system_procs[0].m_pid=A_PID;
	
	g_system_procs[1].mpf_start_pc = &b_process;
	g_system_procs[1].m_pid=B_PID;
	
	g_system_procs[2].mpf_start_pc = &c_process;
	g_system_procs[2].m_pid=C_PID;
	
	g_system_procs[3].mpf_start_pc = &set_priority_process;
	g_system_procs[3].m_pid=SET_PRIORITY_PID;
	
	g_system_procs[4].mpf_start_pc = &clock_process;
	g_system_procs[4].m_pid=CLOCK_PID;
	
	g_system_procs[5].mpf_start_pc = &kcd_process;
	g_system_procs[5].m_pid=KCD_PID;
	
	g_system_procs[6].mpf_start_pc = &crt_process;
	g_system_procs[6].m_pid=CRT_PID;
}

//system processes 
void kcd_process(void){
	char* buffer = (char*)k_request_memory_block();
	int index = 0;
	
	while (1) {
		int sender;
		MSG_BUF* msg = (MSG_BUF*) receive_message(&sender);			
		send_message(CRT_PID, msg);
		buffer[index] = (msg->mtext)[0];
		index++;
		if (index > 32) index = 0;
		release_processor();
	}	
}



void crt_process(void){ 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
	while (1) {
		int i;
		int sender;
		MSG_BUF* msg = (MSG_BUF*) receive_message(&sender);
		
		for(i = 0; i<31; i++)
			if ((msg->mtext)[i] != '\0')
				crt_buffer[i] = (msg->mtext)[i];
		crt_buffer[i] = '\0';
		release_memory_block(msg);
			
		pUart->IER = IER_THRE | IER_RLS | IER_RBR;			
		release_processor();
	}
}

void a_process(void) {}
void b_process(void) {}
void c_process(void) {}
void set_priority_process(void) {}
void clock_process(void) {}