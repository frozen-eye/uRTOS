#include "os.h"
#include "port.h"

static void print_ms(void)
{
    char buf[16];
    uint32_t ms = os_time_ms();
    int i = 0;

    port_uart_putc('[');
    if (ms == 0) {
        port_uart_putc('0');
    } else {
        while (ms > 0 && i < (int)sizeof(buf)) {
            buf[i++] = (char)('0' + (ms % 10U));
            ms /= 10U;
        }
        while (i > 0) {
            port_uart_putc(buf[--i]);
        }
    }
    port_uart_puts(" ms] ");
}

static void task_a(void *arg)
{
    (void)arg;
    for (;;) {
        print_ms();
        port_uart_puts("task A: alive\n");
        os_task_delay(1000);
    }
}

static void task_b(void *arg)
{
    (void)arg;
    for (;;) {
        print_ms();
        port_uart_puts("task B: alive\n");
        os_task_delay(500);
    }
}

static void task_c(void *arg)
{
    (void)arg;
    /* CPU-bound loop — never calls os_yield(); preemption must come from the timer IRQ. */
    volatile uint64_t spin = 0;
    for (;;) {
        spin++;
    }
}

void app_main(void)
{
    os_task_create("A", task_a, NULL, NULL);
    os_task_create("B", task_b, NULL, NULL);
    os_task_create("C", task_c, NULL, NULL);
}
