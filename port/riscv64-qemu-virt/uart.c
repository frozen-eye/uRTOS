#include "port.h"
#include "mmio.h"

#define UART0_BASE      0x10000000UL
#define UART_THR        ((volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_LSR        ((volatile uint32_t *)(UART0_BASE + 0x05))

void port_uart_init(void)
{
}

void port_uart_putc(char c)
{
    while ((mmio_read32(UART_LSR) & 0x20U) == 0U) {
    }
    mmio_write32(UART_THR, (uint32_t)c);
}

void port_uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            port_uart_putc('\r');
        }
        port_uart_putc(*s++);
    }
}
