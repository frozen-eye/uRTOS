#include "smp.h"
#include "port.h"
#include "kernel.h"
#include "os.h"

os_cpu_t os_cpus[OS_CPU_COUNT];

#if OS_CPU_COUNT == 1
/* Assembly on unicore ports still references this symbol. */
os_task_t *os_current_task;
#endif
volatile uint32_t os_cpu_online[OS_CPU_COUNT];
os_spinlock_t os_sched_lock;

extern void os_start_first_task(void) __attribute__((noreturn));
extern void os_cpu_wfi(void);

void os_smp_init(void)
{
    os_spinlock_init(&os_sched_lock);

    for (uint32_t i = 0; i < OS_CPU_COUNT; i++) {
        os_cpus[i].current = NULL;
        os_cpus[i].id = i;
        os_cpu_online[i] = 0U;
    }

    os_cpu_online[0] = 1U;
    port_cpu_init(0U);
#if OS_CPU_COUNT == 1
    os_current_task = NULL;
#endif
}

void os_smp_boot_secondaries(void)
{
#if OS_CPU_COUNT > 1
    port_smp_boot_secondaries();
#endif
}

void os_smp_send_reschedule_ipi(void)
{
#if OS_CPU_COUNT > 1
    port_smp_send_reschedule_ipi();
#else
    (void)0;
#endif
}

void os_secondary_cpu_main(void)
{
    uint32_t id = port_cpu_id();

    if (id == 0U || id >= OS_CPU_COUNT) {
        os_panic("invalid secondary cpu");
    }

    port_cpu_init(id);

    while (os_cpu_online[id] == 0U) {
        port_wfe();
    }

    os_cpu_enter_scheduler();
}

void os_cpu_enter_scheduler(void)
{
    port_irq_enable();

    for (;;) {
        os_schedule();

        if (os_cpu_current() != NULL) {
            os_start_first_task();
        }

        os_cpu_wfi();
    }
}
