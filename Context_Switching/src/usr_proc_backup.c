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

/* Test 1 / 2: Test request memory block
               Test release memory block
 */
void proc1(void)
{
	void* temp;
	int release_ret_val;
	
	// Print starting message
	printf("%sSTART\n\r", GROUP_PREFIX);
	printf("%stotal 6 tests\n\r", GROUP_PREFIX);
	
	// Test 1: Get memory block and see if it's a valid address
	temp = request_memory_block();
	if (NULL != temp) {
		TEST_BIT_PASSED |= (1 << 0);
		TOTAL_TESTS_PASSED++;
	}

	// Test 2: Release memory block and try to release the same block (should be error)
	release_memory_block(temp);
	release_ret_val = release_memory_block(temp);
	if (release_ret_val == RTX_ERR) {
		TEST_BIT_PASSED |= (1 << 1);
		TOTAL_TESTS_PASSED++;
	}
	
	// Set this process as last process
	LAST_PROC = 1;
	
	while(1) {
		release_processor();
	}
}

/* Test 3 / 4: Test process switching
							 Test setting process priority
 */
void proc2(void)
{	
	
	// Test 3: Switching from proc1 to proc2 is a succesful process switch 
  // in correct order of the queue
	if (LAST_PROC == 1) {
		TEST_BIT_PASSED |= (1 << 2);
		TOTAL_TESTS_PASSED++;
	}
	
	LAST_PROC = 2;

	// Test 4: Set proc6 to have highest priority. We expect proc 6 to execute next.
	set_process_priority(6, HIGH);

	while ( 1) {
		release_processor();
	}
}

/* Test 4 / 5: Test 4: proc should be run after proc2 because it tests set priority 
							 Test 5: This proc also tests get priority.
 */
void proc6(void)
{
	int prior2;
	int prior6;
  int	prior100;
	
	// Test 4: proc 2 should have been just run
	if (LAST_PROC == 2) {
		TEST_BIT_PASSED |= (1 << 3);
		TOTAL_TESTS_PASSED++;
	}
		
	// Test 5: the priority of proc 6 should be HIGH
	//         the priority of proc 2 should be LOWEST
	//         the priority of proc100 should return -1
	// 			   setting process priority to 100 should do nothing
	prior2 = get_process_priority(2);
	prior100 = get_process_priority(100);
	set_process_priority(6, 100);
	prior6 = get_process_priority(6);

	
	if (prior2 == LOWEST && prior6 == HIGH && prior100 == -1) {
		TEST_BIT_PASSED |= (1 << 4);
		TOTAL_TESTS_PASSED++;
	}
	
	// Set this as last process
	LAST_PROC = 6;
	
	// Reset this priority to low to run other processes
	set_process_priority(6, LOWEST);
	
	while ( 1) {
		release_processor();
	}
}


/* 
 *Test 6: It will test the blocked queue and preemption(set_process_priority)
 *
 * process 3 request 1 block memory, let 4 preempt it by setting 4 to a higher priority,
 * 4 ask for infinite memory, gets blocked when memory run out, 3 is run again, 3 frees memory
 * which unblocks 4 and it preempts 3, and 4 sees a flag LAST_PROC = 3
 *
 **/
void proc3(void)
{	
	void *temp;
	
	temp = request_memory_block();
	
	//Let 4 preempt 3
	set_process_priority(3, MEDIUM);
	set_process_priority(4, HIGH);
	
	
	
	LAST_PROC = 3;
	release_memory_block(temp);
	
	while(1) {
		release_processor();
	}
}

void proc4(void)
{
	void *requests[30] = {0};
	int i = 0;
	
	for (i = 0; i < 30; i++) {
		if (LAST_PROC == 3) {
			break;
		}
		requests[i] = request_memory_block();
	}
	
	// Test 6: this proc has successfully been blocked and unblocked by proc 3
	TEST_BIT_PASSED |= (1 << 5);
	TOTAL_TESTS_PASSED++;
	
	set_process_priority(3, LOWEST);
	set_process_priority(4, LOWEST);
	
	for (i = 0; i < 30; i++) {
		if (NULL != requests[i]) {
			release_memory_block(requests[i]);
		}
	}
	
	set_process_priority(5, HIGH);
	
	while(1) {
		release_processor();
	}
}

void proc5(void)
{
	int i;
	
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
	
	while(1) {
		release_processor();
	}
}



