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

void null_process(void) {
	while(1) {
			release_processor();
	}
}

//system processes 
void kcd_process(void){
	char* buffer = (char*)k_request_memory_block();
	char commands[NUM_PROCS];
	int index = 0;
	
	int k;
	for(k = 0; k<NUM_PROCS; k++)
		commands[k] = '\0';
	
	while (1) {
		int sender;
		MSG_BUF* msg = (MSG_BUF*) receive_message(&sender);
		
		if (msg->mtype == KCD_REG)  {					//registers the command
			char* msg_str = msg->mtext;
			if (msg_str[0] == '%' && msg_str[1] != '\0') {
				commands[sender] = msg_str[1];
				printf("HI THERE: %d\r\n", sender);
			}						
		} else if (msg->mtype == DEFAULT) {		//single character msg
			char* msg_str = msg->mtext;	
			
			if (index >= 126) {
				index = 0;
				buffer[0] = '\0';
			}					
			send_message(CRT_PID, msg);
			buffer[index] = (msg->mtext)[0];
			buffer[index + 1] = '\0';
			index++;
			
			if (msg_str[0] == '\r') {					// if is new line, check if buffer is a valid command
				int i;				
				if (buffer[0] == '%' && buffer[1] != '\0') {
					for (i = 0; i < NUM_PROCS; i++) {
						if (buffer[1] == commands[i]) {				//if is registered command				
							int j;
							//printf("commands[i]: %c buffer[1]: %c
							MSG_BUF* msg = (MSG_BUF*) request_memory_block();
							
							for(j = 0; j< 127 && buffer[j]!='\0'; j++) {								//strcpy
								(msg->mtext)[j] = buffer[j];
									//printf("%d",msg->mtext);						
							}															
							(msg->mtext)[j] = '\0';	
							
							msg->mtype = DEFAULT;
							send_message(i, msg);
						}
					}
				}
				index = 0;
				buffer[0] = '\0';
			}								
		}
		release_memory_block(msg);
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
	
void clock_process(void) {
	MSG_BUF* reg = (MSG_BUF*)k_request_memory_block();	
	MSG_BUF* self_msg = (MSG_BUF*)k_request_memory_block();	
	int state = 0;
	int second = 0;

	//registers command
	reg->mtype = KCD_REG;
	reg->mtext[0] = '%';
	reg->mtext[1] = 'W';
	send_message(KCD_PID, reg);
	
	self_msg->mtype = DEFAULT;
	self_msg->mtext[0] = 'S';
	send_message(CLOCK_PID, self_msg);	
	
	while (1) {
		MSG_BUF* msg;
		int sender;
		msg = receive_message(&sender);
		if(sender == CLOCK_PID) {
			if (state == 1) {
				//print
				//printf("%d \n\r", second);	
				
				int hours = second/3600;
				int t = second%3600;
				int minutes = t/60;
				int seconds = t%60;
				msg->mtext[0] = '0' + (hours/10);
				msg->mtext[1] = '0' + (hours%10);
				msg->mtext[2] = ':';
				msg->mtext[3] = '0' + (minutes/10);
				msg->mtext[4] = '0' + (minutes%10);
				msg->mtext[5] = ':';
				msg->mtext[6] = '0' + (seconds/10);
				msg->mtext[7] = '0' + (seconds%10);
				msg->mtext[8] = '\r';
				msg->mtext[9] = '\n';
				msg->mtext[10] = '\0';
				//msg->mtext = message;
				//printf("%s", msg->mtext);
				send_message(CRT_PID, msg);
			}
			second++;
			delayed_send(CLOCK_PID, self_msg, 1000);			
		} else {
			char* msg_str = msg->mtext;
			if (msg_str[0] == '%' && msg_str[2] == 'T'){
				state = 0;
			} else if (msg_str[0] == '%' && msg_str[2] == 'S'){
				state = 1;
				// hh:mm:ss
				// 45:78:10 11
				second = (msg_str[4] - '0') * 10 + (msg_str[5] - '0');
				second = second * 60 + (msg_str[7] - '0') * 10 + (msg_str[8] - '0');
				second = second * 60 + (msg_str[10] - '0') * 10 + (msg_str[11] - '0');			
			} else if (msg_str[0] == '%' && msg_str[2] == 'R') {			
				state = 1;
				second = 0;
			}
			release_memory_block(msg);
		}		
	}
}