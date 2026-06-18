#include "port.h"
#include "mmio.h"
#include "os_config.h"

#include <stdint.h>

#define CLINT_BASE          0x02000000UL
#define CLINT_MTIMECMP(h)   ((volatile uint64_t *)(CLINT_BASE + 0x4000UL + (h) * 8UL))
#define CLINT_MTIME         ((volatile uint64_t *)(CLINT_BASE + 0xBFF8UL))

static uint64_t timer_freq;

void port_timer_init(void)
{
    timer_freq = 10000000UL; /* QEMU virt default timebase */
    port_timer_reprogram();
}

void port_timer_start(void)
{
    uint64_t now = mmio_read64(CLINT_MTIME);
    mmio_write64(CLINT_MTIMECMP(0), now + (timer_freq / OS_TICK_HZ));
}

void port_timer_reprogram(void)
{
    uint64_t now = mmio_read64(CLINT_MTIME);
    uint64_t delta = timer_freq / OS_TICK_HZ;
    if (delta == 0) {
        delta = 1;
    }
    mmio_write64(CLINT_MTIMECMP(0), now + delta);
}

uint64_t port_timer_frequency(void)
{
    return timer_freq;
}

void port_irq_enable(void)
{
    __asm__ volatile("csrs mstatus, %0" :: "r"(0x8UL));  /* MIE */
}

void port_irq_disable(void)
{
    __asm__ volatile("csrc mstatus, %0" :: "r"(0x8UL));
}

void port_early_init(void)
{
    port_uart_init();
}
