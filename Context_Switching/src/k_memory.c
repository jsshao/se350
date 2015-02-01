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

void* memory[30] = {0}; // addresses of available memory
int flag[30] = {0}; // 0 is ununsed memory block

extern PCB *gp_current_process;
int NUM_MEM_BLOCKS = 30;

void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
  
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_TEST_PROCS * sizeof(PCB *);
  
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB); 
	}
	
#ifdef DEBUG_0  
	printf("gp_pcbs[0] = 0x%x \n", gp_pcbs[0]);
	printf("gp_pcbs[1] = 0x%x \n", gp_pcbs[1]);
#endif
	
	/* prepare for alloc_stack() to allocate memory for stacks */
	
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}

	/* Fixed sized memory pool (max of 30 blocks of 512 bytes each) */
	for (i = 0; i < 30; i++) {
		if ((char *)p_end + i*512 + 512 > (char*)gp_stack) {
			NUM_MEM_BLOCKS = i+1;
			break;
		}
		flag[i] = 0;
		memory[i] = (void *) (p_end + i*512);
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

void *k_request_memory_block(void) {
	int i;
	int available = 0;
#ifdef DEBUG_0 
	printf("k_request_memory_block: entering...\n");
#endif /* ! DEBUG_0 */
	
	while (!available) {
		//check for if there is available memory
		for (i = 0; i < NUM_MEM_BLOCKS; i++) {	
			// available
			if (flag[i] == 0) {
				available = 1;
				break;
			}
		}
		
		//if there is no memory,
		if (!available) {
			printQ();
			gp_current_process->m_state = BLOCKED;
			addBlockedQ(gp_current_process->m_pid, gp_current_process->m_priority);
			printf("releasing processor due to blocked on memory");
			k_release_processor();		
		}		
	}
	
	flag[i] = 1;
	return memory[i];	
}

int k_release_memory_block(void *p_mem_blk) {
	int pid;
	int index;
#ifdef DEBUG_0 
	printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
#endif /* ! DEBUG_0 */
	index = ((char*)p_mem_blk - (char*)memory[0]) / 512;
	if (index >= NUM_MEM_BLOCKS || index < 0) {
		return RTX_ERR;
	}
	
	if (flag[index] == 0) {
		return RTX_ERR;
	} else {
		flag[index] = 0;
	}
	
	pid = popBlockedQ();
	if (pid != -1) {
		gp_pcbs[pid-1]->m_state = RDY;
		addQ(pid, gp_pcbs[pid-1]->m_priority);
	}
	
	return RTX_OK;
}
