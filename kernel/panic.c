#include "os.h"
#include "port.h"

void os_panic(const char *msg)
{
    port_irq_disable();
    port_uart_puts("\n[panic] ");
    if (msg) {
        port_uart_puts(msg);
    }
    port_uart_puts("\n");
    for (;;) {
        __asm__ volatile("wfi");
    }
}
