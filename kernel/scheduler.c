#include "kernel.h"
#include "port.h"
#include "heap.h"
#include "smp.h"
#include "spinlock.h"

#include <stddef.h>

volatile uint64_t os_tick_count;
volatile int os_reschedule_pending;

static os_task_t *task_list[OS_MAX_TASKS];
static size_t task_count;

extern void os_task_init_context(os_task_t *task);
extern void os_start_first_task(void) __attribute__((noreturn));

static void print_uint64(uint64_t v)
{
    char buf[21];
    int i = 20;
    buf[i] = '\0';
    if (v == 0) {
        port_uart_putc('0');
        return;
    }
    while (v > 0 && i > 0) {
        buf[--i] = (char)('0' + (v % 10U));
        v /= 10U;
    }
    port_uart_puts(&buf[i]);
}

static void os_tick_store(uint64_t v)
{
#if defined(__aarch64__)
    __asm__ volatile("stlr %x0, [%1]" :: "r"(v), "r"(&os_tick_count) : "memory");
#else
    os_tick_count = v;
#endif
}

static uint64_t os_tick_read(void)
{
#if defined(__aarch64__)
    uint64_t v;

    __asm__ volatile("ldar %x0, [%1]" : "=r"(v) : "r"(&os_tick_count) : "memory");
    return v;
#else
    return os_tick_count;
#endif
}

static void wake_sleeping_tasks(void)
{
    uint64_t now = os_tick_read();

    for (size_t i = 0; i < task_count; i++) {
        os_task_t *t = task_list[i];
        if (t->state == OS_TASK_BLOCKED && t->wake_tick != 0U && now >= t->wake_tick) {
            t->state = OS_TASK_READY;
            t->run_cpu = -1;
        }
    }
}

void os_scheduler_init(void)
{
    os_heap_init();
    os_tick_count = 0;
    os_reschedule_pending = 0;
    task_count = 0;
    os_smp_init();
}

static uint8_t clamp_priority(uint8_t priority)
{
    if (priority > OS_TASK_PRIO_MAX) {
        return OS_TASK_PRIO_MAX;
    }
    return priority;
}

os_task_t *os_task_create(const char *name, os_task_fn_t fn, uint8_t *stack, void *arg, uint8_t priority)
{
    os_task_t *task;

    if (task_count >= OS_MAX_TASKS || fn == NULL) {
        return NULL;
    }

    task = os_mem_alloc(sizeof(os_task_t));
    if (task == NULL) {
        return NULL;
    }

    if (stack == NULL) {
        stack = os_mem_alloc(OS_TASK_STACK_SIZE);
        if (stack == NULL) {
            os_mem_free(task);
            return NULL;
        }
        task->stack_on_heap = 1;
    } else {
        task->stack_on_heap = 0;
    }

    task->stack_top = 0;
    task->state = OS_TASK_READY;
    task->fn = fn;
    task->arg = arg;
    task->wake_tick = 0;
    task->name = name;
    task->stack = stack;
    task->priority = clamp_priority(priority);
    task->aff_cpu = -1;
    task->run_cpu = -1;
    task->wait_next = NULL;

    task_list[task_count++] = task;
    os_task_init_context(task);
    return task;
}

void os_task_set_priority(os_task_t *task, uint8_t priority)
{
    if (task == NULL) {
        return;
    }

    os_spinlock_acquire(&os_sched_lock);
    task->priority = clamp_priority(priority);
    os_spinlock_release(&os_sched_lock);
}

uint8_t os_task_priority(const os_task_t *task)
{
    if (task == NULL) {
        return OS_TASK_PRIO_DEFAULT;
    }

    return task->priority;
}

void os_task_bind(os_task_t *task, uint32_t cpu_id)
{
    if (task == NULL || cpu_id >= OS_CPU_COUNT) {
        return;
    }

    os_spinlock_acquire(&os_sched_lock);
    task->aff_cpu = (int8_t)cpu_id;
    os_spinlock_release(&os_sched_lock);
}

static int task_live_on_other_cpu(const os_task_t *t, uint32_t cpu_id)
{
#if OS_CPU_COUNT > 1
    for (uint32_t i = 0; i < OS_CPU_COUNT; i++) {
        if (i != cpu_id && os_cpus[i].current == t) {
            return 1;
        }
    }
#else
    (void)t;
    (void)cpu_id;
#endif
    return 0;
}

static int task_eligible(const os_task_t *t, uint32_t cpu_id, int steal)
{
    if (task_live_on_other_cpu(t, cpu_id)) {
        return 0;
    }

    if (t->aff_cpu >= 0 && (uint32_t)t->aff_cpu != cpu_id) {
        return 0;
    }

    if (!steal) {
        if (t->state != OS_TASK_READY) {
            return 0;
        }

        if (t->run_cpu >= 0 && (uint32_t)t->run_cpu != cpu_id) {
            return 0;
        }

        return 1;
    }

    if (t->state != OS_TASK_READY) {
        return 0;
    }

    if (t->run_cpu < 0 || (uint32_t)t->run_cpu == cpu_id) {
        return 0;
    }

    return 1;
}

