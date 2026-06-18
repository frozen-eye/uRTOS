#ifndef GIC_H
#define GIC_H

#include <stdint.h>

#define GIC_DIST_BASE       0x08000000UL
#define GIC_CPU_BASE        0x08010000UL
#define IRQ_VIRT_TIMER      30

void gic_init(void);
uint32_t gic_acknowledge(void);
void gic_end_interrupt(uint32_t irq);

#endif /* GIC_H */
