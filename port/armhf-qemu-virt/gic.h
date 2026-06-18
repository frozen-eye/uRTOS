#ifndef GIC_H
#define GIC_H

#include <stdint.h>

#define GIC_DIST_BASE       0x08000000UL
#define GIC_CPU_BASE        0x08010000UL

#define IRQ_VIRT_TIMER      27
#define IRQ_SGI_RESCHEDULE  1

void gic_init(void);
void gic_cpu_init(void);
void gic_send_sgi(uint32_t sgi, uint16_t cpu_mask);
uint32_t gic_acknowledge(void);
void gic_end_interrupt(uint32_t irq);

#endif /* GIC_H */