static os_task_t *pick_next_task(uint32_t cpu_id, int steal)
{
    os_task_t *best = NULL;
    uint8_t best_prio = 0;
    size_t start = 0;
    os_task_t *cur = os_cpu_current();

    if (task_count == 0) {
        return NULL;
    }

    if (cur != NULL) {
        for (size_t i = 0; i < task_count; i++) {
            if (task_list[i] == cur) {
                start = (i + 1U) % task_count;
                break;
            }
        }
    }

    for (size_t i = 0; i < task_count; i++) {
        os_task_t *t = task_list[i];

        if (!task_eligible(t, cpu_id, steal)) {
            continue;
        }

        if (best == NULL || t->priority > best_prio) {
            best = t;
            best_prio = t->priority;
        }
    }

    if (best == NULL) {
        return NULL;
    }

    for (size_t n = 0; n < task_count; n++) {
        size_t i = (start + n) % task_count;
        os_task_t *t = task_list[i];

        if (!task_eligible(t, cpu_id, steal)) {
            continue;
        }

        if (t->priority == best_prio) {
            return t;
        }
    }

    return best;
}

static void detach_task_from_cpu(os_task_t *task, uint32_t cpu_id)
{
    if (task == NULL || cpu_id >= OS_CPU_COUNT) {
        return;
    }

    if (os_cpus[cpu_id].current == task) {
        os_cpus[cpu_id].current = NULL;
    }
}

void os_schedule(void)
{
    uint32_t cpu_id = port_cpu_id();
    os_task_t *next;
    os_task_t *cur;
    int stolen = 0;

    os_spinlock_acquire(&os_sched_lock);

    cur = os_cpus[cpu_id].current;
    if (cur != NULL && cur->run_cpu >= 0 && (uint32_t)cur->run_cpu != cpu_id) {
        os_cpus[cpu_id].current = NULL;
        cur = NULL;
    }

    next = pick_next_task(cpu_id, 0);
    if (next == NULL) {
        next = pick_next_task(cpu_id, 1);
        if (next != NULL) {
            stolen = 1;
        }
    }

    if (next == NULL) {
        if (cur != NULL && cur->state != OS_TASK_RUNNING) {
            os_cpus[cpu_id].current = NULL;
        }
        os_spinlock_release(&os_sched_lock);
        return;
    }

    cur = os_cpus[cpu_id].current;
    if (next == cur) {
        os_spinlock_release(&os_sched_lock);
        return;
    }

    if (stolen && next->run_cpu >= 0) {
        detach_task_from_cpu(next, (uint32_t)next->run_cpu);
    }

    if (cur != NULL && cur->state == OS_TASK_RUNNING) {
        cur->state = OS_TASK_READY;
        cur->run_cpu = -1;
    }

    next->state = OS_TASK_RUNNING;
    next->run_cpu = (int8_t)cpu_id;
    os_cpus[cpu_id].current = next;
#if OS_CPU_COUNT == 1
    os_current_task = next;
#endif

    os_spinlock_release(&os_sched_lock);

    if (stolen) {
        os_smp_send_reschedule_ipi();
    }
}

void os_tick_handler(void)
{
    uint64_t now;

    os_spinlock_acquire(&os_sched_lock);
    now = os_tick_read() + 1U;
    os_tick_store(now);
    wake_sleeping_tasks();
    os_reschedule_pending = 1;
    os_spinlock_release(&os_sched_lock);

    os_schedule();
    os_smp_send_reschedule_ipi();
}

void os_critical_enter(void)
{
    port_irq_disable();
}

void os_critical_exit(void)
{
    port_irq_enable();
}

void os_kernel_init(void)
{
    port_early_init();
    os_scheduler_init();
    port_gic_init();
    port_timer_init();
}

void os_kernel_start(void)
{
    if (port_cpu_id() != 0U) {
        os_secondary_cpu_main();
    }

    os_smp_boot_secondaries();

    port_uart_puts("[kernel] scheduler started (SMP)\n");
    port_uart_puts("[tick] timer frequency: ");
    print_uint64(port_timer_frequency());
    port_uart_puts(" Hz\n");

    os_cpu_enter_scheduler();
}

uint64_t os_tick_get(void)
{
    return os_tick_read();
}

uint32_t os_time_ms(void)
{
    return (uint32_t)(os_tick_read() * 1000U / OS_TICK_HZ);
}

uint32_t os_cpu_id(void)
{
    return port_cpu_id();
}

void os_task_delay(uint32_t ms)
{
    os_task_t *self = os_cpu_current();
    if (self == NULL) {
        return;
    }

    uint64_t ticks = ((uint64_t)ms * OS_TICK_HZ) / 1000U;
    if (ticks == 0) {
        ticks = 1;
    }

    uint64_t wake = os_tick_read() + ticks;

    os_critical_enter();
    self->wake_tick = wake;
    self->state = OS_TASK_BLOCKED;
    self->run_cpu = -1;
    os_critical_exit();

    os_yield();
}
