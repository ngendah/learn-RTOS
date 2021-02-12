#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
#warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#define SYS_CLK 				((uint32_t)16E6) //16MHZ internal clock
#define RAM_START				0x20000000
#define RAM_SIZE				(128* 1024)
#define RAM_END					(RAM_START + RAM_SIZE)

#define STACK_SIZE				1024
#define TASKS					3
#define IDLE_STACK_START		(RAM_END)
#define TASK1_STACK_START		(RAM_END - (1* STACK_SIZE))
#define TASK2_STACK_START		(RAM_END - (2* STACK_SIZE))
#define SCHEDULER_STACK_START	(RAM_END - (TASKS * STACK_SIZE))

typedef enum {
	STATE_RUNNING, STATE_BLOCKED,
} task_state_t;

// use a task control block type
typedef struct {
	uint32_t *psp;
	task_state_t state;
	uint32_t delay_count;
	void (*task_handler)(void);
} tcb_t;

// index of the current executing task
uint8_t current_task = 1;

//function prototypes
void
idle(void);
void
task1(void);
void
task2(void);
void
delay(uint32_t);

//initialize the tasks
tcb_t tasks[] = {
		(tcb_t ){ .psp = (uint32_t*) IDLE_STACK_START, .state = STATE_RUNNING,
				.delay_count = 0, .task_handler = &idle },
		(tcb_t ){ .psp = (uint32_t*) TASK1_STACK_START, .state = STATE_RUNNING,
						.delay_count = 0, .task_handler = &task1 },
		(tcb_t ){ .psp = (uint32_t*) TASK2_STACK_START, .state = STATE_RUNNING,
						.delay_count = 0, .task_handler = &task2 }
};

void enable_faults(void) {
	uint32_t *ptr = (uint32_t*) 0xE000ED24;
	*ptr |= (1 << 16); // mem fault
	*ptr |= (1 << 17); //bus fault
	*ptr |= (1 << 18); //usage fault
}

void enable_systick(void) {

	//init registers
	uint32_t *systick_csr = (uint32_t*) 0xE000E010;
	uint32_t *systick_rvr = (uint32_t*) 0xE000E014;

	//set tick period to 1ms
	uint32_t tick = (SYS_CLK / ((uint32_t) 16E3)) - 1; // 1ms
	*systick_rvr &= ~0x00FFFFFF;
	*systick_rvr |= tick;

	//enable systick
	*systick_csr |= (1 << 2); //clk source as internal clock
	*systick_csr |= (1 << 1); //enable systick exception
	*systick_csr |= (1 << 0); //enable counter
}

__attribute__((naked)) void init_scheduler_stack() {
	__asm volatile("msr MSP, %0"::"r"(SCHEDULER_STACK_START));
	__asm volatile("bx LR");
}

void init_task_stack(void) {
	for (int i = 0; i < TASKS; i++) {
		const uint32_t dummy_frame[] = { 0x01000000/*xPSR*/,
				(uint32_t) tasks[i].task_handler/*return addr*/,
				0xFFFFFFFD/*LR*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		const uint32_t len = sizeof(dummy_frame) / sizeof(dummy_frame[0]);
		uint32_t *ptr = tasks[i].psp;
		for (uint32_t i = 0; i < len; i++) {
			ptr--; //the stack is full descending
			*ptr = dummy_frame[i];
		}
		tasks[i].psp = ptr;
	}
}

__attribute__((naked)) void init_psp(void) {
	// init psp
	__asm volatile("push {LR}");
	__asm volatile("mov R0, %0"::"r"(tasks[current_task].psp));
	__asm volatile("msr PSP, R0");
	__asm volatile("pop {LR}");
	//change tasks sp to psp
	__asm volatile("mov R0, #0x02");
	__asm volatile("msr CONTROL, R0");
	__asm volatile("bx LR");
}

void delay(uint32_t msec) {
	tasks[current_task].delay_count = msec;
	tasks[current_task].state = STATE_BLOCKED;
}

int main(void) {
	enable_faults();
	init_scheduler_stack();
	init_task_stack();
	init_psp();
	enable_systick();
	task1();
	/* Loop forever */
	for (;;)
		;
}

void set_current_task_psp(uint32_t psp_addr) {
	tasks[current_task].psp = (uint32_t*) psp_addr;
}

uint32_t* get_current_task_psp(void) {
	return tasks[current_task].psp;
}

void next_task(void) {
	uint16_t next_task = 0;
	uint16_t i = current_task == 0 ? 1 : current_task + 1;
	for (; i != current_task; i++) {
		i = i == TASKS ? 1 : i;
		uint16_t j = i;
		if (tasks[j].state != STATE_RUNNING) {
			tasks[j].delay_count -= 1;
			tasks[j].state =
					tasks[j].delay_count == 0 ? STATE_RUNNING : STATE_BLOCKED;
			continue;
		}
		next_task = j;
		break;
	}
	current_task = next_task;
}

__attribute__((naked)) void SysTick_Handler(void) {
	// I.

	//1. get the current task psp value
	__asm volatile("mrs R0, PSP");
	//2. push registers R4-R11 to private stack,
	// these registers are not automatically saved when the processor switches from
	// thread mode to handler mode
	__asm volatile("stmdb R0!, {R4-R11}");
	//3. save lr before a series of intermediate calls
	__asm volatile("push {LR}");
	//4. modify lr and save the current R0 value to psp stack
	__asm volatile("bl set_current_task_psp");

	// II.

	//1. update the current task index
	__asm volatile("bl next_task");
	//2. get current task psp
	__asm volatile("bl get_current_task_psp");
	//3. retrieve R4-R11 from psp
	__asm volatile("ldmia R0!, {R4-R11}");
	//4. load sp with current psp
	__asm volatile("msr PSP, R0");
	//5. pop lr
	__asm volatile("pop {LR}");
	//6. resume execution
	__asm volatile("bx LR");
}

void task1(void) {
	while (1) {
		delay(1000);
	}
}

void task2(void) {
	while (1) {
		;
	}
}
