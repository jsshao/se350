/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS. 
 *       The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. 
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"
#include "kernel_procs.h"
#include "system_proc.h"
#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */

/* process initialization table */
PROC_INIT g_proc_table[NUM_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];
extern PROC_INIT g_kernel_procs[NUM_KERNEL_PROCS];
extern PROC_INIT g_system_procs[NUM_SYSTEM_PROCS];


/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
 
 //ready queue and blocked queue (each priority has a queue)
int processQueue[5][NUM_PROCS] = {0}; 
int blockedQueue[5][NUM_PROCS] = {0};

int atomic_counter = 0;

void atomic_on() {
	atomic_counter++;
	//if (atomic_counter == 1) {
		__disable_irq();
	//}
}

void atomic_off() {
	atomic_counter--;
	if (atomic_counter <= 0) {
		__enable_irq();
		atomic_counter = 0;
	}
}

void addBlockedQ(int pid, int priority) {
	int i=0;
	for (i = 0; i < NUM_PROCS; i++) {		
		if (blockedQueue[priority][i] == -1) {
			blockedQueue[priority][i] = pid;
			break;
		}
	}
}

void printQ() {
	int i = 0;
	int k = 0;		
	
	printf("Process Ready Queue \r\n");
	for (i = 0; i < 5; i++) {
		for (k = 0; k < NUM_PROCS; k++) {		
			if (processQueue[i][k] == -1) {
				printf("_  ");
			} else if (processQueue[i][k] / 10 >= 1){
				printf("%d ", processQueue[i][k]);
			} else {
				printf("%d  ", processQueue[i][k]);
			}
		}
		printf("\r\n");
	}
}

void printBlockedQ() {
	int i = 0;
	int k = 0;		
	
	printf("Process Blocked Queue \r\n");
	for (i = 0; i < 5; i++) {
		for (k = 0; k < NUM_PROCS; k++) {		
			if (blockedQueue[i][k] == -1) {
				printf("_  ");
			} else if (blockedQueue[i][k] / 10 >= 1){
				printf("%d ", blockedQueue[i][k]);
			} else {
				printf("%d  ", blockedQueue[i][k]);
			}
		}
		printf("\r\n");
	}
}

void printBlockedOnReceiveQ() {
	int k = 0;		
	
	printf("Process Blocked On Receive Queue \r\n");
	for (k = 0; k < NUM_PROCS; k++) {		
		if (gp_pcbs[k]->m_state == BLOCKED_ON_RECEIVE) {
			printf("pid: %d priority: %d \r\n", gp_pcbs[k]->m_pid, gp_pcbs[k]->m_priority);
		}
	}
	printf("\r\n");
}


int popBlockedQ() {
	int i = 0;
	int pid = -1;
	// priority
	for (i = 0; i < 5; i++) {		
		int k;
		if (blockedQueue[i][0] == -1) {
			continue;
		}
		// iterate through and shift
		pid = blockedQueue[i][0];
		for (k = 1; k < NUM_PROCS; k++) {
			blockedQueue[i][k-1] = blockedQueue[i][k];
		}
		
		blockedQueue[i][NUM_PROCS-1] = -1;	
		break;
	}
		
	 return pid;	 
}


void addQ(int pid, int priority) {
	int i = 0;
	
	for (i = 0; i < NUM_PROCS; i++) {		
		if (processQueue[priority][i] == -1) {
			processQueue[priority][i] = pid;
			break;
		}
	}
			
}

int popQ() {
	int i = 0;
	// priority
	for (i = 0; i < 5; i++) {
		int pid;
		int k;
		if (processQueue[i][0] == -1) {
			continue;
		}
		// iterate through and shift
		pid = processQueue[i][0];
		for (k = 1; k < NUM_PROCS; k++) {
			processQueue[i][k-1] = processQueue[i][k];
		}
		
		// Set last pid to -1 
		processQueue[i][NUM_PROCS-1] = -1;

		return pid;	// highest priority proc
	}	
	return -1;
}

