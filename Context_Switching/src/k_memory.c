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

const int NUM_MEM_BLOCKS = 120;
const int MEM_BLOCK_SIZE = 128;
void* memory[NUM_MEM_BLOCKS] = {0}; // addresses of available memory
int flag[NUM_MEM_BLOCKS] = {0}; // 0 is ununsed memory block

extern PCB *gp_current_process;


void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
  
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += (NUM_TEST_PROCS + NUM_KERNEL_PROCS) * sizeof(PCB *);
  
	for ( i = 0; i < NUM_TEST_PROCS + NUM_KERNEL_PROCS; i++ ) {
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

	/* Fixed sized memory pool (max of 120 blocks of 128 bytes each) */
	for (i = 0; i < NUM_MEM_BLOCKS; i++) {
		/*
		if ((char *)p_end + i*512 + 512 > (char*)gp_stack) {
			NUM_MEM_BLOCKS = i+1;
			break;
		} */
		flag[i] = 0;
		memory[i] = (void *) (p_end + i*MEM_BLOCK_SIZE);
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
	int i;
	int available = 0;
	
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
		if (!available) {
			gp_current_process->m_state = BLOCKED;
			addBlockedQ(gp_current_process->m_pid, gp_current_process->m_priority);			
			k_release_processor();		
		}		
	}
	
	flag[i] = gp_current_process->m_pid;
	return memory[i];	
}

/*
	when memory is released, mark that memory block as avaliable
	and remove first element in block queue and put it into ready queue
*/
int k_release_memory_block(void *p_mem_blk) {
	int pid;
	int index;

	// get index of flag array from pointer
	index = ((char*)p_mem_blk - (char*)memory[0]) / MEM_BLOCK_SIZE;
	
	// if index is invalid, return
	if (index >= NUM_MEM_BLOCKS || index < 0) {
		return RTX_ERR;
	}
	
	//set flag array to be avaliable for block at index or if it doesn't belong to the process
	if (flag[index] == 0 || flag[index] != gp_current_process->m_pid) {
		return RTX_ERR;
	} else {
		flag[index] = 0;
	}
	
	
	//remove first process in blockedQ, and check for preemption
	pid = popBlockedQ();
	if (pid != -1) {
		int qPid;
		
		gp_pcbs[pid-1]->m_state = RDY;
		addQ(pid, gp_pcbs[pid-1]->m_priority);
		/*
		qPid = peekQ();
		if(qPid != -1) {
			//if current process has a lower priority than next process in queue
			//not 0 is highest prioty, a>b means a has a LOWER priority
			if(gp_current_process->m_priority > (gp_pcbs[qPid])->m_priority) {
				k_release_processor();
			}
		} */
		k_release_processor();
		
	}
	
	return RTX_OK;
}

int k_super_delete(void *p_mem_blk) {
	int pid;
	int index;

	// get index of flag array from pointer
	index = ((char*)p_mem_blk - (char*)memory[0]) / MEM_BLOCK_SIZE;
	
	// if index is invalid, return
	if (index >= NUM_MEM_BLOCKS || index < 0) {
		return RTX_ERR;
	}
	
	//set flag array to be avaliable for block at index or if it doesn't belong to the process
	if (flag[index] == 0) {
		return RTX_ERR;
	} else {
		flag[index] = 0;
	}	
	
	//remove first process in blockedQ, and check for preemption
	pid = popBlockedQ();
	if (pid != -1) {
		int qPid;		
		gp_pcbs[pid-1]->m_state = RDY;
		addQ(pid, gp_pcbs[pid-1]->m_priority);		
		k_release_processor();
		
	}
	
	return RTX_OK;
}
