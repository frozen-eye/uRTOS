#ifndef PORT_H
#define PORT_H

#include <stdint.h>

void port_early_init(void);
void port_boot_banner(void);
void port_timer_init(void);
void port_timer_start(void);
void port_timer_reprogram(void);
uint64_t port_timer_frequency(void);

void port_irq_enable(void);
void port_irq_disable(void);

void port_uart_init(void);
void port_uart_putc(char c);
void port_uart_puts(const char *s);

#endif /* PORT_H */
