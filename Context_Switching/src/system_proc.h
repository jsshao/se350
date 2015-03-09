#ifndef SYSTEM_PROC_H_
#define SYSTEM_PROC_H_   

#define NUM_NULL_PROCS 1
#define NUM_SYSTEM_PROCS 7

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