//return first element in Q
int peekQ() {
	int i = 0;
	for (i = 0; i < 5; i++) {
		if (processQueue[i][0] == -1) {
			continue;
		}
		return processQueue[i][0];
	}	
	return -1;
}

/** returns the process priority
**/
int k_get_process_priority(int pid) {
	//return -1 if pid is invalid
	if (pid < 0 || pid > NUM_PROCS) {
		return -1;
	}
	
	return gp_pcbs[pid]->m_priority;
}

/** set process priority
**/
int k_set_process_priority(int pid, int priority) {
	int i;
	int j;
	int oldPriority;
	
	//printQ();
	
	if (pid < 1 || pid > NUM_PROCS || priority < 0 || priority > 3) {
		return -1;
	}
	
	oldPriority = (gp_pcbs[pid])->m_priority;
	
	//if setting to the same priority, just return
	if (oldPriority == priority) {
		return 0;
	}

	for (i=0; i<NUM_PROCS; i++) {
		if (processQueue[oldPriority][i] == pid) {
			for (j=i; j<NUM_PROCS-1; j++) {
				processQueue[oldPriority][j] = processQueue[oldPriority][j+1];
			}				
			processQueue[oldPriority][NUM_PROCS-1] = -1;

			addQ(pid, priority);
			break;
		}
	}
	
	for (i=0; i<NUM_PROCS; i++) {
		if (blockedQueue[oldPriority][i] == pid) {
			for (j=i; j<NUM_PROCS-1; j++) {
				blockedQueue[oldPriority][j] = blockedQueue[oldPriority][j+1];
			}
			blockedQueue[oldPriority][NUM_PROCS-1] = -1;
			addBlockedQ(pid, priority);
			break;
		}
	}
	
	(gp_pcbs[pid])->m_priority = priority;

	k_release_processor();
	
	return 0;
}


void process_init() 
{
	int i;
	int j;
	U32 *sp;
  
	for (i = 0; i < 5; i++) {
		for (j = 0; j < NUM_PROCS; j++) {
			processQueue[i][j] = -1;
			blockedQueue[i][j] = -1;
		}
	}
	
  /* fill out the initialization table */
	
	//null process
	g_proc_table[0].m_pid = PID_NULL;
	g_proc_table[0].m_stack_size = 0x100;
	g_proc_table[0].mpf_start_pc = &null_process;
	g_proc_table[0].m_priority = 4;
	addQ(PID_NULL, 4);
	
	//test process
	set_test_procs();
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i + NUM_NULL_PROCS].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i + NUM_NULL_PROCS].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i + NUM_NULL_PROCS].mpf_start_pc = g_test_procs[i].mpf_start_pc;
		g_proc_table[i + NUM_NULL_PROCS].m_priority = g_test_procs[i].m_priority;
		
		addQ(g_proc_table[i + NUM_NULL_PROCS].m_pid, g_proc_table[i + NUM_NULL_PROCS].m_priority);
	}
	
	set_system_procs();
	for ( i = 0; i < NUM_SYSTEM_PROCS; i++ ) {
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS].m_pid = g_system_procs[i].m_pid;
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS].m_stack_size = g_system_procs[i].m_stack_size;
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS].mpf_start_pc = g_system_procs[i].mpf_start_pc;
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS].m_priority = g_system_procs[i].m_priority;						
	}
	addQ(PID_CRT, 0);
	addQ(PID_KCD, 0);
	addQ(PID_CLOCK, 0);
	addQ(PID_SET_PRIO, 0);
	
	addQ(PID_A, g_proc_table[PID_A].m_priority);
	addQ(PID_B, g_proc_table[PID_B].m_priority);
	addQ(PID_C, g_proc_table[PID_C].m_priority);
	
	set_kernel_procs();
	for ( i = 0; i < NUM_KERNEL_PROCS; i++ ) {
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS + NUM_SYSTEM_PROCS].m_pid = g_kernel_procs[i].m_pid;
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS + NUM_SYSTEM_PROCS].m_stack_size = g_kernel_procs[i].m_stack_size;
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS + NUM_SYSTEM_PROCS].mpf_start_pc = g_kernel_procs[i].mpf_start_pc;
		g_proc_table[i + NUM_NULL_PROCS + NUM_TEST_PROCS + NUM_SYSTEM_PROCS].m_priority = g_kernel_procs[i].m_priority;			
	}
  
	//for (i = 0; i <  NUM_KERNEL_PROCS + NUM_TEST_PROCS; i++) {
		//printf("new pid: %d priority: %d \n\r", (g_proc_table[i]).m_pid,(g_proc_table[i]).m_priority); 
		//printf("is null: %d", NULL == gp_pcbs[i]);
	//}
	
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < 16; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;		
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->head = NULL;
		(gp_pcbs[i])->tail = NULL;
		
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
	}
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */



