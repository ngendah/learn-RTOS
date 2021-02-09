# task-scheduler
=========================

This is simple task scheduler for STM32F4.

It schedules 2 tasks to be executed at 1 msec interval.

Highlights are;

1. initialization of tasks private stacks and the scheduler stack.

	on the functions `init_task_stack`, `init_psp` and `init_scheduler_stack`.

2. system tick(systick) context switching.
