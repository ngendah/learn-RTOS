#include <stdint.h>
#include "gpio.h"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#define SYS_CLK 				((uint32_t)16E6) //16MHZ internal clock
#define TASKS					3
#define STACK_SIZE				48

extern uint32_t _estack;
uint32_t *__msp_heap_start = (uint32_t*) &_estack;
uint32_t *__psp_heap_start = ((uint32_t*) &_estack - 1 * 1024);
uint32_t *__psp_heap_end = ((uint32_t*) &_estack - 2 * 1024);

typedef enum
{
  STATE_RUNNING, STATE_BLOCKED,
} task_state_t;

typedef struct
{
  uint32_t *psp;
  task_state_t state;
  uint32_t delay;
  void
  (*task_handler) (void);
} tcb_t;

uint8_t current_task = 0;

//function prototypes
void
idle (void);
void
task1 (void);
void
task2 (void);
void
delay (uint32_t);

//initialize the tasks
tcb_t tasks[] =
  { 0 };

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

void
init_task_stack (void)
{
  void *handlers[] =
    { &idle, &task1, &task2 };
  for (int i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++)
    {
      tasks[i].psp = __psp_heap_start - (i + 1) * STACK_SIZE;
      tasks[i].state = STATE_RUNNING;
      tasks[i].delay = 0;
      tasks[i].task_handler = handlers[i];
      const uint32_t dummy_frame[] =
	{ 0x01000000/*xPSR*/, (uint32_t) tasks[i].task_handler/*return addr*/,
	    0xFFFFFFFD/*LR*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
      const uint32_t len = sizeof(dummy_frame) / sizeof(dummy_frame[0]);
      uint32_t *ptr = tasks[i].psp;
      for (uint32_t i = 0; i < len; i++)
	{
	  ptr--;
	  *ptr = dummy_frame[i];
	}
      tasks[i].psp = ptr;
    }
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
  uint16_t i = current_task == 0 ? 1 : current_task + 1;
  for (; i != current_task; i++)
    {
      i = i == TASKS ? 1 : i;
      uint16_t j = i;
      if (tasks[j].state != STATE_RUNNING)
	{
	  tasks[j].delay -= 1;
	  tasks[j].state =
	      tasks[j].delay == 0 ? STATE_RUNNING : STATE_BLOCKED;
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
  __asm volatile("msr MSP, %0"::"r"((uint32_t)__msp_heap_start));
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
  init_task_stack ();
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
