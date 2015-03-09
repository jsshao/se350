#ifndef KERNEL_PROC_H_
#define KERNEL_PROC_H_   
#include "k_rtx.h"

void set_kernel_procs(void);

//kernel interrupt processes
void timer_i_process(void);
void uart_i_process(void);

#endif
