#ifndef OS_CONFIG_H
#define OS_CONFIG_H

/* Scheduler tick rate (Hz). */
#define OS_TICK_HZ          1000

/* Maximum number of concurrently creatable tasks. */
#define OS_MAX_TASKS        8

/* SMP: number of CPUs (override per port in Makefile). */
#ifndef OS_CPU_COUNT
#define OS_CPU_COUNT        1
#endif

/* Default task stack size in bytes. */
#define OS_TASK_STACK_SIZE  4096

/* Task priority range: higher number = higher priority. */
#define OS_TASK_PRIO_MIN        0
#define OS_TASK_PRIO_MAX        7
#define OS_TASK_PRIO_DEFAULT    3
#define OS_TASK_PRIO_LOW        1
#define OS_TASK_PRIO_NORMAL     3
#define OS_TASK_PRIO_HIGH       5
#define OS_TASK_PRIO_REALTIME   7

/*
 * Bump-heap size for dynamically allocated TCBs and task stacks.
 * Override before including headers to cap RAM below the theoretical max.
 * Default covers OS_MAX_TASKS × (stack + TCB overhead).
 */
#ifndef OS_HEAP_SIZE
#define OS_HEAP_SIZE        ((OS_TASK_STACK_SIZE + 64U) * OS_MAX_TASKS)
#endif

#endif /* OS_CONFIG_H */
