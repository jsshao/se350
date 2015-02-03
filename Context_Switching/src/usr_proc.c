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

void set_test_procs() {	
	int i;	
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
	
  // NULL PROCESS
	g_test_procs[0].m_priority=4;	
	g_test_procs[0].mpf_start_pc = &null_proccess;
	
	g_test_procs[1].mpf_start_pc = &proc1;
	g_test_procs[2].mpf_start_pc = &proc2;
	g_test_procs[3].mpf_start_pc = &proc3;
	g_test_procs[4].mpf_start_pc = &proc4;
	g_test_procs[5].mpf_start_pc = &proc5;
	g_test_procs[6].mpf_start_pc = &proc6;
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


/* Test 1: Test request memory block
 */
void proc1(void)
{
	int i = 0;
	int ret_val;
	void* temp;
	
	while ( 1) {
		temp = request_memory_block();
		if (NULL != temp) {
			printf("%sSTART\n\r", GROUP_PREFIX);
		}
		i++;
	}
}

/**
 * @brief: a process that prints five numbers
 *         and then yields the cpu.
 */
void proc2(void)
{	
	int i = 0;
	int ret_val = 20;	
	void *temp;
	
	while ( 1) {
		temp = request_memory_block();
		if (NULL != temp) {
			printf("%sSTART\n\r", GROUP_PREFIX);
		}
		i++;
	}
	
	
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			ret_val = release_processor();	
#ifdef DEBUG_0			
#endif /* DEBUG_0 */
		}
		//uart0_put_char('0' + i%10);
		i++;
	}
}

void proc3(void)
{	
	while (1) {
		release_processor();	
#ifdef DEBUG_0			
		
#endif /* DEBUG_0 */
		}
				
}

void proc4(void)
{
	int i = 0;
	int ret_val = 20;
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc4: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
		}
		//uart0_put_char('0' + i%10);
		i++;
	}
}

void proc5(void)
{
	int i = 0;
	int ret_val = 20;
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc5: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
		}
		
		//uart0_put_char('0' + i%10);
		i++;
	}
}

void proc6(void)
{
	int i = 0;
	int ret_val = 20;
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc6: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
		}
		//uart0_put_char('0' + i%10);
		i++;
	}
}
