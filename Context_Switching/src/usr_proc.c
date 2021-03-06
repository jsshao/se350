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

PROC_INIT g_test_procs[NUM_TEST_PROCS];
/* initialization table item */

/* Keep track of tests passed (This int will be 6 bits in total 
	 where the i'th bit means the i'th test passed). 
	 ie. 0b111111 means all tests passed.
 */
int TEST_BIT_PASSED = 0x0; 
int TOTAL_TESTS_PASSED = 0;
char GROUP_PREFIX[] = "G016_test: ";
int LAST_PROC = 0;
void *stress_requests[121] = {0};


void set_test_procs() {	
	int i;		
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
	
	g_test_procs[0].m_priority=MEDIUM;
	g_test_procs[1].m_priority=LOW;
	g_test_procs[2].m_priority=MEDIUM;

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

/**
Test 1: proc 1 gets blocked on receive, then unblocked by proc 2 sending
Test 2: after gets unblocked, send 2s delayed message to proc 3
Test 3: then sends 1s delayed message to proc 2

**/
void proc1(void)
{ 
	void *mem;
	void *mem2;
	MSG_BUF *msg;
	int sender;
	
	mem = receive_message(&sender);
	msg = (MSG_BUF*)mem;

	if (2 == sender && strcmp(msg->mtext, "Sent from proc 2") == 0) {
		TEST_BIT_PASSED |= (1 << 0);		//test case 1 passed
		TOTAL_TESTS_PASSED++;
	}
		
	release_memory_block(mem);
	
	mem = request_memory_block();	
	msg = (MSG_BUF*)mem;
	msg->mtype = DEFAULT;
	strcpy(msg->mtext, "Delayed 2s from 1 to 3");
	delayed_send(3, msg, 2000);
	
	mem2 = request_memory_block();	
	msg = (MSG_BUF*)mem2;
	msg->mtype = DEFAULT;
	strcpy(msg->mtext, "Delayed 1s from 1 to 2");
	delayed_send(2, msg, 1000);
	
	set_process_priority(1, LOW);
	
	
	
	while(1) {
		release_processor();
	}
}

/* Test 1: Send message from proc2 to proc1 
	 Test 3: gets message from proc1
*/
void proc2(void)
{	
	MSG_BUF *msg;
	void *mem = request_memory_block();
	int sender;	
	
	msg = (MSG_BUF*)mem;
	msg->mtype = DEFAULT;
	strcpy(msg->mtext, "Sent from proc 2");
	send_message(1, mem);	
	
	mem = receive_message(&sender);	
	msg = (MSG_BUF*)mem;
	if (1 == sender && strcmp(msg->mtext, "Delayed 1s from 1 to 2") == 0 
		&& TOTAL_TESTS_PASSED == 1) {
		TEST_BIT_PASSED |= (1 << 2);		//test case 3 passed
		TOTAL_TESTS_PASSED++;
	}
	release_memory_block(mem);	
	
	while(1) {
		release_processor();
	}
}

/*
	Test case 2: gets delayed message from process 1 (Should be after case 3 passes)
*/
void proc3(void)
{	
	MSG_BUF *msg;
	void *mem;
	int sender;
	int i;
	
	mem = receive_message(&sender);	
	msg = (MSG_BUF*)mem;
	if (1 == sender && strcmp(msg->mtext, "Delayed 2s from 1 to 3") == 0 
		&& TOTAL_TESTS_PASSED == 2) {			
		TEST_BIT_PASSED |= (1 << 1);		//test case 2 passed
		TOTAL_TESTS_PASSED++;
	}
	release_memory_block(mem);
	
	set_process_priority(1, LOWEST);
	set_process_priority(2, LOWEST);	
	printf("Please type %%C 4 0/1 to continue the test (Set proc 4 to HIGH/MEDIUM priority)\r\n");
	
	while(1) {
		release_processor();
	}
}

/* Test case 4: Test %C 4 0 actually set this process to a higher priority (premption) */
void proc4(void)
{
	set_process_priority(3, LOWEST);
	set_process_priority(4, LOW);
	
	TEST_BIT_PASSED |= (1 << 3);		//test case 4 passed
	TOTAL_TESTS_PASSED++;
	
	set_process_priority(5, MEDIUM);
	
	while(1) {
		release_processor();
	}	
}

/* Test case 5 and 6 */
void proc5(void)
{	
	void* temp;
	int release_ret_val;

	set_process_priority(4, LOWEST);
	set_process_priority(5, LOW);
	
	// Test 5: Get memory block and see if it's a valid address
	temp = request_memory_block();
	if (NULL != temp) {
		TEST_BIT_PASSED |= (1 << 4);
		TOTAL_TESTS_PASSED++;
	}

	// Test 6: Release memory block and try to release the same block (should be error)
	release_memory_block(temp);
	release_ret_val = release_memory_block(temp);
	if (release_ret_val == RTX_ERR) {
		TEST_BIT_PASSED |= (1 << 5);
		TOTAL_TESTS_PASSED++;
	}
	
	set_process_priority(6, MEDIUM);
	
	while(1) {
		release_processor();
	}
}


void proc6(void)
{
	int i;
	
	set_process_priority(5, LOWEST);
	set_process_priority(6, LOW);

	printf("\r\n");
	printf("%sSTART\n\r", GROUP_PREFIX);
	printf("%stotal 6 tests\n\r", GROUP_PREFIX);
	for (i = 0; i < 6; i++) {
		if (TEST_BIT_PASSED & (1 << i)) {
			printf("%stest %d OK\n\r", GROUP_PREFIX, i+1);
		} else {
			printf("%stest %d FAIL\n\r", GROUP_PREFIX, i+1);
		}
	}
	
	printf("%s%d/6 tests OK\n\r", GROUP_PREFIX, TOTAL_TESTS_PASSED);
	printf("%s%d/6 tests FAIL\n\r", GROUP_PREFIX, 6 - TOTAL_TESTS_PASSED);
	printf("%sEND\n\r", GROUP_PREFIX);
	
	set_process_priority(PID_A, HIGH);
	
	while(1) {
		release_processor();
	}
}


///---------------------------------------------------------------------------------

#ifdef REG1
/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];



