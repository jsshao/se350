/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_memory.h"
#include "list.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|  
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|          
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

//const int NUM_MEM_BLOCKS = 60;
const int MEM_BLOCK_SIZE = 128;

const int MEM_BLOCK_SIZE_ENV = 40;
int flag_env[NUM_MEM_BLOCKS] = {0}; // 0 is ununsed memory block
void* memory_env[NUM_MEM_BLOCKS] = {0};

void* memory[NUM_MEM_BLOCKS] = {0}; // addresses of available memory
int flag[NUM_MEM_BLOCKS] = {0}; // 0 is ununsed memory block


/* Debug variable to keep track of memory leaks */
int memory_block_count = 0;

extern PCB *gp_current_process;


void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
  
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += (NUM_PROCS) * sizeof(PCB *);
  
	for ( i = 0; i < NUM_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		gp_pcbs[i]->head = NULL;
		gp_pcbs[i]->tail = NULL;

		p_end += sizeof(PCB); 
	}
	
	/* prepare for alloc_stack() to allocate memory for stacks */
	
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}

	/* Fixed sized memory pool*/
	for (i = 0; i < NUM_MEM_BLOCKS; i++) {
		flag[i] = 0;
		
		if ((void*)(p_end + i*MEM_BLOCK_SIZE) > gp_stack) {
			printf("Trying to allocate too much memory \r\n");
			break;	
		}
		else {
			memory[i] = (void *) (p_end + i*MEM_BLOCK_SIZE);
		}
	}
	
	//memory for envelopes
	for (i = 0; i < NUM_MEM_BLOCKS; i++) {
		flag_env[i] = 0;
		
		if ((void*)(p_end + i*MEM_BLOCK_SIZE_ENV + MEM_BLOCK_SIZE * NUM_MEM_BLOCKS) > gp_stack) {
			printf("Trying to allocate too much memory ENVELOPES \r\n");
			break;	
		}
		else {
			memory_env[i] = (void *) (MEM_BLOCK_SIZE * NUM_MEM_BLOCKS + p_end + i*MEM_BLOCK_SIZE_ENV);
		}
	}	
}

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b) 
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */
	
	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);
	
	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) { //1111 & 0100
		--gp_stack; 
	}
	return sp;
}

/*
	while memory is not avaliable, add current process to 
	blocked queue and release processor
*/

void *k_request_memory_block(void) {
	int i, j;
	int available = 0;

	atomic_on();
	
	while (!available) {
		//check for if there is available memory
		for (i = 0; i < NUM_MEM_BLOCKS; i++) {	
			// available
			if (flag[i] == 0) {
				available = 1;
				break;
			}
		}
		
		//if there is no memory, add current process to blocked queue, and release processor
		if (!available && gp_current_process->m_pid != PID_UART_IPROC) {
			gp_current_process->m_state = BLOCKED;
			addBlockedQ(gp_current_process->m_pid, gp_current_process->m_priority);	
			atomic_off();			
			k_release_processor();		
			atomic_on();
		} else if (!available && gp_current_process->m_pid == PID_UART_IPROC) {
			atomic_off();
			return NULL;
		}
	}
	
	flag[i] = gp_current_process->m_pid;
	
	atomic_off();

	//printf("------------------------------\r\n");	
	memory_block_count = 0;
	for (j = 0; j < NUM_MEM_BLOCKS; j++) {
		if (flag[j] == 0)
			memory_block_count++;
		else {
			//printf("%d has a memory block\r\n", flag[j]);
		}
	}
	//printf("------------------------------\r\n");
	
	return memory[i];	
}


