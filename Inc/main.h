/*
 * main.h
 *
 *  Created on: 5 Jan 2021
 *      Author: borislav
 */

#ifndef MAIN_H_
#define MAIN_H_

// Stack constants. Stack is full descending
#define MAX_TASKS				5
#define SIZE_TASK_STACK			1024U
#define SIZE_SCHEDULER_STACK	1024U

#define DUMMY_XPSR				0x01000000U

#define SRAM_START				0x20000000U
#define SIZE_SRAM				((128) * (1024))
#define SRAM_END				( (SRAM_START) + (SIZE_SRAM) )

#define T1_STACK_START			SRAM_END
#define T2_STACK_START			((SRAM_END) - (SIZE_TASK_STACK))
#define T3_STACK_START			((SRAM_END) - (2 * SIZE_TASK_STACK))
#define T4_STACK_START			((SRAM_END) - (3 * SIZE_TASK_STACK))
#define IDLE_STACK_START		((SRAM_END) - (4 * SIZE_TASK_STACK))
#define SCHEDULER_STACK_START	((SRAM_END) - (5 * SIZE_TASK_STACK))

// SysTick definitions
#define HSI_CLOCK				8000000U
#define SYSTICK_TIMER_CLOCK		HSI_CLOCK
#define TICK_HZ					1000U

// Tasks states
#define TASK_READY_STATE		0x00
#define TASK_BLOCKED_STATE		0xFF

#define INTERRUPT_DISABLE() do{ __asm volatile ("MOV R0, #0X1"); __asm volatile ("MSR PRIMASK, R0"); } while(0)
#define INTERRUPT_ENABLE() do{ __asm volatile ("MOV R0, #0X0"); __asm volatile ("MSR PRIMASK, R0"); } while(0)

#endif /* MAIN_H_ */
