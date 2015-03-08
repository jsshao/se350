#ifndef SYSTEM_PROC_H_
#define SYSTEM_PROC_H_   

#define NUM_NULL_PROCS 1
#define NUM_SYSTEM_PROCS 7

#define NULL_PID 0
#define A_PID 7
#define B_PID 8
#define C_PID 9
#define SET_PRIORITY_PID 10
#define CLOCK_PID 11
#define KCD_PID 12
#define CRT_PID 13

void set_system_procs(void);


//NULL process
void null_process(void);


//system processes 
void a_process(void);
void b_process(void);
void c_process(void);
void set_priority_process(void);
void clock_process(void);
void kcd_process(void);
void crt_process(void);

#endif
