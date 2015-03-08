#ifndef KERNEL_PROC_H_
#define KERNEL_PROC_H_   
#include "k_rtx.h"

#define TIMER_PID 14
#define UART_PID 15

void set_kernel_procs(void);

//kernel interrupt processes
void timer_i_process(void);
void uart_i_process(void);

#endif
