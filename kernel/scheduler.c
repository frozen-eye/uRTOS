#include "kernel.h"
#include "port.h"
#include "heap.h"

#include <stddef.h>

os_task_t *os_current_task;
os_task_t *os_next_task;
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

static void wake_sleeping_tasks(void)
{
    for (size_t i = 0; i < task_count; i++) {
        os_task_t *t = task_list[i];
        if (t->state == OS_TASK_BLOCKED && os_tick_count >= t->wake_tick) {
            t->state = OS_TASK_READY;
        }
    }
}

void os_scheduler_init(void)
{
    os_heap_init();
    os_tick_count = 0;
    os_reschedule_pending = 0;
    task_count = 0;
    os_current_task = NULL;
    os_next_task = NULL;
}

os_task_t *os_task_create(const char *name, os_task_fn_t fn, uint8_t *stack, void *arg)
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

    task_list[task_count++] = task;
    os_task_init_context(task);
    return task;
}

static os_task_t *pick_next_task(void)
{
    if (task_count == 0) {
        return NULL;
    }

    size_t start = 0;
    if (os_current_task != NULL) {
        for (size_t i = 0; i < task_count; i++) {
            if (task_list[i] == os_current_task) {
                start = (i + 1) % task_count;
                break;
            }
        }
    }

    for (size_t n = 0; n < task_count; n++) {
        size_t i = (start + n) % task_count;
        if (task_list[i]->state == OS_TASK_READY) {
            return task_list[i];
        }
    }

    return os_current_task;
}

void os_schedule(void)
{
    os_task_t *next = pick_next_task();
    if (next == NULL) {
        return;
    }

    if (next == os_current_task) {
        return;
    }

    if (os_current_task != NULL && os_current_task->state == OS_TASK_RUNNING) {
        os_current_task->state = OS_TASK_READY;
    }

    next->state = OS_TASK_RUNNING;
    os_current_task = next;
}

void os_tick_handler(void)
{
    os_tick_count++;
    wake_sleeping_tasks();
    os_reschedule_pending = 1;
    os_schedule();
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
    os_task_t *first = NULL;

    for (size_t i = 0; i < task_count; i++) {
        if (task_list[i]->state == OS_TASK_READY) {
            first = task_list[i];
            break;
        }
    }

    if (first == NULL) {
        os_panic("no runnable task");
    }

    first->state = OS_TASK_RUNNING;
    os_current_task = first;

    port_uart_puts("[kernel] scheduler started\n");
    port_uart_puts("[tick] timer frequency: ");
    print_uint64(port_timer_frequency());
    port_uart_puts(" Hz\n");

    os_start_first_task();
}

uint64_t os_tick_get(void)
{
    return os_tick_count;
}

uint32_t os_time_ms(void)
{
    return (uint32_t)(os_tick_count * 1000U / OS_TICK_HZ);
}

void os_task_delay(uint32_t ms)
{
    os_task_t *self = os_current_task;
    if (self == NULL) {
        return;
    }

    uint64_t ticks = ((uint64_t)ms * OS_TICK_HZ) / 1000U;
    if (ticks == 0) {
        ticks = 1;
    }

    os_critical_enter();
    self->wake_tick = os_tick_count + ticks;
    self->state = OS_TASK_BLOCKED;
    os_critical_exit();

    os_yield();
}