void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[0].m_priority   = MEDIUM;
	
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[1].m_priority   = MEDIUM;
	
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[2].m_priority   = LOW;
	
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[3].m_priority   = LOW;
	
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[4].m_priority   = LOW;
	
	g_test_procs[5].mpf_start_pc = &proc6;
	g_test_procs[5].m_priority   = LOW;

}


/**
 * @brief: a process that prints 2x5 lowercase letters
 */
void proc1(void)
{
	int i = 0;
	int counter = 0;
	int ret_val = 100;
	while ( 1 ) {
		
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			counter++;
			if ( counter == 2 ) {
				ret_val = set_process_priority(PID_P2, HIGH);
				break;
			} else {
				ret_val = release_processor();
			}
#ifdef DEBUG_0
			printf("proc1: ret_val = %d \n", ret_val);
#endif /* DEBUG_0 */
		}
		uart0_put_char('a' + i%10);
		i++;
	}
	uart0_put_string("proc1 end of testing\n\r");
	while ( 1 ) {
		release_processor();
	}
}

/**
 * @brief: a process that prints 4x5 numbers 
 */
void proc2(void)
{
	int i = 0;
	int ret_val = 20;
	int counter = 0;
	
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			counter++;
			if ( counter == 4 ) {
				ret_val = set_process_priority(PID_P1, HIGH);
				break;
			} else {
				ret_val = release_processor();
			}
#ifdef DEBUG_0
			printf("proc2: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
		}
		uart0_put_char('0' + i%10);
		i++;
	}
	uart0_put_string("proc2 end of testing\n\r");
	while ( 1 ) {
		release_processor();
	}
}

void proc3(void)
{
	
	while(1) {
		uart0_put_string("proc3: \n\r");
		release_processor();
	}
}

void proc4(void)
{
	while(1) {
		uart0_put_string("proc4: \n\r");
		release_processor();
	}
}

void proc5(void)
{
	while(1) {
		uart0_put_string("proc5: \n\r");
		release_processor();
	}
}

void proc6(void)
{
	while(1) {
		uart0_put_string("proc6: \n\r");
		release_processor();
	}
}
#endif
