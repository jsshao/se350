#include "rtx.h"
#include "uart.h"
#include "system_proc.h"
#include <LPC17xx.h>
#include <system_LPC17xx.h>
#ifdef DEBUG_0
#include "printf.h"

#endif /* DEBUG_0 */

PROC_INIT g_system_procs[NUM_SYSTEM_PROCS];


void set_system_procs() {	
	int i;
	for( i = 0; i < NUM_SYSTEM_PROCS; i++ ) {
		g_system_procs[i].m_priority=0;
		g_system_procs[i].m_stack_size=0x100;
	}
	
	g_system_procs[0].mpf_start_pc = &a_process;
	g_system_procs[0].m_pid=PID_A;
	g_system_procs[0].m_priority = LOWEST;
	
	g_system_procs[1].mpf_start_pc = &b_process;
	g_system_procs[1].m_pid=PID_B;
	g_system_procs[1].m_priority = LOWEST;
	
	g_system_procs[2].mpf_start_pc = &c_process;
	g_system_procs[2].m_pid=PID_C;
	g_system_procs[2].m_priority = LOWEST;
	
	g_system_procs[3].mpf_start_pc = &set_priority_process;
	g_system_procs[3].m_pid=PID_SET_PRIO;
	
	g_system_procs[4].mpf_start_pc = &clock_process;
	g_system_procs[4].m_pid=PID_CLOCK;
	
	g_system_procs[5].mpf_start_pc = &kcd_process;
	g_system_procs[5].m_pid=PID_KCD;
	
	g_system_procs[6].mpf_start_pc = &crt_process;
	g_system_procs[6].m_pid=PID_CRT;
}

void null_process(void) {
	while(1) {
			release_processor();
	}
}

//system processes 
void kcd_process(void){
	char* buffer = (char*)request_memory_block();
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
			}						
			release_memory_block(msg);
		} else if (msg->mtype == DEFAULT) {		//single character msg
			char* msg_str = msg->mtext;	
			
			if (index >= 126) {
				index = 0;
				buffer[0] = '\0';
			}

			if(msg->mtext[0] == '\r') {
				msg->mtext[1] = '\n';
				msg->mtext[2] = '\0';
			}
			
			send_message(PID_CRT, msg);
			
			if((msg->mtext[0] == '\b' || msg->mtext[0] == 0x7F) && index != 0) {
				buffer[index] = '\0';
				index--;
			} else if ((msg->mtext[0] == '\b' || msg->mtext[0] == 0x7F) && index == 0) {
				buffer[index] = '\0';
			} else {
				buffer[index] = (msg->mtext)[0];
				buffer[index + 1] = '\0';
				index++;
			}
				
			
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
		//release_processor();
	}	
}

void crt_process(void){ 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
	while (1) {
		int sender;
		MSG_BUF* msg = (MSG_BUF*) receive_message(&sender);
		send_message(PID_UART_IPROC, msg);
		pUart->IER = IER_THRE | IER_RLS | IER_RBR;			
	}
}

void a_process(void) {
	MSG_BUF* reg;
	MSG_BUF* p;
	int num;
	
	//registers command
	reg = (MSG_BUF*)request_memory_block();
	reg->mtype = KCD_REG;
	reg->mtext[0] = '%';
	reg->mtext[1] = 'Z';
	send_message(PID_KCD, reg);
	
	while(1) {
		int sender;
		printf("Please type %%Z to trigger stress test\r\n");
		p = receive_message(&sender);
		set_process_priority(6, LOWEST);
		set_process_priority(PID_B, MEDIUM);
		set_process_priority(PID_C, LOW);
		set_process_priority(PID_A, MEDIUM);
		if(p->mtext[0] == '%' && p->mtext[1] == 'Z') {
			release_memory_block(p);
			break;
		} else {
			release_memory_block(p);
		}
	}
	
	num = 0;
	
	while(1) {
		int sender;
		p = (MSG_BUF*)request_memory_block();
		p->mtype = COUNT_REPORT;
		p->mtext[0] = num;
		send_message(PID_B, p);
		num = (num + 1) % 20;
		release_processor();
	}
}

void b_process(void) {
	int sender;
	while(1) {
		void* msg = receive_message(&sender);
		send_message(PID_C,msg);
	}
}

