/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

/*----- Definitions -----*/

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0
#define NUM_TEST_PROCS 6
#define NUM_KERNEL_PROCS 2
#define NUM_SYSTEM_PROCS 7
#define NUM_PROCS 16		//everything above(15) + null proc(1)

/* Process IDs */
#define PID_NULL 0
#define PID_P1   1
#define PID_P2   2
#define PID_P3   3
#define PID_P4   4
#define PID_P5   5
#define PID_P6   6
#define PID_A    7
#define PID_B    8
#define PID_C    9
#define PID_SET_PRIO     10
#define PID_CLOCK        11
#define PID_KCD          12
#define PID_CRT          13
#define PID_TIMER_IPROC  14
#define PID_UART_IPROC   15

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

#define NUM_MEM_BLOCKS 40

/* process states, note we only assume three states in this example */
typedef enum {NEW = 0, RDY, RUN, BLOCKED, BLOCKED_ON_RECEIVE, BLOCKED_ON_ENV} PROC_STATE_E;  

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/

typedef struct msg_t{
	void* msg;
	struct msg_t* next;
	int dest_pid;
	int sender_pid;	
	int delay;
} MSG_T;

typedef struct pcb 
{ 
	//struct pcb *mp_next;  /* next pcb, not used in this example */  
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	PROC_STATE_E m_state;   /* state of the process */      
	int m_priority;
	MSG_T* head;
	MSG_T* tail;
} PCB;

/* initialization table item */
typedef struct proc_init
{	
	int m_pid;	        /* process id */ 
	int m_priority;         /* initial priority, not used in this example. */ 
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */    
} PROC_INIT;

//kernel copy
/* message buffer */
typedef struct msgbuf
{
	int mtype;              /* user defined message type */
	char mtext[32];          /* body of the message */	
} MSG_BUF;

/* Message Types */
#define DEFAULT 0
#define KCD_REG 1
#define COUNT_REPORT 2
#define WAKEUP10 3
#define CLOCK 4

#endif // ! K_RTX_H_
