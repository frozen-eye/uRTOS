#include "port.h"
#include "mmio.h"

#if OS_CPU_COUNT > 1
#include "spinlock.h"

static os_spinlock_t uart_lock;
#endif

#define UART0_BASE      0x09000000UL
#define UART_DR         ((volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_FR         ((volatile uint32_t *)(UART0_BASE + 0x18))

static void uart_putc_nolock(char c)
{
    while (mmio_read32(UART_FR) & (1U << 5)) {
    }
    mmio_write32(UART_DR, (uint32_t)c);
}

void port_uart_init(void)
{
#if OS_CPU_COUNT > 1
    os_spinlock_init(&uart_lock);
#endif
}

void port_uart_putc(char c)
{
#if OS_CPU_COUNT > 1
    os_spinlock_acquire(&uart_lock);
    uart_putc_nolock(c);
    os_spinlock_release(&uart_lock);
#else
    uart_putc_nolock(c);
#endif
}

void port_uart_puts(const char *s)
{
#if OS_CPU_COUNT > 1
    os_spinlock_acquire(&uart_lock);
#endif

    while (*s) {
        if (*s == '\n') {
            uart_putc_nolock('\r');
        }
        uart_putc_nolock(*s++);
    }

#if OS_CPU_COUNT > 1
    os_spinlock_release(&uart_lock);
#endif
}
