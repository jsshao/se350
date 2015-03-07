#ifndef KERNEL_PROC_H_
#define KERNEL_PROC_H   
#include "k_rtx.h"
#define TIMER_PID NUM_TEST_PROCS + 1

void set_kernel_procs(void);
void timer_i_process(void);

#endif
