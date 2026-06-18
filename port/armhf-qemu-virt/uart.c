#include "port.h"
#include "mmio.h"

#define UART0_BASE      0x09000000UL
#define UART_DR         ((volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_FR         ((volatile uint32_t *)(UART0_BASE + 0x18))

void port_uart_init(void)
{
}

void port_uart_putc(char c)
{
    while (mmio_read32(UART_FR) & (1U << 5)) {
    }
    mmio_write32(UART_DR, (uint32_t)c);
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