PCB *scheduler(void)
{
	int pid;
	pid = popQ();	
	if(pid == -1)
		return NULL;	
	return gp_pcbs[pid];
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old) 
{
	PROC_STATE_E state;
	
	state = gp_current_process->m_state;
	
	if (state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			//lol
		  if(BLOCKED_ON_RECEIVE != p_pcb_old->m_state)
				p_pcb_old->m_state = RDY;
			
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	} 
	
	/* The following will only execute if the if block above is FALSE */

	if (gp_current_process != p_pcb_old) {
		if (state == RDY){ 
			//frank's non-legit hack
			if(BLOCKED_ON_RECEIVE != p_pcb_old->m_state)
				p_pcb_old->m_state = RDY;
			
			p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			gp_current_process->m_state = RUN;
			__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack    
		} else {			
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		} 
	}
	return RTX_OK;
}
/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */

/*
	the normal flow is to add the running process to the ready priorityQueue, and then
	take the first element in the priorityQueue
*/
int k_release_processor(void)
{		
	PCB *p_pcb_old = gp_current_process;
	
	//printQ();
	
	// UNLESS system just started(gp_current_process is NULL) or current process is blocked
	// add current process to ready queue
	if (gp_current_process != NULL  && gp_current_process->m_state != BLOCKED
			&& gp_current_process->m_state != BLOCKED_ON_RECEIVE) {
		addQ(gp_current_process->m_pid, gp_current_process->m_priority);		
	}
	
	//printQ();
	
	//take first element from ready queue
	gp_current_process = scheduler();

	//printQ();
	
	if (gp_current_process == NULL  ) {		
		return RTX_ERR;
	}
	
	if ( p_pcb_old == NULL ) {
		p_pcb_old = gp_current_process;
	}

	//switch to the new process from the old process
	process_switch(p_pcb_old);
	
	return RTX_OK;
}

/* Send p_msg to the process defined at pid */
int k_send_message(int pid, void *p_msg) {	
	MSG_T* msg;
	
	
	msg = (MSG_T*)k_request_memory_block();
	atomic_on();

	msg->sender_pid = gp_current_process->m_pid;
	msg->dest_pid = pid;	
	msg->msg = p_msg;			
	msg->delay = -1;
	
	//push to the tail of the queue
	msg->next = NULL;				
	if (gp_pcbs[pid]->tail != NULL) {			
		gp_pcbs[pid]->tail->next = msg;
	} else {
		gp_pcbs[pid]->head = msg;
	}
	//assign new tail
	gp_pcbs[pid]->tail = msg;
	
	if ( BLOCKED_ON_RECEIVE == gp_pcbs[pid]->m_state) {
		gp_pcbs[pid]->m_state = RDY;
		addQ(pid, gp_pcbs[pid]->m_priority);			
		if(gp_current_process->m_pid != PID_UART_IPROC) {
			atomic_off();
			k_release_processor();
			atomic_on();
		}
	}
	
	atomic_off();
}

