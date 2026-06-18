#ifndef GIC_H
#define GIC_H

#include <stdint.h>

/*
 * QEMU virt (ARM64) GICv2 MMIO bases.
 * See QEMU hw/arm/virt.c and hw/intc/arm_gic.c.
 */
#define GIC_DIST_BASE       0x08000000UL
#define GIC_CPU_BASE        0x08010000UL

/*
 * Private Peripheral Interrupt: ARM Generic Timer (physical, non-secure).
 * PPI 30 on GICv2 for CNTPNSIRQ.
 */
#define IRQ_VIRT_TIMER      30

void gic_init(void);
uint32_t gic_acknowledge(void);
void gic_end_interrupt(uint32_t irq);

#endif /* GIC_H */
