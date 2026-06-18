#ifndef OS_CONFIG_H
#define OS_CONFIG_H

/* Scheduler tick rate (Hz). */
#define OS_TICK_HZ          1000

/* Maximum number of concurrently creatable tasks. */
#define OS_MAX_TASKS        8

/* Default task stack size in bytes. */
#define OS_TASK_STACK_SIZE  4096

/*
 * Bump-heap size for dynamically allocated TCBs and task stacks.
 * Override before including headers to cap RAM below the theoretical max.
 * Default covers OS_MAX_TASKS × (stack + TCB overhead).
 */
#ifndef OS_HEAP_SIZE
#define OS_HEAP_SIZE        ((OS_TASK_STACK_SIZE + 64U) * OS_MAX_TASKS)
#endif

#endif /* OS_CONFIG_H */