/** 
requests memory for envelope
**/
void* k_request_memory_env(void) {
	int i, j;
	int available = 0;

	atomic_on();
	
	while (!available) {
		//check for if there is available memory
		for (i = 0; i < NUM_MEM_BLOCKS; i++) {	
			// available
			if (flag_env[i] == 0) {
				available = 1;
				break;
			}
		}
		
		//if there is no memory, add current process to blocked queue, and release processor
		if (!available && gp_current_process->m_pid != PID_UART_IPROC) {
			gp_current_process->m_state = BLOCKED_ON_ENV;
			addBlockedQ(gp_current_process->m_pid, gp_current_process->m_priority);	
			atomic_off();			
			k_release_processor();		
			atomic_on();
		} else if (!available && gp_current_process->m_pid == PID_UART_IPROC) {
			atomic_off();
			return NULL;
		}
	}
	
	flag_env[i] = gp_current_process->m_pid;
	
	atomic_off();
	
	return memory_env[i];	
}

/*
	when memory is released, mark that memory block as avaliable
	and remove first element in block queue and put it into ready queue
*/
int k_release_memory_block(void *p_mem_blk) {
	int pid;
	int j;
	int index;
	
	atomic_on();

	// get index of flag array from pointer
	index = ((char*)p_mem_blk - (char*)memory[0]) / MEM_BLOCK_SIZE;
	
	// if index is invalid, return
	if (index >= NUM_MEM_BLOCKS || index < 0) {
		atomic_off();
		return RTX_ERR;
	}
	
	//set flag array to be avaliable for block at index or if it doesn't belong to the process
	if (flag[index] == 0) { //|| flag[index] != gp_current_process->m_pid) {
		atomic_off();
		return RTX_ERR;
	} else {
		flag[index] = 0;
	}
	
	
	//remove first process in blockedQ, and check for preemption
	pid = popBlockedQ();
	if (pid != -1) {
		int qPid;
		
		gp_pcbs[pid]->m_state = RDY;
		addQ(pid, gp_pcbs[pid]->m_priority);
		/*
		qPid = peekQ();
		if(qPid != -1) {
			//if current process has a lower priority than next process in queue
			//not 0 is highest prioty, a>b means a has a LOWER priority
			if(gp_current_process->m_priority > (gp_pcbs[qPid])->m_priority) {
				k_release_processor();
			}
		} */
		atomic_off();
		if (gp_current_process->m_pid != PID_UART_IPROC && gp_current_process->m_pid != PID_CLOCK) {
			k_release_processor();
		}
		atomic_on();

	}
	
	atomic_off();
	
	//printf("------------------------------\r\n");
	memory_block_count = 0;
	for (j = 0; j < NUM_MEM_BLOCKS; j++) {
		if (flag[j] == 0)
			memory_block_count++;
		else {
			//printf("%d has a memory block\r\n", flag[j]);
		}
	}
	//printf("------------------------------\r\n");
	
	return RTX_OK;
}


/*
	release env memory
*/
int k_release_memory_env(void *p_mem_blk) {
	int pid;
	int j;
	int index;
	
	atomic_on();

	// get index of flag array from pointer
	index = ((char*)p_mem_blk - (char*)memory_env[0]) / MEM_BLOCK_SIZE_ENV;
	
	// if index is invalid, return
	if (index >= NUM_MEM_BLOCKS || index < 0) {
		atomic_off();
		return RTX_ERR;
	}
	
	//set flag array to be avaliable for block at index or if it doesn't belong to the process
	if (flag_env[index] == 0) { //|| flag[index] != gp_current_process->m_pid) {
		atomic_off();
		return RTX_ERR;
	} else {
		flag_env[index] = 0;
	}
	
	
	//remove first process in blockedQ, and check for preemption
	pid = popBlockedEnvQ();
	if (pid != -1) {
		int qPid;
		
		gp_pcbs[pid]->m_state = RDY;
		addQ(pid, gp_pcbs[pid]->m_priority);
		
		atomic_off();
		if (gp_current_process->m_pid != PID_UART_IPROC && gp_current_process->m_pid != PID_CLOCK) {
			k_release_processor();
		}
		atomic_on();

	}
	
	atomic_off();

	return RTX_OK;
}
