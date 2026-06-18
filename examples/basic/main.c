#include "os.h"
#include "port.h"

#include <stddef.h>

static os_mutex_t counter_mu;
static os_sem_t   tick_sem;
static uint32_t   shared_count;

static int snprintf_uint(char *buf, size_t cap, uint32_t v)
{
    char tmp[12];
    int n = 0;
    int i = 0;

    if (cap == 0U) {
        return 0;
    }

    if (v == 0U) {
        buf[0] = '0';
        return 1;
    }

    while (v > 0U && i < (int)sizeof(tmp)) {
        tmp[i++] = (char)('0' + (v % 10U));
        v /= 10U;
    }

    while (i > 0 && (size_t)n + 1U < cap) {
        buf[n++] = tmp[--i];
    }

    return n;
}

static void print_ms(void)
{
    char buf[32];
    int pos = 0;

    buf[pos++] = '[';
    pos += snprintf_uint(buf + pos, sizeof(buf) - (size_t)pos, os_time_ms());
    buf[pos++] = ' ';
    buf[pos++] = 'm';
    buf[pos++] = 's';
    buf[pos++] = ' ';
    buf[pos++] = 'c';
    buf[pos++] = 'p';
    buf[pos++] = 'u';
    pos += snprintf_uint(buf + pos, sizeof(buf) - (size_t)pos, os_cpu_id());
    buf[pos++] = ']';
    buf[pos++] = ' ';
    buf[pos] = '\0';
    port_uart_puts(buf);
}

static void print_count(uint32_t count)
{
    char buf[16];
    int pos = 0;

    port_uart_puts("count=");
    pos += snprintf_uint(buf + pos, sizeof(buf) - (size_t)pos, count);
    buf[pos] = '\0';
    port_uart_puts(buf);
    port_uart_puts("\n");
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

static void task_producer(void *arg)
{
    (void)arg;
    for (;;) {
        os_task_delay(400);
        os_sem_post(&tick_sem);
    }
}

static void task_consumer(void *arg)
{
    (void)arg;
    for (;;) {
        os_sem_wait(&tick_sem);

        os_mutex_lock(&counter_mu);
        shared_count++;
        print_ms();
        print_count(shared_count);
        os_mutex_unlock(&counter_mu);
    }
}

void app_main(void)
{
    os_mutex_init(&counter_mu);
    os_sem_init(&tick_sem, 0U);

    os_task_create("A", task_a, NULL, NULL, OS_TASK_PRIO_NORMAL);
    os_task_create("B", task_b, NULL, NULL, OS_TASK_PRIO_HIGH);
    os_task_create("prod", task_producer, NULL, NULL, OS_TASK_PRIO_NORMAL);
    os_task_create("cons", task_consumer, NULL, NULL, OS_TASK_PRIO_HIGH);
}