void c_process(void) {
	MSG_BUF* queue[NUM_MEM_BLOCKS] = {0};
	MSG_BUF* msg;
	int sender;
	int i;

	while (1) {
		if (NULL == queue[0]) {
			msg = (MSG_BUF*)receive_message(&sender);
		} else {
			msg = queue[0];
			for (i = 1; i < NUM_MEM_BLOCKS; i++) {
				queue[i-1] = queue[i];
			}
			queue[NUM_MEM_BLOCKS - 1] = NULL;
		}		
		
		if (msg->mtype == COUNT_REPORT) {
			if (msg->mtext[0] % 20 == 0) {
				MSG_BUF *delay; 

				strcpy(msg->mtext, "Process C\r\n");
				msg->mtype = DEFAULT;
				//release_processor();
				send_message(PID_CRT, msg);

				delay = (MSG_BUF*) request_memory_block();
				
				/* Hibernate */
				delay->mtype = WAKEUP10;
				delayed_send(PID_C, delay, 10000);
				while (1) {
					msg = (MSG_BUF*)receive_message(&sender);
					if (WAKEUP10 == msg->mtype) {
						break; // out of while
					} else {
						for (i = 0; i < NUM_MEM_BLOCKS; i++) {
							if (NULL == queue[i]) {
								queue[i] = msg;
								break;
							}
						}
					}
				}
			}
		}
		
		release_memory_block((void*)msg);
		release_processor();
	}	
}
	
void set_priority_process(void) {
	//register %C command
	MSG_BUF* regMsg = (MSG_BUF*)request_memory_block();	
	regMsg->mtype = KCD_REG;
	regMsg->mtext[0] = '%';
	regMsg->mtext[1] = 'C';
	send_message(PID_KCD, regMsg);
	
	while(1) {
		MSG_BUF* msg;
		int sender;
		int error = 0;
		msg = receive_message(&sender);
		
		if (NULL != msg) {
			char* msg_str = msg->mtext + 3;
			char* token = (char*) strtok(msg_str, " ");
			int pid;
			int priority;
			int ret_val;
			
			//format: %C 12 1 
			pid = atoi(token);
			token = (char *) strtok(NULL, " ");
			priority = atoi(token);
			
			if (msg->mtext[2] != ' ') {
					error = 1;
			} else {
				ret_val = set_process_priority(pid, priority);
				if (-1 == ret_val) {
					error = 1;
				}
			}
			
			if (1 == error ) {
				MSG_BUF* error_msg = (MSG_BUF*) request_memory_block();
				strcpy(error_msg->mtext, "Error - invalid input\r\n");
				error_msg->mtype = DEFAULT;
				send_message(PID_CRT, error_msg);
			}
			
			release_memory_block(msg);
		}
	}
}
	
void clock_process(void) {
	MSG_BUF* reg = (MSG_BUF*)request_memory_block();	
	MSG_BUF* self_msg = (MSG_BUF*)request_memory_block();	
	int state = 0;
	int second = 0;

	//registers command
	reg->mtype = KCD_REG;
	reg->mtext[0] = '%';
	reg->mtext[1] = 'W';
	send_message(PID_KCD, reg);
	
	self_msg->mtype = CLOCK;
	self_msg->mtext[0] = 'S';
	send_message(PID_CLOCK, self_msg);	
	
	while (1) {
		MSG_BUF* msg;
		int sender;
		msg = receive_message(&sender);
		if(sender == PID_CLOCK) {
			if (state == 1) {
				
				int hours = (second/3600) % 24;				
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
				//printf("\r\n%s\r\n", msg->mtext);
				send_message(PID_CRT, msg);
			}
			second++;
			delayed_send(PID_CLOCK, self_msg, 1000);			
		} else {
			char* msg_str = msg->mtext;
			if (msg_str[0] == '%' && msg_str[2] == 'T'){
				state = 0;
			} else if (msg_str[0] == '%' && msg_str[2] == 'S'){
				int error = 1;
				// hh:mm:s  s
				// 45:78:10 11
				if (msg_str[6] != ':' || msg_str[9] != ':' ) {								
				}
				else if ((msg_str[4] < '0') || (msg_str[4] > '9') || (msg_str[5] < '0') || (msg_str[5] > '9') || (msg_str[7] < '0') || (msg_str[7] > '9') || 
					(msg_str[8] < '0') || (msg_str[8] > '9') || (msg_str[10] < '0') || (msg_str[10] > '9') || (msg_str[11] < '0') || (msg_str[11] > '9')) {					
			  } else {
					int tempSecond;
					int hours;
					int t;
					int minutes;
					int seconds; 
					
					tempSecond = (msg_str[4] - '0') * 10 + (msg_str[5] - '0');				
					tempSecond = tempSecond * 60 + (msg_str[7] - '0') * 10 + (msg_str[8] - '0');
					tempSecond = tempSecond * 60 + (msg_str[10] - '0') * 10 + (msg_str[11] - '0');			
					
					hours = tempSecond/3600;
					t = tempSecond%3600;
					minutes = t/60;
					seconds = t%60;
					
					if (hours >= 24 || minutes >= 60 || seconds >= 60 || (msg_str[12] != ' ' && msg_str[12] != '\r' && msg_str[12] != '\n'))  {
					} else {			//valid input
						second = tempSecond;
						state = 1;
						error = 0;
					}
				}
				if (error) {
					#ifdef DEBUG_0
						printf("\r\nError - invalid input\r\n");
					#endif
					}				
			} else if (msg_str[0] == '%' && msg_str[2] == 'R') {			
				state = 1;
				second = 0;
			}
			release_memory_block(msg);
		}		
	}
}