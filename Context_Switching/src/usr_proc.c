/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/01/17
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include <string.h>

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

/* Keep track of tests passed (This int will be 6 bits in total 
	 where the i'th bit means the i'th test passed). 
	 ie. 0b111111 means all tests passed.
 */
int TEST_BIT_PASSED = 0x0; 
int TOTAL_TESTS_PASSED = 0;
char GROUP_PREFIX[] = "G016_test: ";

/* Use this to observe previous proc */
int LAST_PROC = -1; 

void set_test_procs() {	
	int i;		
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
	
	g_test_procs[1].m_priority=HIGH;

	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[5].mpf_start_pc = &proc6;
}

/* The null process never terminates. Its priority should be lowest.
*/
void null_proccess(void)
{	
	while (1) {
		printf("null process\n");
		release_processor();
	}
}


/* Test 1: Send message from proc2 to proc1 */
void proc2(void)
{	
	MSG_BUF *msg;
	void *mem = request_memory_block();
	
	msg = (MSG_BUF*)mem;
	msg->mtype = DEFAULT;
	strcpy(msg->mtext, "Sent from proc 2");
	send_message(1, mem);
	set_process_priority(1, HIGH);

	release_memory_block(mem);

	while(1) {
		release_processor();
	}
}

void proc1(void)
{
	void *mem;
	MSG_BUF *msg;
	int sender;

	mem = receive_message(&sender);
	msg = (MSG_BUF*)mem;

	if (2 == sender && strcmp(msg->mtext, "Sent from procjg 2") == 0) {
		printf("Test 1 Passed");
	} 
	
	set_process_priority(1, LOW);
	while(1) {
		release_processor();
	}
}

void proc6(void)
{
}


void proc3(void)
{	
	
}

void proc4(void)
{
}

void proc5(void)
{
}



