#ifndef OS_H
#define OS_H

#include <stdint.h>
#include <stddef.h>

#include "os_config.h"

typedef void (*os_task_fn_t)(void *arg);

typedef enum {
    OS_TASK_READY = 0,
    OS_TASK_RUNNING,
    OS_TASK_BLOCKED,
    OS_TASK_SUSPENDED,
} os_task_state_t;

typedef struct os_task {
    uintptr_t       stack_top;
    os_task_state_t state;
    os_task_fn_t    fn;
    void            *arg;
    uint64_t        wake_tick;
    const char      *name;
    uint8_t         *stack;
    uint8_t         stack_on_heap;
    uint8_t         priority;
    int8_t          aff_cpu;
    int8_t          run_cpu;
    struct os_task  *wait_next;
} os_task_t;

#include "os_sync.h"

void os_kernel_init(void);
void os_kernel_start(void) __attribute__((noreturn));

os_task_t *os_task_create(const char *name, os_task_fn_t fn, uint8_t *stack, void *arg, uint8_t priority);
void os_task_set_priority(os_task_t *task, uint8_t priority);
uint8_t os_task_priority(const os_task_t *task);
void os_task_bind(os_task_t *task, uint32_t cpu_id);
void os_yield(void);
void os_task_delay(uint32_t ms);

uint64_t os_tick_get(void);
uint32_t os_time_ms(void);
uint32_t os_cpu_id(void);

void os_panic(const char *msg) __attribute__((noreturn));

#endif /* OS_H */