void send_message_t(MSG_T* msg) {
	int pid = msg->dest_pid;
	PCB * dest = gp_pcbs[pid];
	
	atomic_on();
	
 //push to the tail of the queue
	msg->next = NULL;				
	if (gp_pcbs[pid]->tail != NULL) {			
		gp_pcbs[pid]->tail->next = msg;
	} else {
		gp_pcbs[pid]->head = msg;
	}
	//assign new tail
	gp_pcbs[pid]->tail = msg;
	
	if (BLOCKED_ON_RECEIVE == gp_pcbs[pid]->m_state) {
		gp_pcbs[pid]->m_state = RDY;
		addQ(pid, gp_pcbs[pid]->m_priority);	
	}
	
	atomic_off();
}

/* send message to process defined by pid with a delay */
int k_delayed_send(int pid, void *p_msg, int delay) {	
	MSG_T* msg;
		
	msg = (MSG_T*)k_request_memory_block();
	atomic_on();

	msg->sender_pid = gp_current_process->m_pid;
	msg->dest_pid = pid;	
	msg->msg = p_msg;
	msg->delay = delay;
	
	//push to the tail of the queue
	msg->next = NULL;		
	if (gp_pcbs[PID_TIMER_IPROC]->tail != NULL) {			
		gp_pcbs[PID_TIMER_IPROC]->tail->next = msg;
	} else {
		gp_pcbs[PID_TIMER_IPROC]->head = msg;
	}
	//assign new tail
	gp_pcbs[PID_TIMER_IPROC]->tail = msg;		
	
	atomic_off();
}

/* This is a blocking receive */
void *k_receive_message(int *p_pid) {
	int current_pid = gp_current_process->m_pid;	
	void* msg;
	MSG_T * msg_t;
	
	atomic_on();
	
	while (NULL == gp_pcbs[current_pid]->head ||	NULL == gp_pcbs[current_pid]->tail) {
		gp_current_process->m_state = BLOCKED_ON_RECEIVE;		
		atomic_off();
		k_release_processor();		
		atomic_on();
	}
	msg_t = gp_pcbs[current_pid]->head;
	gp_pcbs[current_pid]->head = gp_pcbs[current_pid]->head->next;
	if (gp_pcbs[current_pid]->head == NULL) {
		gp_pcbs[current_pid]->tail = NULL;
	}	
	*p_pid = msg_t->sender_pid;
	msg = msg_t->msg;
	atomic_off();
	k_super_delete((void*)msg_t);		
	
	
	return msg;
}

/* This is a non-blocking receive */
//returns NULL if no memory available
void *k_receive_message_nb(int *p_pid) {
	int current_pid = gp_current_process->m_pid;	
	void* msg_buf;	
	MSG_T * msg_t;
	
	atomic_on();
	
	if (NULL == gp_pcbs[current_pid]->head ||	NULL == gp_pcbs[current_pid]->tail) {
		atomic_off();
		return NULL;
	}
	msg_t = gp_pcbs[current_pid]->head;
	gp_pcbs[current_pid]->head = gp_pcbs[current_pid]->head->next;
	if (gp_pcbs[current_pid]->head == NULL) {
		gp_pcbs[current_pid]->tail = NULL;
	}	
	*p_pid = msg_t->sender_pid;
	msg_buf = msg_t->msg;
	//msg_buf = msg_t->msg;
	
	atomic_off();
	k_super_delete((void*)msg_t);		
	
	return msg_buf;
}


/* non-blocking recieve for delayed messages that returns a MSG_T */
/* pre-cond: expected current_process is the timer interupt process */
void *k_receive_message_t() {
	//deqeue the head
	MSG_T* msg_t = gp_current_process->head;
	
	atomic_on();
	
	if (NULL == msg_t) {
		atomic_off();
		return NULL;				//return null (instead of block) if no msg on queue
	}
	
	gp_current_process->head = gp_current_process->head->next;
	if (gp_current_process->head == NULL) {
		gp_current_process->tail = NULL;
	}
	
	atomic_off();
	
	return msg_t;
}