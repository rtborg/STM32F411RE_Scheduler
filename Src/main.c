#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "led.h"

/*
 * Implementing round robin task scheduler
 * The Nucleo board is running on its internal HSI oscillator at 8MHz
 *
 * Note: Tasks use PSP stack pointer, scheduler uses MSP stack pointer
 * */


// Function definitions
void init_systick_timer(uint32_t tick_hz);
__attribute__((naked)) void init_scheduler_start(uint32_t scheduler_stack_start);
void init_task_stack();
void enable_processor_faults();
uint32_t get_psp_value();
void update_next_task();
void save_psp_value(uint32_t current_psp_value);
__attribute__((naked)) void  switch_sp_to_psp();
void UsageFault_Handler_c(uint32_t *pBaseStackFrame);
void task_delay(uint32_t tick_count);


// Keeps track of running task. 0 is the idle task
uint8_t current_task = 1;
uint32_t global_tick_count = 0;

// Task control block
typedef struct
{
	uint32_t psp_value;
	uint32_t block_count;
	uint8_t current_state;
	void (*task_handler)(void);
} TCB_t;

TCB_t user_tasks[MAX_TASKS];

// Tasks declarations
void idle_task();
void task1_handler();
void task2_handler();
void task3_handler();
void task4_handler();



int main(void)
{
	led_init_all();

	led_on(LED_1);
	led_on(LED_2);
	led_on(LED_3);
	led_on(LED_4);

	enable_processor_faults();
	init_scheduler_start(SCHEDULER_STACK_START);
	init_task_stack();
	init_systick_timer(TICK_HZ);

	switch_sp_to_psp();
	task1_handler();

    /* Loop forever */
	for(;;);
}

void task1_handler()
{
	while(1)
	{
		led_on(LED_1);
		task_delay(1000);
		led_off(LED_1);
		task_delay(1000);
	}
}

void task2_handler()
{
	while(1)
	{
		led_on(LED_2);
		task_delay(500);
		led_off(LED_2);
		task_delay(500);
	}
}

void task3_handler()
{
	while(1)
	{
		led_on(LED_3);
		task_delay(250);
		led_off(LED_3);
		task_delay(250);
	}
}

void task4_handler()
{
	while(1)
	{
		led_on(LED_4);
		task_delay(1250);
		led_off(LED_4);
		task_delay(1250);
	}
}

void idle_task(void)
{
	while(1);
}

void schedule()
{
	uint32_t *pICSR = (uint32_t*)0xE000ED04;
	*pICSR |= (1 << 28);
}

/**
 * Set the tick count for a task
 * Change state to blocked
 * Trigger PendSV
 */
void task_delay(uint32_t tick_count)
{
	// Disable ISRs
	INTERRUPT_DISABLE();

	if (current_task) {
		user_tasks[current_task].block_count = global_tick_count + tick_count;
		user_tasks[current_task].current_state = TASK_BLOCKED_STATE;
		schedule();
	}

	// Enable ISRs
	INTERRUPT_ENABLE();
}

// Initialize and configure SysTick interrupt
void init_systick_timer(uint32_t tick_hz)
{
	uint32_t *pSYST_CSR = (uint32_t*)0xE000E010;
	uint32_t *pSYST_RVR = (uint32_t*)0xE000E014;

	// Get the preload value
	uint32_t count_value = (SYSTICK_TIMER_CLOCK/tick_hz) - 1;

	// Load value in RVR
	*pSYST_RVR = 0x0;
	*pSYST_RVR |= count_value;

	// Enable systick, clock source is internal HSI
	*pSYST_CSR |= 0b111;
}

// Function initializes scheduler stack pointer (MSP)
__attribute__((naked)) void init_scheduler_start(uint32_t scheduler_stack_start)
{
	// Initialize main stack pointer with the function argument
	__asm volatile ("MSR MSP,%0": : "r" (scheduler_stack_start) : );
	// Go back to caller using return to address stored in link register
	__asm volatile ("BX LR");
}

// Function initializes tasks control block, and stack pointers with dummy data
// The stack pointer contains SP,LR, PC and registers r0 to r12
void init_task_stack()
{
	// Initialize tasks control block

	user_tasks[0].current_state = TASK_READY_STATE;
	user_tasks[1].current_state = TASK_READY_STATE;
	user_tasks[2].current_state = TASK_READY_STATE;
	user_tasks[3].current_state = TASK_READY_STATE;
	user_tasks[4].current_state = TASK_READY_STATE;

	user_tasks[0].psp_value = IDLE_STACK_START;
	user_tasks[1].psp_value = T1_STACK_START;
	user_tasks[2].psp_value = T2_STACK_START;
	user_tasks[3].psp_value = T3_STACK_START;
	user_tasks[4].psp_value = T4_STACK_START;

	user_tasks[0].task_handler = idle_task;
	user_tasks[1].task_handler = task1_handler;
	user_tasks[2].task_handler = task2_handler;
	user_tasks[3].task_handler = task3_handler;
	user_tasks[4].task_handler = task4_handler;

	uint32_t *pPSP;

	for	(int i = 0; i < MAX_TASKS; i++)
	{
		// Get pointer to stack. Remember stack is full descending
		pPSP = (uint32_t*)user_tasks[i].psp_value;
		// Decrement stack pointer
		pPSP--;		// Points to location of XPSR
		*pPSP = DUMMY_XPSR; // See cortex M4 generic user guide, value should be 0x01000000

		pPSP--;		// Points to location of PC
		*pPSP = (uint32_t) user_tasks[i].task_handler;

		pPSP--;		// Points to location of LR
		*pPSP = 0xFFFFFFFD;	// See cortex M4 generic user guide

		// All other registers are set to 0 (r0 to r12)
		for (int c = 0; c < 13; c++) {
			pPSP--;
			*pPSP = 0;
		}

		// Store the new value of the stack pointer to array
		user_tasks[i].psp_value = (uint32_t)pPSP;
	}
}

