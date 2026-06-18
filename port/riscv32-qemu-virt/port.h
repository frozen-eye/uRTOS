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

uint32_t port_cpu_id(void);
void port_cpu_init(uint32_t cpu_id);
void port_wfe(void);
void port_smp_boot_secondaries(void);
void port_smp_send_reschedule_ipi(void);

#endif /* PORT_H */
