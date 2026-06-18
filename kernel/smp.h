#ifndef OS_SMP_H
#define OS_SMP_H

#include <stdint.h>

#include "os.h"
#include "port.h"
#include "spinlock.h"

#ifndef OS_CPU_COUNT
#define OS_CPU_COUNT 1
#endif

typedef struct os_cpu {
    os_task_t *current;
    uint32_t  id;
    uint8_t   irq_stack[4096] __attribute__((aligned(16)));
} os_cpu_t;

extern os_cpu_t          os_cpus[OS_CPU_COUNT];
extern volatile uint32_t os_cpu_online[OS_CPU_COUNT];
extern os_spinlock_t     os_sched_lock;

#if OS_CPU_COUNT == 1
extern os_task_t *os_current_task;
#endif

static inline os_task_t *os_cpu_current(void)
{
    return os_cpus[port_cpu_id()].current;
}

static inline void os_cpu_set_current(os_task_t *task)
{
    os_cpus[port_cpu_id()].current = task;
}

void os_smp_init(void);
void os_smp_boot_secondaries(void);
void os_smp_send_reschedule_ipi(void);
void os_secondary_cpu_main(void) __attribute__((noreturn));
void os_cpu_enter_scheduler(void) __attribute__((noreturn));

#endif /* OS_SMP_H */
