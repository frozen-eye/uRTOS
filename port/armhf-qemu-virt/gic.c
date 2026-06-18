#include "gic.h"
#include "mmio.h"

#define GICD_CTLR       ((volatile uint32_t *)(GIC_DIST_BASE + 0x0000))
#define GICD_ISENABLER  ((volatile uint32_t *)(GIC_DIST_BASE + 0x0100))
#define GICD_IPRIORITYR ((volatile uint8_t  *)(GIC_DIST_BASE + 0x0400))
#define GICD_ITARGETSR  ((volatile uint8_t  *)(GIC_DIST_BASE + 0x0800))
#define GICD_ICFGR      ((volatile uint32_t *)(GIC_DIST_BASE + 0x0C00))

#define GICC_CTLR       ((volatile uint32_t *)(GIC_CPU_BASE + 0x0000))
#define GICC_PMR        ((volatile uint32_t *)(GIC_CPU_BASE + 0x0004))
#define GICC_IAR        ((volatile uint32_t *)(GIC_CPU_BASE + 0x000C))
#define GICC_EOIR       ((volatile uint32_t *)(GIC_CPU_BASE + 0x0010))

static void gicd_enable_irq(uint32_t irq)
{
    uint32_t reg = irq / 32U;
    uint32_t bit = irq % 32U;
    mmio_write32(GICD_ISENABLER + reg, 1U << bit);
}

void gic_init(void)
{
    mmio_write32(GICD_CTLR, 0x1);
    mmio_write32(GICC_CTLR, 0x1);
    mmio_write32(GICC_PMR, 0xFF);

    for (uint32_t i = 8; i < 256; i++) {
        GICD_ITARGETSR[i] = 0x01;
    }

    GICD_ICFGR[IRQ_VIRT_TIMER / 16U] = 0;
    GICD_IPRIORITYR[IRQ_VIRT_TIMER] = 0xA0;
    gicd_enable_irq(IRQ_VIRT_TIMER);
}

uint32_t gic_acknowledge(void)
{
    return mmio_read32(GICC_IAR) & 0x3FFU;
}

void gic_end_interrupt(uint32_t irq)
{
    mmio_write32(GICC_EOIR, irq);
}