void enable_processor_faults()
{
    // Enable all configurable exceptions
	uint32_t *pSHCSR = (uint32_t*)0xE000ED24;
	*pSHCSR |= (1 << 16); // mem manage
	*pSHCSR |= (1 << 17); // bus fault
	*pSHCSR |= (1 << 18); // usage fault
}

uint32_t get_psp_value()
{
	return user_tasks[current_task].psp_value;
}

void save_psp_value(uint32_t current_psp_value)
{
	user_tasks[current_task].psp_value = current_psp_value;
}

void update_next_task()
{
	int state = TASK_BLOCKED_STATE;

	// Check all tasks except 0, the idle task. The loop looks at task 1 first
	for (int i = 0; i < MAX_TASKS; i++)
	{
		current_task++;
		current_task %= MAX_TASKS;
		state = user_tasks[current_task].current_state;
		if ((state == TASK_READY_STATE) && (current_task != 0))
			break;
	}

	// If ALL tasks are blocked, the idle task can run
	if (state != TASK_READY_STATE) {
		current_task = 0;
	}
}

__attribute__((naked)) void switch_sp_to_psp()
{
	// Initialize the PSP with task 1 stack start

	// Save the value of LR, as when a naked function calls another, the value of LR is corrupted
	__asm volatile ("PUSH {LR}");
	// Get the value of psp of current task. Upon return, the return value is stored in R0
	__asm volatile ("BL get_psp_value");
	// Copy value held in R0 to PSP
	__asm volatile ("MSR PSP,R0");
	// Pop LR back. That ensures the function will return to main
	__asm volatile ("POP {LR}");

	// Change SP to PSP using CONTROL register
	__asm volatile ("MOV R0,#0x02");
	__asm volatile ("MSR CONTROL,R0");

	// Go back to main
	__asm volatile ("BX LR");
}

void unblock_tasks()
{
	// Check tasks states. Task 0 is the idle task and is always in RUN state
	for (int i = 1; i < MAX_TASKS; i++)
	{
		// If a task is NOT running
		if (user_tasks[i].current_state != TASK_READY_STATE)
		{
			// Check if the task delay has elapsed
			if (user_tasks[i].block_count == global_tick_count)
			{
				user_tasks[i].current_state = TASK_READY_STATE;
			}
		}
	}
}

/**
 * SysTick handler's jobs are
 * Increment global tick count
 * Decide which task is to run next
 * pend the PendSV interrupt
 */
void SysTick_Handler()
{
	global_tick_count++;
	unblock_tasks();
	schedule();
}

// Context switching is done in the PendSV handler
__attribute__((naked)) void PendSV_Handler()
{
	// Save the context of the current task. SF1 is already saved by the interrupt hardware

	// 1. Get the current running taks's PSP value
	__asm volatile ("MRS R0,PSP");	// Move from special register to general register
	// 2. Using that PSP value store SF2(R4 to R11)
	__asm volatile ("STMDB R0!,{R4-R11}");	// Store Multiple registers, decrement before. They're stored onto the stack as well


	// Save the value of LR, as when a naked function calls another, the value of LR is corrupted
	__asm volatile ("PUSH {LR}");


	// 3. Save the current value of PSP. R0 already holds the PSP value, so branch to function
	__asm volatile ("BL save_psp_value");

	// Retrieve the context of the next task
	// 1. Decide the task to run
	__asm volatile ("BL update_next_task");
	// 2. Get its past PSP value
	__asm volatile ("BL get_psp_value");
	// 3. Using that PSP value retrieve SP2
	__asm volatile ("LDMIA R0!,{R4-R11}");
	// 4. Update PSP and exit
	__asm volatile ("MSR PSP,R0");
	// Pop LR back. That ensures the function will return to main
	__asm volatile ("POP {LR}");
	// Go back to main
	__asm volatile ("BX LR");
}

// Implement fault handlers
void HardFault_Handler()
{
	printf("HardFault handler\n");
	while(1);
}
void MemManage_Handler()
{
	printf("MemManage handler\n");
	while(1);
}

void BusFault_Handler(){
	printf("BusFault handler\n");
	while(1);
}

__attribute__ ((naked)) void UsageFault_Handler()
{
	// Copy the Main Stack Pointer to R0
	// MSP is the base address of the stack frame which got saved during the exception entry from
	// thread to handler mode
		__asm ("MRS r0, MSP");
	// Jump to C function. The value of r0 becomes the callee's first argument
		__asm ("B UsageFault_Handler_c");
}

void UsageFault_Handler_c(uint32_t *pBaseStackFrame)
{
	// Read the UsageFault status register
	uint32_t *pUFSR = (uint32_t*) 0xE000ED2A;
	printf("UsageFault\n");
	printf("UFSR = %lx\n", (*pUFSR) & 0xffff);
	printf("MSP = %p\n", pBaseStackFrame);
	printf("Value of r0: %lx\n", pBaseStackFrame[0]);
	printf("Value of r1: %lx\n", pBaseStackFrame[1]);
	printf("Value of r2: %lx\n", pBaseStackFrame[2]);
	printf("Value of r3: %lx\n", pBaseStackFrame[3]);
	printf("Value of r12: %lx\n", pBaseStackFrame[4]);
	printf("Value of LR: %lx\n", pBaseStackFrame[5]);
	printf("Value of PC: %lx\n", pBaseStackFrame[6]);
	printf("Value of XPSR: %lx\n", pBaseStackFrame[7]);

	while(1);
}
