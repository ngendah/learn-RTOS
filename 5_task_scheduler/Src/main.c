#include <stdint.h>
#include "gpio.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#define SYS_CLK 				((uint32_t)16E6) //16MHZ internal clock
#define MAX_TASKS				16
#define STACK_SIZE				48
#define KB(size)				(((uint32_t)((size * 1024)/8))*8)

extern uint32_t _estack;
uint32_t *__psp_heap_start = ((uint32_t*) &_estack - KB(1));
uint32_t *__psp_heap_end = ((uint32_t*) &_estack - KB(2));

typedef enum
{
  STATE_RUNNING, STATE_BLOCKED,
} task_state_t;

typedef struct
{
  uint32_t *psp;
  task_state_t state;
  uint32_t delay;
} tcb_t;

typedef void
(*task_handler_t) (void);

tcb_t tasks[MAX_TASKS] =
  { 0 };

uint8_t no_of_tasks = 0;

uint8_t current_task = 0;

void
idle (void);
void
task1 (void);
void
task2 (void);
void
task3 (void);
void
delay (uint32_t);

void
enable_faults (void)
{
  uint32_t *ptr = (uint32_t*) 0xE000ED24;
  *ptr |= (1 << 16); // mem fault
  *ptr |= (1 << 17); //bus fault
  *ptr |= (1 << 18); //usage fault
}

void
enable_systick (void)
{
  uint32_t *systick_csr = (uint32_t*) 0xE000E010;
  uint32_t *systick_rvr = (uint32_t*) 0xE000E014;
  uint32_t tick = (SYS_CLK / ((uint32_t) 16E3)) - 1; // 1ms
  *systick_rvr &= ~0x00FFFFFF;
  *systick_rvr |= tick;
  *systick_csr |= (1 << 2); //clk source as internal clock
  *systick_csr |= (1 << 1); //enable systick exception
  *systick_csr |= (1 << 0); //enable counter
}

uint8_t
task_start (task_handler_t handler)
{
  if (no_of_tasks >= MAX_TASKS)
    {
      return -1; //error
    }
  uint32_t stack_frame[] =
    { 0x01000000/*xPSR*/, (uint32_t) handler/*return addr*/, 0xFFFFFFFD/*LR*/,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 /*R4 - R11*/};
  uint8_t index = no_of_tasks;
  uint8_t len = sizeof(stack_frame) / sizeof(stack_frame[0]);
  uint32_t *ptr = __psp_heap_start - index * STACK_SIZE;
  for (uint8_t i = 0; i < len; i++)
    {
      ptr--;
      *ptr = stack_frame[i];
    }
  tasks[index].psp = ptr;
  tasks[index].state = STATE_RUNNING;
  no_of_tasks += 1;
  return index;
}

void
set_current_task_psp (uint32_t psp_addr)
{
  tasks[current_task].psp = (uint32_t*) psp_addr;
}

uint32_t*
get_current_task_psp (void)
{
  return tasks[current_task].psp;
}

void
next_task (void)
{
  uint16_t next_task = 0;
  if (no_of_tasks == 1)
    {
      current_task = next_task;
      return;
    }
  uint16_t i = current_task == 0 ? 1 : current_task + 1;
  for (; i != current_task; i++)
    {
      i = i == no_of_tasks ? 1 : i;
      uint16_t j = i;
      if (tasks[j].delay > 0)
	{
	  tasks[j].delay -= 1;
	  tasks[j].state = tasks[j].delay == 0 ? STATE_RUNNING : STATE_BLOCKED;
	  continue;
	}
      next_task = j;
      break;
    }
  current_task = next_task;
}

__attribute__((naked)) void
SysTick_Handler (void)
{
  __asm volatile("mrs R0, PSP");
  __asm volatile("stmdb R0!, {R4-R11}");
  __asm volatile("push {LR}");
  __asm volatile("bl set_current_task_psp");
  __asm volatile("bl next_task");
  __asm volatile("bl get_current_task_psp");
  __asm volatile("pop {LR}");
  __asm volatile("ldmia R0!, {R4-R11}");
  __asm volatile("msr PSP, R0");
  __asm volatile("bx LR");
}

__attribute__((naked)) void
init_psp (void)
{
  __asm volatile("msr PSP, %0"::"r"(tasks[0].psp));
  __asm volatile("mov R0, #0x02");
  __asm volatile("msr CONTROL, R0");
  __asm volatile("bx LR");
}

void
delay (uint32_t msec)
{
  tasks[current_task].delay = msec;
  tasks[current_task].state = STATE_BLOCKED;
}

void
scheduler_init ()
{
  enable_faults ();
  task_start (&idle);
}

void
scheduler_run ()
{
  init_psp ();
  enable_systick ();
  idle ();
}

int
main (void)
{
  enable_gpiod ();
  configure_gpiod (P12);
  configure_gpiod (P13);
  scheduler_init ();
  task_start (&task1);
  task_start (&task2);
  scheduler_run ();
}

void
idle (void)
{
  while (1)
    {
      ;
    }
}

void
task1 (void)
{
  while (1)
    {
      toggle_gpiod (P12);
      delay (5000);
    }
}

void
task2 (void)
{
  while (1)
    {
      toggle_gpiod (P13);
      delay (2500);
    }
}

void
task3 (void)
{
  while (1)
    {
      toggle_gpiod (P14);
      delay (3500);
    }
}
