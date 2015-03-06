//#include "rtx.h"
#include "uart_polling.h"
#include "kernel_procs.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"

#endif /* DEBUG_0 */


MSG_T* timer_head = NULL;
MSG_T* timer_tail = NULL; 

void timer_i_process() {
	printf("tingz");
	
} 