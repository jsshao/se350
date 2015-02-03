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

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */

/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];


/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
 
 
int processQueue[5][NUM_TEST_PROCS] = {0}; 
int blockedQueue[5][NUM_TEST_PROCS] = {0};

void addBlockedQ(int pid, int priority) {
	int i=0;
	int j = 0;
	for (i = 0; i < NUM_TEST_PROCS; i++) {		
		if (blockedQueue[priority][i] == -1) {
			blockedQueue[priority][i] = pid;
			break;
		}
	}
}

int popBlockedQ() {
	int i = 0;
	int pid = -1;
	int k  =0;
	// priority
	for (i = 0; i < 5; i++) {		
		int k;
		if (blockedQueue[i][0] == -1) {
			continue;
		}
		// iterate through and shift
		pid = blockedQueue[i][0];
		for (k = 1; k < NUM_TEST_PROCS; k++) {
			blockedQueue[i][k-1] = blockedQueue[i][k];
		}
		
		blockedQueue[i][NUM_TEST_PROCS-1] = -1;	
		break;
	}
		
	 return pid;	 
}


void addQ(int pid, int priority) {
	int i = 0;
	int j = 0;
	
	for (i = 0; i < NUM_TEST_PROCS; i++) {		
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
		for (k = 1; k < NUM_TEST_PROCS; k++) {
			processQueue[i][k-1] = processQueue[i][k];
		}
		
		// Set last pid to -1 
		processQueue[i][NUM_TEST_PROCS-1] = -1;

		return pid;	// highest priority proc
	}	
	return -1;
}

/** returns the process priority
**/
int k_get_process_priority(int pid) {
	if (pid < 0 || pid > NUM_TEST_PROCS) {
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
	
	if (pid < 1 || pid > NUM_TEST_PROCS || priority < 0 || priority > 3) {
		return -1;
	}
	
	oldPriority = (gp_pcbs[pid])->m_priority;
	
	//if setting to the same priority, just return
	if (oldPriority == priority) {
		return 0;
	}

	for (i=0; i<NUM_TEST_PROCS; i++) {
		if (processQueue[oldPriority][i] == pid) {
			for (j=i; j<NUM_TEST_PROCS-1; j++) {
				processQueue[oldPriority][j] = processQueue[oldPriority][j+1];
			}				
			processQueue[oldPriority][NUM_TEST_PROCS-1] = -1;

			addQ(pid, priority);
			break;
		}
	}
	
	for (i=0; i<NUM_TEST_PROCS; i++) {
		if (blockedQueue[oldPriority][i] == pid) {
			for (j=i; j<NUM_TEST_PROCS-1; j++) {
				blockedQueue[oldPriority][j] = blockedQueue[oldPriority][j+1];
			}
			blockedQueue[oldPriority][NUM_TEST_PROCS-1] = -1;
			addBlockedQ(pid, priority);
			break;
		}
	}
	
	(gp_pcbs[pid])->m_priority = priority;

	return 0;
}


void process_init() 
{
	int i;
	int j;
	U32 *sp;
  
	for (i = 0; i < 5; i++) {
		for (j = 0; j < NUM_TEST_PROCS; j++) {
			processQueue[i][j] = -1;
			blockedQueue[i][j] = -1;
		}
	}
	
  /* fill out the initialization table */
	set_test_procs();
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
		addQ(g_test_procs[i].m_pid, g_test_procs[i].m_priority);
	}
  
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		(gp_pcbs[i])->m_state = NEW;
		
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
	int pid = popQ();	
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
int k_release_processor(void)
{	
	PCB *p_pcb_old = gp_current_process;
	
	if (gp_current_process != NULL  && gp_current_process->m_state != BLOCKED) {
		addQ(gp_current_process->m_pid, gp_current_process->m_priority);		
	}
	gp_current_process = scheduler();

	if (gp_current_process == NULL  ) {		
		return RTX_ERR;
	}
	
	if ( p_pcb_old == NULL ) {
		p_pcb_old = gp_current_process;
	}

	process_switch(p_pcb_old);
	
	return RTX_OK;
}